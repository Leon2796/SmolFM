# GainProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Konstante Verstärkung/Dämpfung: multipliziert das eingehende Signal mit dem
Gain-Faktor und leitet das Ergebnis weiter. Stateless — kein Envelope, kein
per-Note-State. `gain = 1` ist transparent, ein unverbundener Node liefert
Stillstand (Default-Eingang 0.0).

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 8 | `GraphNodeRegistry::maxGains` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `in` (`PortType::signal`), `factor_mod_in` (`PortType::signal`) | `InputPort input`, `InputPort modInput` | [src/processors/GainProcessor.h](../../src/processors/GainProcessor.h) |
| Output-Ports | `out` (`PortType::signal`) | `OutputPort output` | [src/processors/GainProcessor.h](../../src/processors/GainProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Gain | `gain<N>Factor` | Float, 0–10, Default 1 (UI-Bipolar: Mitte ≙ 1.0/Unity — linke Hälfte linear 0–1, rechte Hälfte quadratisch 1–10; Rohwert bleibt 0–10) | `std::atomic<float>* gain` | [src/processors/GainProcessor.h](../../src/processors/GainProcessor.h) |

`<N>` = Instanzindex 0–7.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(keine)* | ein Rotary-Slider (Gain-Faktor, Suffix „x") | `GainComponent::gainSlider` | [src/gui/components/GainComponent.h](../../src/gui/components/GainComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $x_n$ | Eingangssample | Amplitude |
| $G$ | Gain-Faktor | linear |
| $G_{\text{eff}}$ | wirksamer Gain-Faktor | linear |
| $y_n$ | Ausgangssample | Amplitude |

Pro Sample:

$$y_n = G_{\text{eff}} \cdot x_n$$

Spezialfälle:
- $G = 1$: transparent.
- $G = 0$: Ausgang stumm (Layer-Mute).
- $G < 0$ (API-only): Phasen-Inversion.
- $G > 1$: Boost; Vorsicht vor Clipping am Master-Output (8 gestackte Layer).

### Abschnitt 3b — Modulations-Input `factor_mod_in`

Optionaler Mod-Eingang für den **Gain-Faktor** (PortType `signal`). Der
Stereo-Mittelwert wird auf $[-1,+1]$ geclampt und multipliziert den
statischen Faktor: $G_{\text{eff}} = \mathrm{clamp}(G\cdot(1+m), 0, 10)$.
Unverbunden ($m = 0$) ist der Port vollständig transparent. Damit wird das
Gain zum modulierbaren VCA: LFO → lineares Tremolo/Swell (ohne die AM-
Ring-Semantik), Square-LFO → rhythmisches Gate, statischer Osc → ADSR →
eigene Volume-Envelope pro Layer.
Port-ID-Sync: `GraphNodes`-Spec, XSD-Annotation (`gain`-complexType) und
`SynthVoice::resolveInput` tragen denselben Namen `factor_mod_in`.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $G$ | `gain->load()` → `gBase` → `g` | [GainProcessor.cpp](../../src/processors/GainProcessor.cpp) | Basis gelesen; `factor_mod_in` verbunden: $G_{\text{eff}} = \mathrm{clamp}(G\cdot(1+m), 0, 10)$, $m$ = Stereo-Mittel geclampt $[-1,+1]$ |
| $x_n$ | `input.getSample()` → `sourceSample` | [GainProcessor.cpp](../../src/processors/GainProcessor.cpp) | Port lesen (Default 0.0f unverbunden) |
| $y_n$ | `sourceSample * g` → `output.setSample()` | [GainProcessor.cpp](../../src/processors/GainProcessor.cpp) | Multiplikation, Ergebnis in den Port |