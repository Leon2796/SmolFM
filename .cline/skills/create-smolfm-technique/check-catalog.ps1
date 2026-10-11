# Structural check for the SmolFM technique catalogue (doc/techniques):
# - exactly one "graph_ascii:" per technique, one "parameters:" per file
#   (exception: full-import aliases like auto-sweep-resonance carry no own
#   involved_nodes/parameters by design),
# - the graph_ascii block scalar stays intact (every line until the next
#   two-space key is blank or indented >= 4),
# - imports.name resolves to an existing catalogue file (import integrity),
# - used_by mirrors imports in both directions: every import target lists
#   its importer in used_by, every used_by name imports its owner back,
# - instances entries in every summarizing registry (any *.yaml with an
#   `instances:` section: driver-patterns, delay-patterns,
#   stereo-branch-patterns) resolve to existing catalogue files, are
#   duplicate-free ACROSS ALL REGISTRIES COMBINED, and each carries
#   exactly one graph_ascii, one verlauf and one tonal.
# Usage: powershell -ExecutionPolicy Bypass -File check-catalog.ps1

$root = Join-Path $PSScriptRoot '..\..\..\'
$root = [IO.Path]::GetFullPath($root)
$root = Join-Path $root 'doc\techniques'
$bad = New-Object System.Collections.Generic.List[string]
Get-ChildItem $root -Filter *.yaml | ForEach-Object {
  $c = [IO.File]::ReadAllLines($_.FullName)
  $g = @(); $p = @()
  for ($i = 0; $i -lt $c.Count; $i++) {
    if ($c[$i] -eq '  graph_ascii: |') { $g += $i }
    if ($c[$i] -eq '  parameters:')    { $p += $i }
  }
  if ($g.Count -ne 1) { $bad.Add($_.Name + '|graph_ascii-count:' + $g.Count) }
  if ($p.Count -ne 1 -and $_.Name -ne 'auto-sweep-resonance.yaml') {
    $bad.Add($_.Name + '|parameters-count:' + $p.Count)
  }
  if ($g.Count -eq 1) {
    for ($j = $g[0] + 1; $j -lt $c.Count; $j++) {
      $l = $c[$j]
      if ($l -match '^  [a-z_]+:') { break }
      if ($l.Trim() -ne '' -and $l -notmatch '^\s{4,}') {
        $bad.Add($_.Name + '|ascii-block@row' + ($j + 1))
      }
    }
  }
}
Write-Output ('struct problems: ' + $bad.Count)
$bad | ForEach-Object { Write-Output ('  ' + $_) }

$bi = New-Object System.Collections.Generic.List[string]
Get-ChildItem $root -Filter *.yaml | ForEach-Object {
  $t = [IO.File]::ReadAllText($_.FullName)
  if ($t.Contains([string][char]10 + '  imports:')) {
    foreach ($x in [regex]::Matches($t, '(?m)^    - name: (\S+)$')) {
      $n = $x.Groups[1].Value
      if (-not (Test-Path (Join-Path $root ($n + '.yaml')))) {
        $bi.Add($_.Name + ' -> ' + $n)
      }
    }
  }
}
Write-Output ('broken imports: ' + $bi.Count)
$bi | ForEach-Object { Write-Output ('  ' + $_) }

