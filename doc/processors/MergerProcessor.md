# MergerProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Führt zwei Seitenzweige zu einem Stereo-Signal zusammen. Reine Topologie
ohne DSP und ohne Parameter: `in1` liefert die linke, `in2` die rechte
Kanalspur des Ausgangs; eine unverbundene Seite liest 0 und trägt Stille
auf genau dieser Seite bei. Der Typ bleibt `PortType::signal` (Welt A:
EIN Audiotyp), daher dürfen zentrierte Mono-Quellen beide Eingänge
speisen — `out` ist dann wieder das zentrierte Eingangsframe.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 8 | `GraphNodeRegistry::maxMergers` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `in1` (`PortType::signal`, Hinweis `ChannelSide::left`), `in2` (`PortType::signal`, Hinweis `ChannelSide::right`) | `InputPort leftInput`, `rightInput` | [src/processors/MergerProcessor.h](../../src/processors/MergerProcessor.h) |
| Output-Ports | `out` (`PortType::signal`) | `OutputPort output` | [src/processors/MergerProcessor.h](../../src/processors/MergerProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| *(keine)* | — | — | — | — |

Der Merger besitzt **keine APVTS-Parameter**. Die Port-Hinweise
`ChannelSide::left` / `ChannelSide::right` an `in1`/`in2` sind reine
Anzeige-Metadaten für das L/R-Badge im UI.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| statisches Label | Pin-Hinweis `L \| R → out` | `MergerComponent::infoLabel` | [src/gui/components/MergerComponent.h](../../src/gui/components/MergerComponent.h) |
| Wire-Badge | L/R-Tag am Draht-Mittelpunkt, gelesen aus dem Side-Hint des Quell-Pins | `DraggablePanel::drawSideBadge` | [src/gui/DraggablePanel.cpp](../../src/gui/DraggablePanel.cpp) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | Index des gerade berechneten Samples | — |
| $x^{L}_n$ | Frame am Eingang `in1` im Sample $n$ | linear |
| $x^{R}_n$ | Frame am Eingang `in2` im Sample $n$ | linear |
| $y_n$ | Frame am Ausgang `out` im Sample $n$ | linear |

Pro Sample gilt:

$$
y_n = \big(x^{L}_{l,n},\ x^{R}_{r,n}\big)
$$

Sonderfälle:

- **Eine Seite unverbunden:** der Default liest 0 → die zugehörige
  Ausgangsseite ist stumm (`out = (x^L_l, 0)` bei unverbundenem `in2`).
- **Zentrierte Quelle auf beiden Eingängen:** `out` = `(x_l, x_r)` — ein
  vollständiges, zentriertes Frame (keine Mittelung, keine Verdopplung).

Hinweis zur Taktordnung: der Merger-Pool läuft nach dem Splitter-Pool und
vor den Signal-Stufen; `Splitter → Merger` in einem Tick ist exakt frisch.
Producer aus späteren Signal-Pools erreichen den Merger einen Tick
verzögert (unhörbar).

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x^{L}$ | `leftInput.getSample()` | [MergerProcessor.cpp](../../src/processors/MergerProcessor.cpp) | Port lesen; Default 0 wenn unverbunden (`setDefaultValue(0.0f)`) |
| $x^{R}$ | `rightInput.getSample()` | [MergerProcessor.cpp](../../src/processors/MergerProcessor.cpp) | Port lesen; Default 0 wenn unverbunden (`setDefaultValue(0.0f)`) |
| $y$ | `out` → `output.setSample(out)` | [MergerProcessor.cpp](../../src/processors/MergerProcessor.cpp) | `(leftSide.l, rightSide.r)` |
| Hinweis `L`/`R` | `setChannelSide (ChannelSide::left/right)` | [MergerProcessor.cpp](../../src/processors/MergerProcessor.cpp) | Konstruktor; Anzeige-Metadaten (`getChannelSide()`) |