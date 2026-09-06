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
| Input-Ports | `in` (`PortType::signal`) | `InputPort input` | [src/processors/GainProcessor.h](../../src/processors/GainProcessor.h) |
| Output-Ports | `out` (`PortType::signal`) | `OutputPort output` | [src/processors/GainProcessor.h](../../src/processors/GainProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Gain | `gain<N>Factor` | Float, 0–10, Default 1 | `std::atomic<float>* gain` | [src/processors/GainProcessor.h](../../src/processors/GainProcessor.h) |

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
| $y_n$ | Ausgangssample | Amplitude |

Pro Sample:

$$y_n = G \cdot x_n$$

Spezialfälle:
- $G = 1$: transparent.
- $G = 0$: Ausgang stumm (Layer-Mute).
- $G < 0$ (API-only): Phasen-Inversion.
- $G > 1$: Boost; Vorsicht vor Clipping am Master-Output (8 gestackte Layer).

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $G$ | `gain->load()` → `g` | [GainProcessor.cpp](../../src/processors/GainProcessor.cpp) | in `processSample()` gelesen |
| $x_n$ | `input.getSample()` → `sourceSample` | [GainProcessor.cpp](../../src/processors/GainProcessor.cpp) | Port lesen (Default 0.0f unverbunden) |
| $y_n$ | `sourceSample * g` → `output.setSample()` | [GainProcessor.cpp](../../src/processors/GainProcessor.cpp) | Multiplikation, Ergebnis in den Port |