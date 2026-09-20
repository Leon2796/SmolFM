function Ins-After ($path, [string]$anchor, [string[]]$add)
{
    $L = [System.Collections.Generic.List[string]](Get-Content $path)
    for ($i = 0; $i -lt $L.Count; $i++)
    {
        if ($L[$i].Trim() -eq $anchor.Trim())
        {
            $L.InsertRange($i + 1, [string[]]$add)
            [System.IO.File]::WriteAllLines((Resolve-Path $path).Path, $L, (New-Object System.Text.UTF8Encoding($false)))
            Write-Host "OK  $path  <- [$($add[0].Trim())]"
            return $true
        }
    }
    Write-Host "MISS $path anchor='$anchor'"
    return $false
}

# ---------- SynthVoice.h ----------
Ins-After 'src/SynthVoice.h' '#include "processors/MasterOutputProcessor.h"' @('#include "processors/RingModulatorProcessor.h"')
Ins-After 'src/SynthVoice.h' 'std::array<AdsrProcessor*, GraphNodeRegistry::maxAdsr> adsrProcessors {};' @('    std::array<RingModulatorProcessor*, GraphNodeRegistry::maxRingModulators> ringModulators {};')

# ---------- SynthVoice.cpp ----------
# buildGraph: ring pool right after the adsr pool
$L = [System.Collections.Generic.List[string]](Get-Content 'src/SynthVoice.cpp')
for ($i = 0; $i -lt $L.Count; $i++)
{
    if ($L[$i].Trim() -eq '// Master output (singleton).')
    {
        $block = @(
            '    // Ring modulator pool — pure signal multipliers, no parameters.',
            '    for (int i = 0; i < GraphNodeRegistry::maxRingModulators; ++i)',
            '    {',
            '        auto ring = std::make_unique<RingModulatorProcessor>();',
            '        ringModulators[static_cast<size_t> (i)] = ring.get();',
            '        graph.addProcessor (std::move (ring));',
            '    }',
            ''
        )
        $L.InsertRange($i, [string[]]$block)
        [System.IO.File]::WriteAllLines((Resolve-Path 'src/SynthVoice.cpp').Path, $L, (New-Object System.Text.UTF8Encoding($false)))
        Write-Host 'OK  buildGraph ring pool'
        break
    }
}
#L = $null