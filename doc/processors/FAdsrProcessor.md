# FAdsrProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

F-ADSR: Pitch-Hüllkurve im Frequenzbereich. Multipliziert die eingehende
Frequenz mit einem Skalierungsfaktor, der zwischen den Parametern Down-Factor
(bei Envelope-Wert E = 0) und Up-Factor (bei E = 1) interpoliert. Kann als
klassische Pitch-Envelope vor dem Carrier-Oszillator, in FM-Ketten oder als
perkussives Pitch-Drop-Element dienen. Wrappt `juce::ADSR`; Parameter werden
bei Note-On gelesen (JUCE-Empfehlung) und wirken daher auf die nächste Note.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 8 | `GraphNodeRegistry::maxFAdsr` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `freq_in` (`PortType::frequency`), `vel` (`PortType::velocity`) | `InputPort freqInput`, `InputPort velIn` | [src/processors/FAdsrProcessor.h](../../src/processors/FAdsrProcessor.h) |
| Output-Ports | `out` (`PortType::frequency`) | `OutputPort output` | [src/processors/FAdsrProcessor.h](../../src/processors/FAdsrProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Attack | `fadsr<N>Attack` | Float, 0.001–5 s | `std::atomic<float>* attack` | [src/processors/FAdsrProcessor.h](../../src/processors/FAdsrProcessor.h) |
| Decay | `fadsr<N>Decay` | Float, 0.001–5 s | `std::atomic<float>* decay` | [src/processors/FAdsrProcessor.h](../../src/processors/FAdsrProcessor.h) |
| Sustain | `fadsr<N>Sustain` | Float, 0–1 | `std::atomic<float>* sustain` | [src/processors/FAdsrProcessor.h](../../src/processors/FAdsrProcessor.h) |
| Release | `fadsr<N>Release` | Float, 0.001–10 s | `std::atomic<float>* release` | [src/processors/FAdsrProcessor.h](../../src/processors/FAdsrProcessor.h) |
| Up Factor | `fadsr<N>Up` | Float, 0–10, Default 1 | `std::atomic<float>* up` | [src/processors/FAdsrProcessor.h](../../src/processors/FAdsrProcessor.h) |
| Down Factor | `fadsr<N>Down` | Float, 0–10, Default 1 | `std::atomic<float>* down` | [src/processors/FAdsrProcessor.h](../../src/processors/FAdsrProcessor.h) |

`<N>` = Instanzindex 0–3.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| Pitch-Envelope-Vorschau | Zeichnet die ADSR-Kurve aus den vier Timing-Slidern (linearer Attack, exponentieller Decay/Release via `quadraticTo`); Repaint bei Slider-Drag | `FAdsrComponent::CurveDisplay::paint` | [src/gui/components/FAdsrComponent.cpp](../../src/gui/components/FAdsrComponent.cpp) |
| *(Rest wie gehabt)* | sechs Rotary-Slider (A/D/S/R + Up/Down) | `FAdsrComponent::attackSlider` u. a. | [src/gui/components/FAdsrComponent.h](../../src/gui/components/FAdsrComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $f_n$ | Eingangsfrequenz am Port `freq_in` | Hz |
| $E_n$ | Hüllkurvenwert (0–1) | linear |
| $F_{up}$ | Skalierungsfaktor bei $E = 1$ | linear |
| $F_{down}$ | Skalierungsfaktor bei $E = 0$ | linear |
| $A, D, R$ | Attack-, Decay-, Release-Zeit | s |
| $S$ | Sustain-Pegel | 0–1 |
| $f'_n$ | Ausgangsfrequenz | Hz |

Skalierungsfaktor (lineare Interpolation zwischen den Extremen):

$$F(E_n) = F_{down} + (F_{up} - F_{down}) \cdot E_n$$

Pro Sample:

$$f'_n = f_n \cdot F(E_n)$$

Spezialfälle:
- $F_{up} = F_{down} = 1$: transparent (Hüllkurve hat keinen Einfluss).
- $F_{down} = 0$: Ton startet bei 0 Hz (Oszillator bleibt still) und gleitet beim Attack auf die Spielpitch hoch.
- $F_{up} > 1$: Ton schwingt bei Envelope-Peak über die Spielpitch (Pitch-Bling), $F_{up} < 1$ darunter.
- Sustain-Level steuert, welcher Faktor während der Haltephase dominiert.

### Velocity-Port `vel` — Accent-Tiefe (Pull-to-Neutral)

Die verbundene `note.vel`-Quelle (Stereo-Mittel $v \in [0,1]$) skaliert die
**Ankertiefe**, nicht die Kurvenform: beide Anker gleiten auf eigener Schiene
von Neutral 1.0 zum konfigurierten Wert.

$$F_{up,v} = 1 + v \cdot (F_{up} - 1), \qquad F_{down,v} = 1 + v \cdot (F_{down} - 1)$$

- $v = 0$: flache Hüllkurve (Faktor 1 über alle $E$) — byteidentisch zum unverdrahteten Verhalten.
- $v = 1$: exakt die konfigurierten Anker ($F_{up}$, $F_{down}$).
- Der Ankerabstand skaliert linear mit ($\text{gap} = v \cdot (F_{up} - F_{down})$); A/D/S/R-Zeiten und die Anker-Richtung (auch invertiert, $F_{down} > F_{up}$) bleiben unangetastet.
- Unverbunden: Default 1.0 (neutral) — voll klanginvariant wie die Receive-Pins an `gain`, `shape`, `fm`, `filter`. Der Unterschied: dort multipliziert Velocity den Wert (Level-Duck), hier skaliert sie die Tiefe (Abweichung von Neutral).

Funktionsablauf: Bei Note-On wird die Hüllkurve mit $(A, D, S, R)$ parametrisiert
und gestartet (`noteOn`), bei Note-Off läuft die Release-Phase (`noteOff`).
`juce::ADSR` liefert pro Sample den Wert $E_n$ entlang der linearen Attack- und
exponentiellen Decay/Release-Segmente.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $A, D, S, R$ | `attack->load()` … `release->load()` → `juce::ADSR::Parameters` | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | in `startNote()` gelesen, `envelope.setParameters(params)` |
| $E_n$ | `envelope.getNextSample()` → `env` | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | JUCE-ADSR-State-Machine; Sample-Rate via `envelope.setSampleRate()` in `prepare()` |
| $F_{up}$ | `up->load()` → `upValue` | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | Faktor bei E = 1 |
| $F_{down}$ | `down->load()` → `downValue` | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | Faktor bei E = 0 |
| $F(E_n)$ | `downValue + (upValue - downValue) * env` → `scaled`-Berechnung | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | lineare Interpolation in `processSample()` |
| $v$ | `velIn.getSample()` → Stereo-Mittel | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | `vel` verbunden: Ankertiefe ($v \in [0,1]$); unverbunden: Default 1.0 (neutral) |
| $F_{up,v}$, $F_{down,v}$ | `upEffective`, `downEffective` | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | Pull-to-Neutral $1 + v \cdot (F - 1)$, nur bei verbundener `vel` |
| $f_n$ | `freqInput.getSample()` → `sourceFrequency` | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | Port lesen (Default 440 Hz unverbunden) |
| $f'_n$ | `sourceFrequency * F(E)` → `output.setSample()` | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | Ergebnis in den Port |
| Note-Off | `noteOff()` → `envelope.noteOff()` | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | aus `SynthVoice::stopNote` bei `allowTailOff` |
| Still-Reset | `reset()` → `envelope.reset()` | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | aus `SynthVoice::applyConnectionPatch` — ein neues Patch erbt keinen hängenden Sustain |
| Aktiv-Flag | `isActive()` → `envelope.isActive()` | [FAdsrProcessor.cpp](../../src/processors/FAdsrProcessor.cpp) | Voice wird freigegeben, wenn Hüllkurve endet |