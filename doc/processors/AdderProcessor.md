# AdderProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Summiert zwei Signale sampleweise kanalweise zu einem Summen-Bus: `out = in1 + in2`
beide Kanäle (Welt A: EIN Audiotyp). Der Bus-Builder des Graphen: parallele
Quellen (Oszillator-Schwärme, Layer-Stacks, Percussion-Cluster) werden VOR einer
gemeinsamen Downstream-Stufe (Filter, Shaper, Hüllkurve, Delay) zu EINEM Signal
zusammengelegt, das dann die geteilte Kette durchläuft — ein Filter-Set schmückt
den ganzen Bus statt jede Quelle einzeln. Das ist KEIN Routing (dafür gibt es
den Merger: Seitenzweige seitentreu) und keine Balance-Stufe (dafür Gain):
reine, clip- und gain-freie Addition.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 8 | `GraphNodeRegistry::maxAdds` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `in1` (`PortType::signal`), `in2` (`PortType::signal`) | `InputPort input1`, `input2` | [src/processors/AdderProcessor.h](../../src/processors/AdderProcessor.h) |
| Output-Ports | `out` (`PortType::signal`) | `OutputPort output` | [src/processors/AdderProcessor.h](../../src/processors/AdderProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| *(keine)* | — | — | — | — |

Der Adder besitzt **keine APVTS-Parameter**. Er ist zustandslos: `prepare` und
`startNote` leeren nichts, es gibt keine Internat, die ein Reset brauchte.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| statisches Label | Pin-Hinweis `in1 + in2 → out` | `AdderComponent::infoLabel` | [src/gui/components/AdderComponent.h](../../src/gui/components/AdderComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | Index des gerade berechneten Samples | — |
| $x^{1}_n$ | Frame am Eingang `in1` im Sample $n$ | linear |
| $x^{2}_n$ | Frame am Eingang `in2` im Sample $n$ | linear |
| $y_n$ | Frame am Ausgang `out` im Sample $n$ | linear |

Pro Sample gilt:

$$
y_n = \big(x^{1}_{l,n} + x^{2}_{l,n},\ x^{1}_{r,n} + x^{2}_{r,n}\big)
$$

Sonderfälle:

- **Ein Summand unverbunden:** der Default liest 0 → der Adder ist neutral und
  reichnet die verbundene Seite unverändert weiter (`out = x^1` bei
  unverbundenem `in2`).
- **Beide Eingänge von derselben Quelle:** `out` = $2 \cdot x$ — die verdoppelnde
  Mischung ist wie jede Addition ungeclippt; für Headroom im Bus eine Gain-Stufe
  vor die Downstream-Stufe legen.
- **Kaskade:** $k$ Quellen brauchen $k-1$ Adder; der Adder-Pool (8) deckt
  Sechs-Quellen-Cluster mit zwei freien Stufen ab.

Hinweis zur Taktordnung: der Adder-Pool läuft nach dem Merger-Pool und vor den
Signal-Stufen — `oscN → add → filter` im selben Tick frisch; ein Adder, der in
den Splitter/Merger (frühere Pools) speist, landet einen Tick später dort
(unhörbar; gleiche Ausnahme wie alle anderen späteren Signal-Pools).

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x^{1}$ | `input1.getSample()` | [AdderProcessor.cpp](../../src/processors/AdderProcessor.cpp) | Port lesen; Default 0 wenn unverbunden (`setDefaultValue(0.0f)`) |
| $x^{2}$ | `input2.getSample()` | [AdderProcessor.cpp](../../src/processors/AdderProcessor.cpp) | Port lesen; Default 0 wenn unverbunden (`setDefaultValue(0.0f)`) |
| $y$ | `out` → `output.setSample(out)` | [AdderProcessor.cpp](../../src/processors/AdderProcessor.cpp) | `(a.l + b.l, a.r + b.r)` |
| Summen-Kontrast | Merger `(in1.l, in2.r)` vs. Adder `(in1.l+in2.l, in1.r+in2.r)` | [MergerProcessor.h](../../src/processors/MergerProcessor.h) | Merger routet Seitenzweige, Adder summiert Vollsignale |