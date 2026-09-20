$p='src/SynthVoice.cpp'
$L=[System.Collections.Generic.List[string]](Get-Content $p)
function Ins($anchor,[string[]]$block){
  for($i=0;$i -lt $L.Count;$i++){
    if($L[$i].Trim() -eq [string]$anchor.Trim()){
      $L.InsertRange($i+1,[string[]]$block); return $true
    }
  }
  return $false
}
# add ring pool param to both resolve signatures (identical line appears twice)
$sig='    juce::ignoreUnused (params, noteSources);'
# resolveInput: after juce::ignoreUnused line is safe? We insert block before oscillator branch.
Ins '        if (type == NodeType::oscillator' @(
'        if (type == NodeType::ringModulator',
'         && index >= 0 && index < GraphNodeRegistry::maxRingModulators',
'         && ringModulators[static_cast<size_t> (index)] != nullptr)',
'        {',
'            if (portId == "in1") return &ringModulators[static_cast<size_t> (index)]->getInput1();',
'            if (portId == "in2") return &ringModulators[static_cast<size_t> (index)]->getInput2();',
'            return nullptr;',
'        }',
''
)
[System.IO.File]::WriteAllLines((Resolve-Path $p).Path,$L,(New-Object System.Text.UTF8Encoding($false)))
'done part1'