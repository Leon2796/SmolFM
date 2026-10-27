# OscillatorProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Oszillator-Knoten mit wählbarer Wellenform. Dient als Carrier (am Ende einer
FM-Kette, gespeist über `note_in`) oder als Modulator (speist `signal` in einen
FM-Modulator-Eingang). Rendert das Audiosignal und integriert dabei die
Momentanfrequenz in die Phase — das ist die Stelle, an der echte FM entsteht.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 16 | `GraphNodeRegistry::maxOscillators` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Bis zu 16 Wellen-Generatoren pro Voice — je Carrier-/Modulator-Rolle eine (Träger, Modulator-Quelle, LFO-Pendel). |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `note_in` | Input | `frequency` | Pitch-Modus: liest 0 Hz → der Oszillator schweigt; Static-/LFO-Modus: der Port wird ignoriert | **Stimm-Führer.** Die Momentanfrequenz in Hz, die pro Sample in die Phase integriert wird — das ist die Stelle, an der echte FM entsteht: jede Frequenzwanderung (FM-Stufe, F-ADSR, Key-Tracking) biegt die Welle direkt. Im Static-/LFO-Modus ignoriert der Oszillator den Draht und folgt seinem eigenen Hz-Regler. | `InputPort noteInput` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |
| `out` | Output | `signal` | — | **Gerenderte Wellenform.** Die erzeugte Welle (Sine bis Simplex Noise) — als Carrier der eigentliche Ton, als Quelle für FM-/AM-/Vel-Verbraucher das Spektrum-Material. | `OutputPort output` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| Wellenform | `osc<N>Waveform` | Choice: Sine/Saw/Square/Triangle/Noise/Perlin Noise/Simplex Noise | Die Rohfarbe der Quelle — die Gestalt der Welle: rein und klar (Sine), sägescharf obertonreich (Saw), hörnig-hohl (Square), weich gerundet (Triangle), unregelmäßig lebendig (Noise/Perlin/Simplex). | `std::atomic<float>* waveform` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |
| Modus | `osc<N>Mode` | Choice: Pitch/Static/LFO, Default Pitch | Wem die Frequenz gehört: der Note-/FM-Kette (Pitch — der übliche Carrier), dem eigenen Hz-Regler (Static — feststehende Tonquelle) oder dem langsamen Pendel (LFO — feste Modulationsquelle für Effekt-Ports, z. B. Tremolo/Wobble). | `std::atomic<float>* mode` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |
| Statische Frequenz | `osc<N>StaticFreq` | Float, 20–20000 Hz, Default 440 | Der feste Ton im Static-Modus (20–20000 Hz) — die Grundstimmung, wenn keine Note die Frequenz übernimmt. | `std::atomic<float>* staticFreq` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |
| LFO-Rate | `osc<N>LfoRate` | Float, 0.01–50 Hz, Default 1 | Das Pendeltempo im LFO-Modus (0.01–50 Hz) — wie schnell die Quelle auf- und abwippt, wenn sie als Modulator für Tremolo/Wobble dient. | `std::atomic<float>* lfoRate` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |
| Bypass | `osc<N>Bypass` | Bool, Default aus | Bypass-Hahn: 1 = der Oszillator schaltet ab — der Ausgang schreibt exakt 0 (Stille statt eines veralteten Frames) und die Phase parkt; Un-Bypass setzt die geparkte Phase fort. | `std::atomic<float>*` via `Processor::setBypassParam` | [src/processors/Processor.h](../../src/processors/Processor.h) |

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
| Bypass-Schalter | „Bypass": Ausgang = Stille (Quelle abgeschaltet), Phase parkt; Un-Bypass setzt die geparkte Phase fort | `DraggableComponent::bypassButton` — ⊘-Icon links oben in der Node-Titel-Leiste; engagiert = Node ausgegraut | [src/gui/DraggableComponent.cpp](../../src/gui/DraggableComponent.cpp) |
| *(Basis)* | ComboBox (Wellenform) mit 7 Optionen: Sine, Saw, Square, Triangle, Noise, Perlin Noise, Simplex Noise | `OscillatorPanel::waveformBox` | [src/gui/components/OscillatorPanel.h](../../src/gui/components/OscillatorPanel.h) |

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
   | Noise | `random(-1, 1)` — uniformes Zufallsrauschen, phase-unabhängig |
   | Perlin Noise | `perlin2D(x, 0)` mit $x = \frac{p}{2\pi} \cdot 32$; Oktaven-Summierung: $\sum_{i=0}^{3} \text{perlin}(x \cdot 2^i, 0) \cdot 0.5^i$ |
   | Simplex Noise | `simplex2D(x, 0)` mit $x = \frac{p}{2\pi} \cdot 32$ — klassisches 2D-Simplex-Noise |

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
| $m$ | `waveformFromIndex(round(waveform->load()))` | [OscillatorProcessor.cpp](../../src/processors/OscillatorProcessor.cpp) | APVTS-Float (0–6) → `int` → `enum class Waveform` |
| $w(\cdot)$ | `evaluateWaveform(float)` | [SimpleOscillator.h](../../src/SimpleOscillator.h) | `switch` über `Waveform`; Phase vorher per `fmod` normalisiert (erlaubt Through-Zero) |
| $out_n$ | `getNextSample(0.0f)` → `output.setSample()` | [OscillatorProcessor.cpp](../../src/processors/OscillatorProcessor.cpp) | Sample erzeugen und in den Port schreiben |
