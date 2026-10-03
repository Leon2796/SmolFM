# AmProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Klassische Amplitudenmodulation mit regelbarer Tiefe. Ringmodulation ist der
Grenzfall `amount = 1` ohne Bias; der AmProcessor verschiebt den bipolaren
Modulator hingegen in einen positiven Gain-Bereich, sodass das Trägersignal
nie ganz ausgelöscht wird.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 8 | `GraphNodeRegistry::maxAmModulators` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Acht „Lautstärke-Pumpen" pro Voice — je Layer eine, um Träger klanggestaltend zu wobbeln. |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `carrier_in` | Input | `signal` | liest 0 → Stille (ohne Träger kann nichts schwingen) | **Träger.** Das Signal, das am Ende hörbar bleiben soll — der AmProcessor verformt es nicht, es wird nur im Rhythmus des Modulators laut und leise gewogen (wie eine Stimme hinter einem öffnenden und schließenden Fenster). | `InputPort carrierIn` | [src/processors/AmProcessor.h](../../src/processors/AmProcessor.h) |
| `modulator_in` | Input | `signal` | liest 0 → sauberer Durchlass ($g = 1$, byteidentisch zum Bypass) | **Pump-Signal.** Seine Schwingung treibt die Lautstärke hoch und runter: langsam und sanft = Tremolo-Zittern, kräftig und schnell = Chopper, der den Träger periodisch ausblendet. Der bipolare Modulator wird vor der Multiplikation in einen positiven Gain-Bereich geschoben, damit der Träger nie ganz verschwindet. | `InputPort modulatorIn` | [src/processors/AmProcessor.h](../../src/processors/AmProcessor.h) |
| `out` | Output | `signal` | — | **Gepumptes Signal:** Träger × zeitlich wandernder Gain-Faktor — klingt wie der Träger, aber im Takt des Modulators atmend. | `OutputPort output` | [src/processors/AmProcessor.h](../../src/processors/AmProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| Depth | `am<N>Amount` | Float, 0.0–1.0 (Default 0.0) | Wie tief die Lautstärke-Schwankung greift: 0 = trocken bzw. neutral (reiner Durchlass), 1 = der Modulator diktiert die Lautheit komplett (bis zum periodischen Auslöschen — der Grenzfall zur Ringmodulation). | `std::atomic<float>* amount` | [src/processors/AmProcessor.h](../../src/processors/AmProcessor.h) |

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
