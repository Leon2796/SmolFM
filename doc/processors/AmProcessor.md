# AmProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Klassische Amplitudenmodulation mit regelbarer Tiefe. Ringmodulation ist der
Grenzfall `amount = 1` ohne Bias; der AmProcessor verschiebt den bipolaren
Modulator hingegen in einen positiven Gain-Bereich, sodass das Trägersignal
nie ganz ausgelöscht wird.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 4 | `GraphNodeRegistry::maxAmModulators` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `carrier_in`, `modulator_in` (`PortType::signal`) | `InputPort carrierIn`, `modulatorIn` | [src/processors/AmProcessor.h](../../src/processors/AmProcessor.h) |
| Output-Ports | `out` (`PortType::signal`) | `OutputPort output` | [src/processors/AmProcessor.h](../../src/processors/AmProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Depth | `am<N>Amount` | Float, 0.0 – 1.0 (Default 0.0) | `std::atomic<float>* amount` | [src/processors/AmProcessor.h](../../src/processors/AmProcessor.h) |

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(noch keine)* | Rotary-Slider `Depth` + Pin-Labels `carrier × mod` | `AmComponent::amountSlider` | [src/gui/components/AmComponent.h](../../src/gui/components/AmComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | Sample-Index | — |
| $c_n$ | Trägersignal an `carrier_in` im Sample $n$ | linear |
| $m_n$ | Modulatorsignal an `modulator_in` im Sample $n$, bipolär in $[-1, 1]$ | linear |
| $d$ | Depth (APVTS `am<N>Amount`), $0 \le d \le 1$ | linear |
| $g_n$ | resultierender Gain-Faktor im Sample $n$ | linear |
| $y_n$ | Ausgangssignal im Sample $n$ | linear |

Pro Sample wird der bipolare Modulator in einen positiven Gain gemappt und
mit dem Träger multipliziert:

$$
g_n = (1 - d) + d \bigl( m_n \cdot 0.5 + 0.5 \bigr) \cdot 2 = 1 - d + d\,(m_n + 1)
$$

$$
y_n = c_n \cdot g_n = c_n \bigl[ 1 - d + d\,(m_n + 1) \bigr]
$$

Grenzfälle:

- **$d = 0$:** $g_n = 1$ → $y_n = c_n$ (trockenes Durchreichen).
- **$d = 1$:** $g_n = m_n + 1$ → der Gain pendelt zwischen $0$ (bei $m_n = -1$)
  und $2$ (bei $m_n = +1$). Klingt wie Ringmodulation, solange der Modulator
  die $-1$ nicht erreicht.
- **Unwired carrier:** $c_n = 0$ → Stille.
- **Unwired modulator:** $m_n = 0$ → $g_n = 1 - d + d = 1$ → $y_n = c_n$
  (sauberer Bypass).

Für eine langsame Modulator-Schwingung ($f_m \ll f_c$) entsteht Tremolo bei
niedriger Depth; bei hoher Depth und $m_n = -1$-Phasen wird der Carrier
periodisch ausgeblendet (Chopper-Effekt).

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $d$ | `amount->load()` | [AmProcessor.cpp](../../src/processors/AmProcessor.cpp) | APVTS-Atomic lesen; `nullptr` → 0 |
| $m$ | `modulatorIn.getSample()` | [AmProcessor.cpp](../../src/processors/AmProcessor.cpp) | Port lesen; Default 0 wenn unwired |
| $c$ | `carrierIn.getSample()` | [AmProcessor.cpp](../../src/processors/AmProcessor.cpp) | Port lesen; Default 0 wenn unwired |
| $g$ | `gain` | [AmProcessor.cpp](../../src/processors/AmProcessor.cpp) | `1.0f - depth + depth * (mod * 0.5f + 0.5f) * 2.0f` |
| $y$ | `result` → `output.setSample(result)` | [AmProcessor.cpp](../../src/processors/AmProcessor.cpp) | `carrier * gain`; eine Multiplikation pro Sample |
