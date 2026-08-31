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
| Wellenform | `osc<N>Waveform` | Choice: Sine/Saw/Square/Triangle | `std::atomic<float>* waveform` | [src/processors/OscillatorProcessor.h](../../src/processors/OscillatorProcessor.h) |

`<N>` = Instanzindex 0–7. Die Frequenz kommt ausschließlich über `note_in`;
ohne Verbindung liefert der Oszillator 0 Hz (Stille).

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(noch keine)* | ComboBox (Wellenform) | `OscillatorPanel::waveformBox` | [src/gui/components/OscillatorPanel.h](../../src/gui/components/OscillatorPanel.h) |

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

1. Frequenzwahl: $f = f_{note\_in}$ falls verbunden, sonst $0$ (kein Ton);
   es gibt keinen Slider-Fallback.
2. Phase-Increment:
$$\Delta\varphi = \frac{2\pi f}{f_s}$$
3. Phasenintegration mit Wrap in $[0, 2\pi)$:
$$\varphi_{n+1} = (\varphi_n + \Delta\varphi) \bmod 2\pi$$

   Kurz erklärt: Die Phase ist der aktuelle Ort innerhalb einer Wellenrunde.
   Eine volle Runde hat $2\pi$ Radiant. Bei $f_s$ Samples pro Sekunde muss eine
   Frequenz von $f$ Hertz deshalb bei jedem Sample um $2\pi f / f_s$ weitergehen:
   Nach $f_s / f$ Samples ist genau eine Runde erreicht. Das Addieren dieses
   konstanten Schritts ist die Phasenintegration. Der Wrap beginnt anschließend
   wieder bei $0$, weil die nächste Wellenrunde gleich aussieht.

4. Wellenform: `evaluateWaveform()` normalisiert die Phase zunächst auf
   `[0, 2*pi)` und berechnet anschließend direkt (ohne Wavetable):

   | Form | Berechnung für die normalisierte Phase `p` |
   |---|---|
   | Sine | `sin(p)` |
   | Saw | `2 * (p / (2 * pi)) - 1` |
   | Square | `1`, wenn `p < pi`; sonst `-1` |
   | Triangle | `2 * (p / pi) - 1`, wenn `p < pi`; sonst `3 - 2 * (p / pi)` |

   Das Ergebnis wird als `out_n` ausgegeben.

Dieser Prozessor erzeugt aktuell keine FM: Er ruft
`getNextSample(0.0f)` auf und übergibt damit keinen Phasenversatz. Die hier
beschriebene Phasenintegration ist die normale Grundlage jedes digitalen
Oszillators. Erst wenn ein Modulatorsignal die Frequenz oder Phase pro Sample
verändert, entsteht FM beziehungsweise Phasenmodulation.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $f$ | `freq` (lokal), `noteInput.getSample()` | [OscillatorProcessor.cpp](../../src/processors/OscillatorProcessor.cpp) | Port-Wert falls `isConnected()`, sonst `0.0f` |
| $f_s$ | `sampleRate` | [SimpleOscillator.h](../../src/SimpleOscillator.h) | gesetzt in `prepare(double)` |
| $\Delta\varphi$ | `phaseIncrement` | [SimpleOscillator.h](../../src/SimpleOscillator.h) | `updatePhaseIncrement()`: `twoPi * frequency / sampleRate` |
| $\varphi_n$ | `phase` | [SimpleOscillator.h](../../src/SimpleOscillator.h) | `getNextSample()`: `phase += phaseIncrement`, Wrap per `while` |
| $m$ | `waveformFromIndex(round(waveform->load()))` | [OscillatorProcessor.cpp](../../src/processors/OscillatorProcessor.cpp) | APVTS-Float → `int` → `enum class Waveform` |
| $w(\cdot)$ | `evaluateWaveform(float)` | [SimpleOscillator.h](../../src/SimpleOscillator.h) | `switch` über `Waveform`; Phase vorher per `fmod` normalisiert (erlaubt Through-Zero) |
| $out_n$ | `getNextSample(0.0f)` → `output.setSample()` | [OscillatorProcessor.cpp](../../src/processors/OscillatorProcessor.cpp) | Sample erzeugen und in den Port schreiben |
