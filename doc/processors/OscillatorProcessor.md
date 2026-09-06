# OscillatorProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Oszillator-Knoten mit wählbarer Wellenform. Dient als Carrier (am Ende einer
FM-Kette, gespeist über `note_in`) oder als Modulator (speist `signal` in einen
FM-Modulator-Eingang). Rendert das Audiosignal und integriert dabei die
Momentanfrequenz in die Phase — das ist die Stelle, an der echte FM entsteht.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 8 | `GraphNodeRegistry::maxOscillators` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `note_in` (`PortType::frequency`) | `InputPort noteInput` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |
| Output-Ports | `out` (`PortType::signal`) | `OutputPort output` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Wellenform | `osc<N>Waveform` | Choice: Sine/Saw/Square/Triangle/Noise | `std::atomic<float>* waveform` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |
| Modus | `osc<N>Mode` | Choice: Pitch/Static/LFO, Default Pitch | `std::atomic<float>* mode` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |
| Statische Frequenz | `osc<N>StaticFreq` | Float, 20–20000 Hz, Default 440 | `std::atomic<float>* staticFreq` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |
| LFO-Rate | `osc<N>LfoRate` | Float, 0.01–50 Hz, Default 1 | `std::atomic<float>* lfoRate` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |

`<N>` = Instanzindex 0–7.

Frequenzquellen:
- **Pitch-Modus** (`Mode = Pitch`): Die Frequenz kommt ausschließlich über
  `note_in`; ohne Verbindung liefert der Oszillator 0 Hz (Stille).
- **Static-Modus** (`Mode = Static`): Die Frequenz ist der feste
  `StaticFreq`-Wert (20–20000 Hz). `note_in` wird ignoriert. Note-On resettet
  die Phase und hält die Voice am Leben.
- **LFO-Modus** (`Mode = LFO`): Die Frequenz ist der feste `LfoRate`-Wert
  (0.01–50 Hz). `note_in` wird ignoriert. Bei Note-On wird die Phase
  zurückgesetzt, sodass jede Note mit der gleichen LFO-Phase startet —
  die Rate selbst bleibt konstant.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| Frequenz-Modus-Umschalter | Wählt zwischen Pitch (note_in getrieben), Static (feste Audio-Frequenz) und LFO (feste Niedrigfrequenz) | `OscillatorPanel::modeBox` | [src/gui/components/OscillatorPanel.h](../../src/gui/components/OscillatorPanel.h) |
