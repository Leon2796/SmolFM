# AdsrProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

ADSR-Hüllkurve als letzter Knoten der Kette. Multipliziert das eingehende
Audiosignal mit dem Hüllkurvenwert. Wrappt `juce::ADSR`; Parameter werden bei
Note-On gelesen (JUCE-Empfehlung) und wirken daher auf die nächste Note.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 1 | `GraphNodeRegistry::maxAdsr` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `in` (`PortType::signal`) | `InputPort input` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |
| Output-Ports | `out` (`PortType::signal`) | `OutputPort output` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Attack | `attack` | Float, 0.001–5 s | `std::atomic<float>* attack` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |
| Decay | `decay` | Float, 0.001–5 s | `std::atomic<float>* decay` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |
| Sustain | `sustain` | Float, 0–1 | `std::atomic<float>* sustain` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |
| Release | `release` | Float, 0.001–10 s | `std::atomic<float>* release` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(noch keine)* | vier Rotary-Slider (A/D/S/R) | `AdsrPanel::attackSlider` u. a. | [src/gui/components/AdsrPanel.h](../../src/gui/components/AdsrPanel.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $x_n$ | Eingangssample | Amplitude |
| $g_n$ | Hüllkurvenwert (Gain) | 0–1 |
| $A, D, R$ | Attack-, Decay-, Release-Zeit | s |
| $S$ | Sustain-Pegel | 0–1 |
| $y_n$ | Ausgangssample | Amplitude |

Funktionsablauf:

Bei Note-On wird die Hüllkurve mit $(A, D, S, R)$ parametrisiert und gestartet
(`noteOn`), bei Note-Off läuft die Release-Phase (`noteOff`). `juce::ADSR`
liefert pro Sample den Gain $g_n$ entlang der linearen Attack- und
exponentiellen Decay/Release-Segmente.

Pro Sample:

$$y_n = x_n \cdot g_n$$

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $A, D, S, R$ | `attack->load()` … `release->load()` → `juce::ADSR::Parameters` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | in `startNote()` gelesen, `envelope.setParameters(params)` |
| $g_n$ | `envelope.getNextSample()` → `env` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | JUCE-ADSR-State-Machine; Sample-Rate via `envelope.setSampleRate()` in `prepare()` |
| $x_n$ | `input.getSample()` → `sourceSample` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | Port lesen (Default 0.0f unverbunden) |
| $y_n$ | `sourceSample * env` → `output.setSample()` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | Multiplikation, Ergebnis in den Port |
| Note-Off | `noteOff()` → `envelope.noteOff()` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | aus `SynthVoice::stopNote` bei `allowTailOff` |
| Aktiv-Flag | `isActive()` → `envelope.isActive()` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | Voice wird freigegeben, wenn Hüllkurve endet |
