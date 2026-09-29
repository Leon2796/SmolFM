# MasterOutputProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Endstufe des per-Voice-Graphen. Mischt bis zu acht Signaleingänge zu einem
stereofähigen Ausgangsframe (kanalweise Summierung: L zu L, R zu R) und
skaliert das Ergebnis mit dem Mastervolume. Der höchste Pegel beider Kanäle
wird atomar publiziert, damit die UI eine Pegelanzeige zeigen kann.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 1 | `GraphNodeRegistry::maxMasterOutputs` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `in1` … `in8` (`PortType::signal`) | `std::array<InputPort, 8> inputs` | [src/processors/MasterOutputProcessor.h](../../src/processors/MasterOutputProcessor.h) |
| Output-Ports | *(keiner — Endstufe)* | — | — |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Master Level | `masterLevel` | Float, 0–1 (Default 0.8) | `std::atomic<float>* level` | [src/processors/MasterOutputProcessor.h](../../src/processors/MasterOutputProcessor.h) |

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(noch keine)* | Rotary-Slider (Level) + Pegelanzeige | `MasterOutputComponent::levelSlider`, `MasterOutputComponent::displayedPeak` | [src/gui/components/MasterOutputComponent.h](../../src/gui/components/MasterOutputComponent.h) |

Die Pegelanzeige pollt pro 30 Hz-Timer den höchsten Peak aller Voices über
`AudioPluginAudioProcessor::getMasterPeakLevel()`.

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | Index des gerade berechneten Samples | — |
| $x_{k,n}$ | Stereopaar am Eingang $k$ im Sample $n$ | Amplitude |
| $L$ | Mastervolume | — |
| $y_n$ | Ausgangs-Stereopaar $(y_{n,L},\, y_{n,R})$ | Amplitude |

Pro Sample (kanalweise):

$$
y_{n,L} = L \sum_{k=1}^{8} x_{k,n,L} \qquad y_{n,R} = L \sum_{k=1}^{8} x_{k,n,R}
$$

Kurz erklärt: Der Prozessor summiert alle acht Eingänge kanalweise (L zu L, R
zu R) und multipliziert mit dem Master-Slider. Unverbundene Eingänge liefern
0 und verändern die Summe nicht.

Die Pegelanzeige folgt einem Peak-Follower mit schnellem Attack und langsamem
Release (`smoothedPeak`), damit die Anzeige nicht flackert; der lautere der
beiden Kanäle bestimmt den angezeigten Peak.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x_{k,n}$ | `inputs[k].getSample()` | [MasterOutputProcessor.cpp](../../src/processors/MasterOutputProcessor.cpp) | Port lesen (Stereopaar); Default 0 wenn unverbunden |
| $L$ | `level->load()` | [MasterOutputProcessor.cpp](../../src/processors/MasterOutputProcessor.cpp) | atomarer Read des APVTS-Werts |
| $y_n$ | `out` (Rückgabewert) | [MasterOutputProcessor.cpp](../../src/processors/MasterOutputProcessor.cpp) | kanalweise Summe × Level; Peak = max(|y_L|, |y_R|) in `peakLevel` publiziert |
