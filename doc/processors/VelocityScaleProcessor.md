# VelocityScaleProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Skaliert die Velocity-Domäne vor den vel-Verbrauchern: eine Kurve
(linear / sqrt / square) mal ein Faktor. Die Analog-Brücke zwischen rohem
MIDI-Anschlag und den Akzentgesetzen hardware-simulierter Instrumente (der
TR-808-Anschlag ist eine Trigger-Spannung mit eigenem Gesetz, nicht die rohe
MIDI-Skala). Chainable und stateless — das Velocity-Domänen-Pendant zum
FrequencyScaleProcessor.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 8 | `GraphNodeRegistry::maxVelocityScales` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Acht Anschlags-Former pro Voice — je Anschlags-Schiene (z. B. sanft gegen hart) eine. |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `vel_in` | Input | `velocity` | liest 1.0 (die Velocity-Neutral) → der Knoten gibt 1.0 weiter (inert, nicht stumm) | **Roh-Anschlag am Eingang.** Die Anschlagstärke der aktuellen Note (0–1), die hier ihre Bahnkurve bekommt — wie ein Ball, der erst über die Rampe geformt wird. | `InputPort velInput` | [src/processors/VelocityScaleProcessor.h](../../src/processors/VelocityScaleProcessor.h) |
| `out` | Output | `velocity` | — | **Geformte Anschlagsspannung:** die gekrümmte und skalierte Velocity (je Kanal auf 0–1 geclampt) — fließt in die `vel`-Ports der nachgeschalteten Klangverbraucher. | `OutputPort output` | [src/processors/VelocityScaleProcessor.h](../../src/processors/VelocityScaleProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| Curve | `vsc<N>Curve` | Choice 0–2 (Linear, Sqrt, Square), Default 0 | Die Anschlags-Dynamikform: Linear = roh und transparent; Sqrt = weiche Anschläge werden spürbar lauter (der Wahrnehmung gerecht); Square = zarte Anschläge fast verschwinden, harte behalten volle Dynamik (Akzent-Reserve). | `std::atomic<float>* curve` | [src/processors/VelocityScaleProcessor.h](../../src/processors/VelocityScaleProcessor.h) |
| Scale | `vsc<N>Scale` | Float, 0.25–4.0, Default 1 | Der globale Anschlags-Faktor: hebt die ganze Anschlagsleiter an oder senkt sie — wie ein Drehregler am Anschlag-Pedal. | `std::atomic<float>* scale` | [src/processors/VelocityScaleProcessor.h](../../src/processors/VelocityScaleProcessor.h) |

`<N>` = Instanzindex 0–7.

`vel_in` unverbunden: die Velocity-Domänen-Default (1.0) — der Knoten ist
inert (der Ausgang = 1.0 bei `scale = 1`), weil Velocity als Pegel- und
Klangmodifikator fließt und nicht als Audio.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(keine)* | Curve-ComboBox + Scale-Rotary | `VelocityScaleComponent::curveBox`, `scaleSlider` | [src/gui/components/VelocityScaleComponent.h](../../src/gui/components/VelocityScaleComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $v_n$ | Eingangsvelocity (Stereo-Paar) | 0–1 |
| $c$ | Curve-Index (0–2) | — |
| $s$ | Skalierung (0.25–4) | linear |
| $y_n$ | Ausgangsvelocity | 0–1 |

$$y_n = \mathrm{jlimit}\,(0, 1,\ \mathrm{curve}_c(v_n)\cdot s)$$

Kurven:
- $c = 0$ (linear): $y = v$ — rohe MIDI-Anschläge, transparent.
- $c = 1$ (sqrt): $y = \sqrt{v}$ — weiche Anschläge werden lauter
  (die wahrgenommene Perkussions-Lautheit wächst etwa quadratisch mit der
  Amplitude; die Wurzel gleicht das aus).
- $c = 2$ (square): $y = v^2$ — weiche Anschläge verschwinden, harte behalten
  die volle Dynamik (Akzent-Reserve).

Spezialfälle:
- `curve = linear` + `scale = 1`: identischer Durchlass ($v_{out} = v_{in}$).
- Unverbunden: `vel_in` liest 1.0 (die Velocity-Neutral), der Ausgang = 1.0
  bei `scale = 1` — der Knoten ist inert, nicht stumm.
- Der Ausgang ist je Kanal auf 0–1 geclampt: Velocity bleibt Velocity.
- Port-ID-Sync: `GraphNodes`-Spec, XSD-Annotation (`VelocityScaleNodeType`)
  und `SynthVoice::resolveInput` tragen denselben Namen `vel_in`; das
  Curve-APVTS-ID-Muster entsteht per Ersetzen-Trick aus `vsc%Scale`.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $v_n$ | `velInput.getSample()` | [VelocityScaleProcessor.cpp](../../src/processors/VelocityScaleProcessor.cpp) | Port lesen (Default 1.0f unverbunden) |
| $c$ | `curve->load()` → `curveIndex` | [VelocityScaleProcessor.cpp](../../src/processors/VelocityScaleProcessor.cpp) | round, via `vscCurveFromIndex` geclampt |
| $\mathrm{curve}_c(v)$ | `vscCurveFromIndex (curveIndex, v)` | [VelocityScaleProcessor.h](../../src/processors/VelocityScaleProcessor.h) | linear/sqrt/square per switch; $v \le 0$ → 0 |
| $s$ | `scale->load()` → `scaleValue` | [VelocityScaleProcessor.cpp](../../src/processors/VelocityScaleProcessor.cpp) | jlimit 0.25–4 |
| $y_n$ | `output.setSample(result)` | [VelocityScaleProcessor.cpp](../../src/processors/VelocityScaleProcessor.cpp) | je Kanal geclampt 0–1 |