| Static-Freq-Regler | Feste Frequenz im Audiorange, 20–20000 Hz; nur im Static-Modus sichtbar | `OscillatorPanel::staticFreqSlider` | [src/gui/components/OscillatorPanel.h](../../src/gui/components/OscillatorPanel.h) |
| LFO-Rate-Regler | Feste LFO-Frequenz, 0.01–50 Hz; nur im LFO-Modus sichtbar | `OscillatorPanel::lfoRateSlider` | [src/gui/components/OscillatorPanel.h](../../src/gui/components/OscillatorPanel.h) |
| *(Basis)* | ComboBox (Wellenform) | `OscillatorPanel::waveformBox` | [src/gui/components/OscillatorPanel.h](../../src/gui/components/OscillatorPanel.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $f$ | effektive Frequenz (Port oder Slider) | Hz |
| $f_s$ | Sample-Rate | Hz |
| $\Delta\varphi$ | Phase-Increment pro Sample | rad |
| $\varphi_n$ | Phase im Sample $n$ | rad |
| $m$ | Wellenform-Index (0–3) | — |
| $w(\cdot)$ | Wellenformfunktion | — |
| $out_n$ | Ausgangssample | Amplitude |

Funktionsablauf pro Sample:

1. Frequenzwahl: Im Pitch-Modus $f = f_{note\_in}$ falls verbunden, sonst $0$
   (kein Ton, kein Slider-Fallback). Im Static-Modus
   $f = \mathrm{clamp}(f_{\text{static}}, 20, 20000)$; im LFO-Modus
   $f = \mathrm{clamp}(f_{\text{rate}}, 0.01, 50)$ — jeweils unabhängig von
   `note_in`; Note-On resettet die Phase.
2. Phase-Increment:
$$\Delta\varphi = \frac{2\pi f}{f_s}$$
3. Phasenintegration mit Wrap in $[0, 2\pi)$:
$$\varphi_{n+1} = \mathrm{fmod}(\varphi_n + \Delta\varphi,\ 2\pi)$$

   Falls das Ergebnis negativ ist (kann bei durch FM erzeugten negativen
   Momentanfrequenzen auftreten), wird $2\pi$ addiert, sodass
   $\varphi_{n+1} \in [0, 2\pi)$ gilt.

   Kurz erklärt: Die Phase ist der aktuelle Ort innerhalb einer Wellenrunde.
   Eine volle Runde hat $2\pi$ Radiant. Bei $f_s$ Samples pro Sekunde muss eine
   Frequenz von $f$ Hertz deshalb bei jedem Sample um $2\pi f / f_s$ weitergehen:
   Nach $f_s / f$ Samples ist genau eine Runde erreicht. Das Addieren dieses
   Schritts ist die Phasenintegration. Der Wrap beginnt anschließend wieder bei
   $0$, weil die nächste Wellenrunde gleich aussieht.

   **Wichtig**: Die Frequenz wird auf den Bereich $[-f_s/2, f_s/2]$ begrenzt,
   damit die Phasenintegration nicht über Nyquist hinausläuft (Aliasing).
   Negative Momentanfrequenzen bleiben erlaubt (Through-Zero-FM): Die Phase
   läuft dann rückwärts und der Wrap per `fmod` hält sie in $[0, 2\pi)$ —
   ohne Diskontinuitäten, auch bei Saw, Square und Triangle.

4. Wellenform: `evaluateWaveform()` normalisiert die Phase zunächst auf
   `[0, 2*pi)` und berechnet anschließend direkt (ohne Wavetable):

   | Form | Berechnung für die normalisierte Phase `p` |
   |---|---|
   | Sine | `sin(p)` |
   | Saw | `2 * (p / (2 * pi)) - 1` |
   | Square | `1`, wenn `p < pi`; sonst `-1` |
   | Triangle | `2 * (p / pi) - 1`, wenn `p < pi`; sonst `3 - 2 * (p / pi)` |

   Das Ergebnis wird als `out_n` ausgegeben.

Dieser Prozessor ist die Stelle, an der in der Frequenzdomänen-Architektur
die eigentliche FM entsteht: Er integriert die vom `note_in`-Port gelieferte
Momentanfrequenz in die Phase (phase accumulator) und rendert daraus die
Wellenform. Ein vorgeschalteter FM-Modulator verändert dabei pro Sample die
Frequenz, nicht die Phase. Der optionale `phaseModulation`-Parameter von
`getNextSample()` bleibt ungenutzt (`0.0f`) — er ist ein Relikt aus dem
früheren Phasenmodulations-Design.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $f$ | `freq` (lokal), `noteInput.getSample()` / `staticFreq->load()` / `lfoRate->load()` | [OscillatorProcessor.cpp](../../src/processors/OscillatorProcessor.cpp) | Pitch: Port-Wert falls `isConnected()`, sonst `0.0f`; Static: geclampt auf $[20, 20000]$; LFO: geclampt auf $[0.01, 50]$; danach `setFrequency()` auf $[-f_s/2, f_s/2]$ geclippt (Through-Zero erlaubt) |
| $f_s$ | `sampleRate` | [SimpleOscillator.h](../../src/SimpleOscillator.h) | gesetzt in `prepare(double)` |
| $\Delta\varphi$ | `phaseIncrement` | [SimpleOscillator.h](../../src/SimpleOscillator.h) | `updatePhaseIncrement()`: `twoPi * frequency / sampleRate` |
| $\varphi_n$ | `phase` | [SimpleOscillator.h](../../src/SimpleOscillator.h) | `getNextSample()`: `phase += phaseIncrement`, Wrap per `fmod()` mit Negativ-Korrektur |
| $m$ | `waveformFromIndex(round(waveform->load()))` | [OscillatorProcessor.cpp](../../src/processors/OscillatorProcessor.cpp) | APVTS-Float → `int` → `enum class Waveform` |
| $w(\cdot)$ | `evaluateWaveform(float)` | [SimpleOscillator.h](../../src/SimpleOscillator.h) | `switch` über `Waveform`; Phase vorher per `fmod` normalisiert (erlaubt Through-Zero) |
| $out_n$ | `getNextSample(0.0f)` → `output.setSample()` | [OscillatorProcessor.cpp](../../src/processors/OscillatorProcessor.cpp) | Sample erzeugen und in den Port schreiben |
