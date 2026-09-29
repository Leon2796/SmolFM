# WaveshaperProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Verzerrt das eingehende Signal über eine von drei wählbaren Transfer­funktionen
(Soft / Hard / Fold). Ein Drive-Regler skaliert den Eingang vor der Kurve.
Stateless — kein Envelope, kein per-Note-State. `drive = 0` mit Shape „Soft"
ist nahezu transparent (Identität bei kleinen Amplituden).

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 16 | `GraphNodeRegistry::maxWaveshapers` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `in` (`PortType::signal`), `drive_mod_in` (`PortType::signal`) | `InputPort input`, `InputPort driveMod` | [src/processors/WaveshaperProcessor.h](../../src/processors/WaveshaperProcessor.h) |
| Output-Ports | `out` (`PortType::signal`) | `OutputPort output` | [src/processors/WaveshaperProcessor.h](../../src/processors/WaveshaperProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Drive | `shape<N>Drive` | Float, 0–1, Default 0 | `std::atomic<float>* drive` | [src/processors/WaveshaperProcessor.h](../../src/processors/WaveshaperProcessor.h) |
| Shape | `shape<N>Shape` | Choice 0–2 (Soft, Hard, Fold), Default 0 | `std::atomic<float>* shape` | [src/processors/WaveshaperProcessor.h](../../src/processors/WaveshaperProcessor.h) |

`<N>` = Instanzindex 0–7.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(keine)* | Drive-Rotary + Shape-Combo | `WaveshaperComponent::driveSlider`, `shapeBox` | [src/gui/components/WaveshaperComponent.h](../../src/gui/components/WaveshaperComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $x_n$ | Eingangssample | Amplitude |
| $d$ | Drive (0–1) | linear |
| $x'_n$ | angetriebener Eingang | Amplitude |
| $f$ | Transferfunktion (Soft/Hard/Fold) | — |
| $y_n$ | Ausgangssample | Amplitude |

Antrieb:

$$x'_n = x_n \cdot (1 + k \cdot d), \quad k = 9 \text{ (Soft/Hard)}, \; 19 \text{ (Fold)}$$

Transferfunktionen:

$$
f_{\text{soft}}(x') = \frac{x'}{1 + |x'|}
\qquad
f_{\text{hard}}(x') = \mathrm{clamp}(x', -1, 1)
$$

$$
f_{\text{fold}}(x') = \text{Foldback um } \pm 1 \text{ (solange } |y| > 1: y = 2\,\mathrm{sign}(y) - y)
$$

Pro Sample: $y_n = f(x'_n)$.

Spezialfälle:
- $d = 0$, Soft: $y \approx x$ bei kleinen Amplituden (nahezu transparent).
- Fold bei hohem Drive: Wellenfaltung erzeugt inharmonische Sidebands (metallisch).
- Hard bei hohem Drive: schnelle Harmonische-Zerfallsrate, digital-hart.

### Abschnitt 3b — Modulations-Input `drive_mod_in`

Optionaler Mod-Eingang für den **Drive** (PortType `signal`). Der
Stereo-Mittelwert wird auf $[-1,+1]$ geclampt und multipliziert den
statischen Drive: $d_{\text{eff}} = \mathrm{clamp}(d\cdot(1+m), 0, 1)$.
Unverbunden ($m = 0$) ist der Port vollständig transparent. Damit ist die
Transferkurve selbst enveloppierbar: statischer Osc → ADSR →
`drive_mod_in` = Klangfarben-Envelope (clean→growl), Perlin → driftende
Saturation, Square-LFO → Timbre-Stepper.
Port-ID-Sync: `GraphNodes`-Spec, XSD-Annotation (`shape`-complexType) und
`SynthVoice::resolveInput` tragen denselben Namen `drive_mod_in`.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $d$ | `drive->load()` → `dBase` → `d` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | Basis jlimit 0–1; `drive_mod_in` verbunden: $d = \mathrm{clamp}(d_{\text{base}}\cdot(1+m), 0, 1)$, $m$ = Stereo-Mittel geclampt $[-1,+1]$ |
| Shape-Auswahl | `shape->load()` → `shapeIndex` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | round + jlimit 0–2 |
| $x_n$ | `input.getSample()` → `x` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | Port lesen (Default 0.0f unverbunden) |
| $x'_n$ | `x * (1 + k*d)` → `driven` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | pro Shape-Case |
| $f_{\text{soft}}$ | `driven / (1 + abs(driven))` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | case 0 |
| $f_{\text{hard}}$ | `jlimit(-1, 1, driven)` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | case 1 |
| $f_{\text{fold}}$ | Foldback-While-Schleife | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | case 2 |
| $y_n$ | `output.setSample(y)` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | Ergebnis in den Port |