# RingModulatorProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Klassische Ringmodulation als sampleweise Multiplikation zweier Signale.
Zustandslos, parameterlos; ein unwired Eingang liest als 0 und macht den
Ausgang stumm (analog zu einem Dioden-Ringmodulator).

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 8 | `GraphNodeRegistry::maxRingModulators` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Acht Multiplikator-Paare pro Voice — je Glocken-/Metall-Farbprojekt eines. |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `in1` | Input | `signal` | liest 0 → Ausgang stumm (analog zum Dioden-Ringmodulator: ohne zweite Welle schwingt nichts) | **Faktor 1.** Eine der beiden Wellen — typisch das Träger-/Klangmaterial (Oscillatorschwarm, Resonator-Ring). | `InputPort input1` | [src/processors/RingModulatorProcessor.h](../../src/processors/RingModulatorProcessor.h) |
| `in2` | Input | `signal` | liest 0 → Ausgang stumm, bis beide Seiten gepatcht sind | **Faktor 2.** Das Gegenstück: langsam schwingend (1–10 Hz) macht es Tremolo-Schwebung, schnell und klangreich eine metallische Glockenstimme. | `InputPort input2` | [src/processors/RingModulatorProcessor.h](../../src/processors/RingModulatorProcessor.h) |
| `out` | Output | `signal` | — | **Das Produkt.** Beide Wellen multiplizieren sich sampleweise und verschwinden beide selbst — zurück bleiben neue Frequenzen (Summen- & Differenzfrequenzen, grundsätzlich ohne Grundton), wie zwei Wellenberge, die ineinander durchlaufen und beim Durchgang neue Berge wachsen lassen. | `OutputPort output` | [src/processors/RingModulatorProcessor.h](../../src/processors/RingModulatorProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| *(keine)* | — | — | Keine Drehregler — die Wirkung ist reine Multiplikation; Klangfarbe kommt komplett aus der Verdrahtung der beiden Faktoren. | — | — |

Der Ringmodulator besitzt **keine eigenen APVTS-Parameter**. Die beiden
Eingangssignale kommen ausschließlich über die Ports; das Ausgangssignal ist
ihr Produkt.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(noch keine)* | statisches Label `in1 × in2` | `RingModulatorComponent::infoLabel` | [src/gui/components/RingModulatorComponent.h](../../src/gui/components/RingModulatorComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | Index des gerade berechneten Samples | — |
| $x_{1,n}$ | Signal am Eingang `in1` im Sample $n$ | linear |
| $x_{2,n}$ | Signal am Eingang `in2` im Sample $n$ | linear |
| $y_n$ | Ausgangssignal im Sample $n$ | linear |

Pro Sample gilt:

$$
y_n = x_{1,n} \cdot x_{2,n}
$$

Mit zwei Sinustönen $x_1 = \sin(2\pi f_1 t)$ und $x_2 = \sin(2\pi f_2 t)$
entstehen nach dem trigonometrischen Produkt-Satz

$$
\sin(\alpha)\sin(\beta) = \tfrac{1}{2}\big[\cos(\alpha-\beta) - \cos(\alpha+\beta)\big]
$$

die Summen- und Differenzfrequenz $f_1 - f_2$ und $f_1 + f_2$ — ohne
Grundton. Liegt einer der Faktoren außerhalb des harmonischen Rasterverhältnisses,
ergeben sich die inharmonischen Obertöne, die Glocken und metallische
Flächen prägen.

Sonderfälle:

- **Ein Port unwired:** der entsprechende Faktor ist 0 → $y_n = 0$, der
  Ringmodulator bleibt stumm, bis beide Seiten gepatcht sind.
- **Tremolo:** $x_2$ langsam (z. B. LFO-artig 1–10 Hz) → periodische
  Lautstärkeschwebung.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x_{1}$ | `input1.getSample()` | [RingModulatorProcessor.cpp](../../src/processors/RingModulatorProcessor.cpp) | Port lesen; Default 0 wenn unverbunden (`setDefaultValue(0.0f)`) |
| $x_{2}$ | `input2.getSample()` | [RingModulatorProcessor.cpp](../../src/processors/RingModulatorProcessor.cpp) | Port lesen; Default 0 wenn unverbunden (`setDefaultValue(0.0f)`) |
| $y$ | `result` → `output.setSample(result)` | [RingModulatorProcessor.cpp](../../src/processors/RingModulatorProcessor.cpp) | `input1.getSample() * input2.getSample()`; eine float-Multiplikation pro Sample |