# used_by <-> imports bidirectional integrity
$uBad = New-Object System.Collections.Generic.List[string]
$table = @{}
Get-ChildItem $root -Filter *.yaml | ForEach-Object {
  $c = [IO.File]::ReadAllLines($_.FullName)
  $sec = ''; $imports = @{}; $usedBy = @{}
  foreach ($l in $c) {
    if ($l -match '^  ([a-z_]+):') { $sec = $matches[1] }
    elseif (($sec -eq 'imports' -or $sec -eq 'used_by') -and $l -match '^    - name: (.*)$') {
      $n = $matches[1].Trim()
      if ($n.Contains(' (imports')) { $n = $n.Substring(0, $n.IndexOf(' (imports')).Trim() }
      if ($n -ne '' -and -not $n.Contains('<')) {
        if ($sec -eq 'imports') { $imports[$n] = $true } else { $usedBy[$n] = $true }
      }
    }
  }
  $table[$_.BaseName] = @{ i = $imports; u = $usedBy }
}
foreach ($fname in $table.Keys) {
  $entry = $table[$fname]
  foreach ($x in $entry.i.Keys) {
    if (-not $table.ContainsKey($x)) { $uBad.Add($fname + ' -> missing import target: ' + $x); continue }
    if (-not $table[$x].u.ContainsKey($fname)) { $uBad.Add('missing used_by: ' + $x + ' does not list ' + $fname) }
  }
  foreach ($x in $entry.u.Keys) {
    if (-not $table.ContainsKey($x)) { $uBad.Add($fname + ' -> missing used_by target: ' + $x); continue }
    if (-not $table[$x].i.ContainsKey($fname)) { $uBad.Add('reverse import missing: ' + $x + ' does not import ' + $fname) }
  }
}
Write-Output ('used_by problems: ' + $uBad.Count)
$uBad | ForEach-Object { Write-Output ('  ' + $_) }

# instances: resolution, duplicate count, per-entry field requirements
# (every *.yaml with an `instances:` section = a summarizing registry)
$names = @()
$iBad = New-Object System.Collections.Generic.List[string]
Get-ChildItem $root -Filter *.yaml | ForEach-Object {
  $c = [IO.File]::ReadAllLines($_.FullName)
  $inI = $false; $cur = ''; $g = 0; $v = 0; $t = 0
  foreach ($l in $c) {
    if ($l -match '^  ([a-z_]+):') {
      if ($inI -and $cur -ne '') {
        if ($g -ne 1) { $iBad.Add($_.BaseName + '|instance ' + $cur + '|graph_ascii-count:' + $g) }
        if ($v -ne 1) { $iBad.Add($_.BaseName + '|instance ' + $cur + '|verlauf-count:' + $v) }
        if ($t -ne 1) { $iBad.Add($_.BaseName + '|instance ' + $cur + '|tonal-count:' + $t) }
      }
      $inI = ($matches[1] -eq 'instances')
      $cur = ''; $g = 0; $v = 0; $t = 0
      continue
    }
    if (-not $inI) { continue }
    if ($l -match '^    - name: (.+)$') {
      if ($cur -ne '') {
        if ($g -ne 1) { $iBad.Add($_.BaseName + '|instance ' + $cur + '|graph_ascii-count:' + $g) }
        if ($v -ne 1) { $iBad.Add($_.BaseName + '|instance ' + $cur + '|verlauf-count:' + $v) }
        if ($t -ne 1) { $iBad.Add($_.BaseName + '|instance ' + $cur + '|tonal-count:' + $t) }
      }
      $n = $matches[1].Trim()
      if ($n.Contains(' (imports')) { $n = $n.Substring(0, $n.IndexOf(' (imports')).Trim() }
      if ($n -ne '') {
        $cur = $n
        $names += $n
        if (-not (Test-Path (Join-Path $root ($n + '.yaml')))) { $iBad.Add('broken instance: ' + $n) }
      }
      $g = 0; $v = 0; $t = 0
    }
    elseif ($l -eq '      graph_ascii: |') { $g++ }
    elseif ($l -eq '      verlauf: |')     { $v++ }
    elseif ($l -eq '      tonal: |')       { $t++ }
  }
  if ($cur -ne '') {
    if ($g -ne 1) { $iBad.Add($_.BaseName + '|instance ' + $cur + '|graph_ascii-count:' + $g) }
    if ($v -ne 1) { $iBad.Add($_.BaseName + '|instance ' + $cur + '|verlauf-count:' + $v) }
    if ($t -ne 1) { $iBad.Add($_.BaseName + '|instance ' + $cur + '|tonal-count:' + $t) }
  }
}
$d = @($names | Group-Object | Where-Object { $_.Count -gt 1 })
Write-Output ('instances: ' + $names.Count + ', duplicates: ' + $d.Count + ', field problems: ' + $iBad.Count)
if ($d) { $d | ForEach-Object { Write-Output ('  duplicate instance: ' + $_.Name) } }
$iBad | ForEach-Object { Write-Output ('  ' + $_) }