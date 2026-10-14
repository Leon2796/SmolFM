# WaveshaperProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Verzerrt das eingehende Signal über eine von vier wählbaren Transfer­funktionen
(Soft / Hard / Fold / Quantize). Ein Drive-Regler skaliert den Eingang vor der
Kurve; bei Quantize setzt er die Stufengitter-Größe (64 bis 2 Stufen pro Einheit).
Stateless — kein Envelope, kein per-Note-State. `drive = 0` mit Shape „Soft"
ist nahezu transparent (Identität bei kleinen Amplituden); Quantize ebenfalls
(maximaler Stufenfehler 1/128 bei `drive = 0`).

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 16 | `GraphNodeRegistry::maxWaveshapers` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Bis zu 16 Kurven-Zauberkästen pro Voice — je Klangverzerrungs-Schritt einer. |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `in` | Input | `signal` | liest 0 → stumm | **Rohsignal.** Das Signal, dessen Wellenberge durch die gewählte Kurve neu „gezeichnet" werden — wie ein Stift, der Berge zurecht staucht; das Rohsignal selbst kommt nicht wieder heraus, nur seine Formverzerrung. | `InputPort input` | [src/processors/WaveshaperProcessor.h](../../src/processors/WaveshaperProcessor.h) |
| `drive_mod_in` | Input | `signal` | liest 0 → vollständig transparent (neutral) | **Drive-Fremdfahrer.** Multipliziert den eingestellten Drive ($\cdot\,(1+m)$, auf 0–1 geclampt) — die Kurvenform wird selbst enveloppierbar: ADSR = Klangfarben-Envelope (clean→growl), Perlin = driftende Saturation, Square = Timbre-Stepper. | `InputPort driveMod` | [src/processors/WaveshaperProcessor.h](../../src/processors/WaveshaperProcessor.h) |
| `vel` | Input | `velocity` | liest 1.0 → neutral, klanginvariant | **Anschlags-Drive.** Anschlagstärke multipliziert den wirksamen Drive ($d\cdot v$): harte Anschläge drücken tiefer in die Kurve — die timbrale Hälfte des TR-808-Accents. | `InputPort velIn` | [src/processors/WaveshaperProcessor.h](../../src/processors/WaveshaperProcessor.h) |
| `out` | Output | `signal` | — | **Verzerrtes Signal:** die durch die Transferkurve geformte Welle — von sanfter Färbung bis metallischer Faltung. | `OutputPort output` | [src/processors/WaveshaperProcessor.h](../../src/processors/WaveshaperProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| Drive | `shape<N>Drive` | Float, 0–1, Default 0 | Wie kräftig das Signal in die Kurve hineingedrückt wird — das Antriebsrad vor der Zurechtstauchung: nahe 0 mit „Soft" fast neutral, voll aufgedreht kantig. | `std::atomic<float>* drive` | [src/processors/WaveshaperProcessor.h](../../src/processors/WaveshaperProcessor.h) |
| Shape | `shape<N>Shape` | Choice 0–3 (Soft, Hard, Fold, Quantize), Default 0 | Der Verzerrungs-Charakter: Soft = sanfte tubige Weichzeichnung, Hard = harte Clipschere (digital-randige Kanten), Fold = Wellenfaltung, die inharmonisch-metallisch klingt, Quantize = Amplituden-Treppstufen (Bitcrusher-Wertachse): das Sample wird aufs Stufengitter gerastet, Drive macht das Gitter gröber (64→2 Stufen pro Einheit). | `std::atomic<float>* shape` | [src/processors/WaveshaperProcessor.h](../../src/processors/WaveshaperProcessor.h) |

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
| $f$ | Transferfunktion (Soft/Hard/Fold/Quantize) | — |
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

$$
f_{\text{quantize}}(x_n) = \frac{\mathrm{round}(x_n \cdot L)}{L}, \quad
L = \mathrm{clamp}\big(\mathrm{round}(64 \cdot (1 - d)),\, 2,\, 64\big)
$$

($L$ = Stufen pro Einheit; Quantize treibt den Eingang nicht — $f$ liest dort
$x_n$ statt $x'_n$, der Drive steckt im Gitterabstand.)

Pro Sample: $y_n = f(x'_n)$ — bei Quantize $y_n = f_{\text{quantize}}(x_n)$.

Spezialfälle:
- $d = 0$, Soft: $y \approx x$ bei kleinen Amplituden (nahezu transparent).
- Fold bei hohem Drive: Wellenfaltung erzeugt inharmonische Sidebands (metallisch).
- Hard bei hohem Drive: schnelle Harmonische-Zerfallsrate, digital-hart.
- Quantize: $d = 0 \Rightarrow L = 64$ (Stufenfehler $\le 1/128$, nahezu
  transparent); $d = 1 \Rightarrow L = 2$ (Signum-Treppen, maximaler Crush).
  Ein Sinus springt treppenförmig zwischen den Stufenniveaus — ungerade
  Oberwellen mit festem Stufenmuster: die Wertachse des Bitcrush (die
  Zeitachse kommt vom `hold`-Node, siehe `doc/techniques/bitcrush.yaml`).

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

### Abschnitt 3c — Velocity-Input `vel`

Optionaler Velocity-Eingang (PortType `velocity`, 0–1, Anschlagstärke der
aktuellen Note). Der Stereo-Mittelwert multipliziert den wirksamen Drive:
$d_{\text{eff}} = \mathrm{clamp}(d \cdot v, 0, 1)$. Unverbunden (Default
1.0) ist der Port vollständig sound-invariant. Rolle im 808-Schema:
Accent-Drive — harte Anschläge drücken tiefer in die Kurve (die timbrale
Hälfte des TR-808-Accents).
Port-ID-Sync: `GraphNodes`-Spec, XSD-Annotation (`shape`-complexType) und
`SynthVoice::resolveInput` tragen denselben Namen `vel`.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $d$ | `drive->load()` → `dBase` → `d` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | Basis jlimit 0–1; `drive_mod_in` verbunden: $d = \mathrm{clamp}(d_{\text{base}}\cdot(1+m), 0, 1)$, $m$ = Stereo-Mittel geclampt $[-1,+1]$ |
| Shape-Auswahl | `shape->load()` → `shapeIndex` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | round + jlimit 0–3 |
| $x_n$ | `input.getSample()` → `x` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | Port lesen (Default 0.0f unverbunden) |
| $x'_n$ | `x * (1 + k*d)` → `driven` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | pro Shape-Case |
| $f_{\text{soft}}$ | `driven / (1 + abs(driven))` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | case 0 |
| $f_{\text{hard}}$ | `jlimit(-1, 1, driven)` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | case 1 |
| $f_{\text{fold}}$ | Foldback-While-Schleife | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | case 2 |
| $f_{\text{quantize}}$ | `round(x * levels) / levels` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | case 3; `levels` = jlimit(2, 64, round(64·(1−d))) |
| $y_n$ | `output.setSample(y)` | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | Ergebnis in den Port |
| $v$ | `velIn.getSample()` → Stereo-Mittel | [WaveshaperProcessor.cpp](../../src/processors/WaveshaperProcessor.cpp) | `vel` verbunden: $d_{\text{eff}} = \mathrm{clamp}(d \cdot v, 0, 1)$; unverbunden: Default 1.0 (neutral) |