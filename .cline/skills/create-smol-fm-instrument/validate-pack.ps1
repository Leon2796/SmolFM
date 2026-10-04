# Validates SmolFM v3 instruments: XML well-formedness, node bases, budgets,
# note/output uniqueness, ranges, enums, port kinds, fan-in rule, layout
# completeness, output reachability.
# Usage: powershell -File validate-pack.ps1 [-Folder instruments/witch-house-sleaze]
param([string]$Folder = "instruments/witch-house-sleaze")
$ErrorCount=0; $FileCount=0
function Base([string]$id) { return ($id -replace '\d+$','') }
$PortSpec=@{
  note=@{ins=@{};outs=@{out='frequency';vel='velocity'}}
  osc=@{ins=@{note_in='frequency'};outs=@{out='signal'}}
  fm=@{ins=@{freq_in='frequency';modulator_in='signal';amount_mod_in='signal';vel='velocity'};outs=@{out='frequency'}}
  fscale=@{ins=@{freq_in='frequency'};outs=@{out='frequency'}}
  adsr=@{ins=@{in='signal'};outs=@{out='signal'}}
  fadsr=@{ins=@{freq_in='frequency';vel='velocity'};outs=@{out='frequency'}}
  gain=@{ins=@{in='signal';factor_mod_in='signal';vel='velocity'};outs=@{out='signal'}}
  shape=@{ins=@{in='signal';drive_mod_in='signal';vel='velocity'};outs=@{out='signal'}}
  filter=@{ins=@{in='signal';cutoff_mod_in='frequency';resonance_mod_in='signal';vel='velocity'};outs=@{out='signal'}}
  resonator=@{ins=@{in='signal';tune='frequency';q_mod_in='signal';vel='velocity';strike_vel='velocity'};outs=@{out='signal'}}
  vsc=@{ins=@{vel_in='velocity'};outs=@{out='velocity'}}
  feedback=@{ins=@{in='signal';fb_in='signal'};outs=@{out='signal'}}
  delay=@{ins=@{in='signal';time_mod_in='signal'};outs=@{out='signal'}}
  ring=@{ins=@{in1='signal';in2='signal'};outs=@{out='signal'}}
  am=@{ins=@{carrier_in='signal';modulator_in='signal'};outs=@{out='signal'}}
  add=@{ins=@{in1='signal';in2='signal'};outs=@{out='signal'}}
  merge=@{ins=@{in1='signal';in2='signal'};outs=@{out='signal'}}
  split=@{ins=@{in='signal'};outs=@{outL='signal';outR='signal'}}
  output=@{ins=@{in1='signal';in2='signal';in3='signal';in4='signal';in5='signal';in6='signal';in7='signal';in8='signal'};outs=@{}}
}
$Max=@{note=1;osc=16;fm=8;fscale=16;vsc=8;ring=8;am=8;delay=16;adsr=16;fadsr=8;gain=16;shape=16;filter=16;resonator=8;feedback=4;add=8;split=8;merge=8;output=1}
$Range=@{
  'osc.staticfreq'=20,20000;'osc.lforate'=0.01,50;'fm.amount'=0,10;'fscale.factor'=0,10
  'adsr.attack'=0.001,5;'adsr.decay'=0.001,5;'adsr.sustain'=0,1;'adsr.release'=0.001,10
  'fadsr.attack'=0.001,5;'fadsr.decay'=0.001,5;'fadsr.sustain'=0,1;'fadsr.release'=0.001,10
  'fadsr.up'=0,10;'fadsr.down'=0,10;'gain.factor'=0,10;'shape.drive'=0,1
  'filter.cutoff'=20,20000;'filter.resonance'=0,1;'delay.time'=1,2000;'am.amount'=0,1
  'delay.feedback'=0,0.95;'delay.mix'=0,1;'output.level'=0,1
  'resonator.tune'=20,8000;'resonator.q'=0,1;'resonator.decay'=0.01,10;'resonator.spread'=0.25,4;'resonator.strikelevel'=0,2;'resonator.striketime'=0,0.2;'feedback.amount'=0,0.95;'vsc.scale'=0.25,4
}
$Enums=@{
  'osc.waveform'=@('sine','saw','square','triangle','noise','perlin','simplex');'osc.mode'=@('pitch','static','lfo')
  'shape.shape'=@('soft','hard','fold');'filter.mode'=@('lp','bp','hp','notch')
  'delay.sync'=@('off','on');'delay.division'=@('half','quarter','eighth','sixteenth')
  'fscale.interval'=@('none','octave_up','octave_down','fifth_up','fifth_down','fourth_up','fourth_down','minor_third_up','major_third_up','semitone_up','semitone_down')
  'vsc.curve'=@('linear','sqrt','square')
}
$files=Get-ChildItem $Folder -Filter *.smolfm | Sort-Object Name
foreach ($f in $files)
{
  $FileCount++; $errs=@()
  try { $doc=[xml](Get-Content $f.FullName -Raw) } catch { Write-Output ("FAIL(XML) "+$f.Name); $ErrorCount++; continue }
  $root=$doc.SmolFM
  if ($null -eq $root) { Write-Output ("FAIL(root) "+$f.Name); $ErrorCount++; continue }
  $nodes=@($root.Graph.Nodes.Node); $ids=@{}
  foreach ($n in $nodes) { $b=Base $n.id; if ($ids.ContainsKey($n.id)) { $errs+="duplicate id: $($n.id)" }; if (-not $PortSpec.ContainsKey($b)) { $errs+="unknown base: $($n.id)" }; $ids[$n.id]=$b }
  $count=@{}
  foreach ($n in $nodes) { $b=Base $n.id; if ($count.ContainsKey($b)) { $count[$b]++ } else { $count[$b]=1 } }
  foreach ($b in $count.Keys) { if ($PortSpec.ContainsKey($b) -and $count[$b] -gt $Max[$b]) { $errs+="budget: $($count[$b]) x $b" } }
  if ($count['note'] -ne 1) { $errs+="note count wrong" }
  if ($count['output'] -ne 1) { $errs+="output count wrong" }
  foreach ($n in $nodes) { $b=Base $n.id
    foreach ($attr in $n.Attributes) { if ($attr.Name -eq 'id') { continue }
      $key="$b.$($attr.Name)"
      if ($Range.ContainsKey($key)) { $v=0.0
        if (-not [double]::TryParse($attr.Value, [System.Globalization.NumberStyles]::Float, [System.Globalization.CultureInfo]::InvariantCulture, [ref]$v)) { $errs+="bad number: $($n.id) $($attr.Name)" }
        elseif ($v -lt $Range[$key][0] -or $v -gt $Range[$key][1]) { $errs+="range: $($n.id) $($attr.Name)=$v" }
      } elseif ($Enums.ContainsKey($key)) { if ($Enums[$key] -notcontains $attr.Value) { $errs+="enum: $($n.id) $($attr.Name)=$($attr.Value)" } }
      else { $errs+="unknown attr: $($n.id) $($attr.Name)" }
    }
  }
  $rev=@{}; $fanin=@{}
  if ($null -ne $root.Graph.Connections) { $wires=@($root.Graph.Connections.Wire) } else { $wires=@() }
  foreach ($w in $wires) {
    if (-not $ids.ContainsKey($w.from)) { $errs+="wire from missing node: $($w.from)"; continue }
    if (-not $ids.ContainsKey($w.to)) { $errs+="wire to missing node: $($w.to)"; continue }
    $fb=Base $w.from; $tb=Base $w.to
    if (-not $PortSpec.ContainsKey($fb) -or -not $PortSpec.ContainsKey($tb)) { continue }
    if (-not $PortSpec[$fb].outs.ContainsKey($w.fromPort)) { $errs+="not an output port: $($w.from).$($w.fromPort)" }
    if ($PortSpec[$tb].ins.ContainsKey($w.toPort)) {
      if ($PortSpec[$fb].outs[$w.fromPort] -ne $PortSpec[$tb].ins[$w.toPort]) { $errs+="kind mismatch: $($w.from).$($w.fromPort) -> $($w.to).$($w.toPort)" }
      $k="$($w.to).$($w.toPort)"
      if ($fanin.ContainsKey($k)) { $errs+="fan-in: multiple wires into $k" }
      $fanin[$k]=$true
      if ($rev.ContainsKey($k)) { $rev[$k]+=$w.from } else { $rev[$k]=@($w.from) }
    } else { $errs+="not an input port: $($w.to).$($w.toPort)" }
  }
  $boxed=@{}
  if ($null -ne $root.Layout -and $null -ne $root.Layout.Boxes) { foreach ($bx in @($root.Layout.Boxes.Box)) { if ($boxed.ContainsKey($bx.id)) { $errs+="duplicate box: $($bx.id)" }; $boxed[$bx.id]=$true } }
  foreach ($n in $nodes) { if (-not $boxed.ContainsKey($n.id)) { $errs+="no layout box: $($n.id)" } }
  $reach=@{}
  foreach ($n in $nodes) { $reach[$n.id]=($n.id -eq 'output') }
  $changed=$true
  while ($changed) { $changed=$false
    foreach ($k in $rev.Keys) { $tgt=($k -split '\.')[0]
      if ($reach[$tgt]) { foreach ($src in @($rev[$k])) { if (-not $reach[$src]) { $reach[$src]=$true; $changed=$true } } }
    }
  }
  foreach ($n in $nodes) { if (-not $reach[$n.id] -and $ids[$n.id] -ne 'note') { $errs+="dangling: $($n.id)" } }
  if ($errs.Count -eq 0) { Write-Output ("OK   "+$f.Name) } else {
    $ErrorCount++
    Write-Output ("FAIL "+$f.Name)
    foreach ($e in $errs) { Write-Output ("      - "+$e) }
  }
}
Write-Output ("== files: $FileCount, failed: $ErrorCount ==")