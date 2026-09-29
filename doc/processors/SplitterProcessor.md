# SplitterProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Teilt ein Stereo-Signal in seine beiden Seitenzweige. Reine Topologie ohne
DSP und ohne Parameter: `outL` trägt nur die linke, `outR` nur die rechte
Seite; die andere Seite ist exakt 0 (Seitenreinheit). Beide Ausgänge bleiben
`PortType::signal` (Welt A: EIN Audiotyp), daher speist jeder Zweig direkt
jeden Standardprozessor ohne Konverter. Ein einzelner Seitenzweig kann
direkt in den Master geroutet werden und mischt nur in genau einen Kanal.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 4 | `GraphNodeRegistry::maxSplitters` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `in` (`PortType::signal`) | `InputPort input` | [src/processors/SplitterProcessor.h](../../src/processors/SplitterProcessor.h) |
| Output-Ports | `outL` (`PortType::signal`, Hinweis `ChannelSide::left`), `outR` (`PortType::signal`, Hinweis `ChannelSide::right`) | `OutputPort leftOutput`, `rightOutput` | [src/processors/SplitterProcessor.h](../../src/processors/SplitterProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| *(keine)* | — | — | — | — |

Der Splitter besitzt **keine APVTS-Parameter**. Die Port-Attribute
`ChannelSide::left` / `ChannelSide::right` sind reine Anzeige-Metadaten für
das L/R-Badge im UI; `connect()` und der Audiopfad ignorieren sie.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| statisches Label | Pin-Hinweis `in → L \| R` | `SplitterComponent::infoLabel` | [src/gui/components/SplitterComponent.h](../../src/gui/components/SplitterComponent.h) |
| Wire-Badge | L/R-Tag am Draht-Mittelpunkt, gelesen aus dem Side-Hint des Quell-Pins | `DraggablePanel::drawSideBadge` | [src/gui/DraggablePanel.cpp](../../src/gui/DraggablePanel.cpp) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | Index des gerade berechneten Samples | — |
| $x_n = (x_{l,n}, x_{r,n})$ | Stereo-Frame am Eingang `in` im Sample $n$ | linear |
| $y^{L}_n$ | Frame am Ausgang `outL` im Sample $n$ | linear |
| $y^{R}_n$ | Frame am Ausgang `outR` im Sample $n$ | linear |

Pro Sample gilt:

$$
y^{L}_n = (x_{l,n},\ 0) \qquad y^{R}_n = (0,\ x_{r,n})
$$

Sonderfälle:

- **Eingang unwired:** der Default liest 0 auf beiden Kanälen → beide
  Zweige sind stumm (`setDefaultValue(0.0f)`).
- **Mono-Quelle (zentriertes Frame $x_{l,n} = x_{r,n}$):** die Zweige
  bleiben dennoch seitenrein — `(v, 0)` bzw. `(0, v)`; bestehende
  Mono-Patches verlieren so keine Energie in die andere Seite.
- **Direktes Routing:** `(L, 0)` in den MasterOutput trägt nur zum linken
  Kanal bei (der Master summiert pro Kanal), ohne Merger und ohne Überlappen.

Hinweis zur Taktordnung: der Splitter-Pool läuft nach allen
Frequenz-Produzenten und vor den Signal-Stufen. Ein Producer aus einem
späteren Signal-Pool (z. B. Gain) erreicht den Splitter daher einen Tick
verzögert — unhörbar und ohne Rückkopplungspfad.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x$ | `input.getSample()` | [SplitterProcessor.cpp](../../src/processors/SplitterProcessor.cpp) | Port lesen; Default 0 wenn unverbunden (`setDefaultValue(0.0f)`) |
| $y^{L}$ | `left` → `leftOutput.setSample(left)` | [SplitterProcessor.cpp](../../src/processors/SplitterProcessor.cpp) | `(in.l, 0)` |
| $y^{R}$ | `right` → `rightOutput.setSample(right)` | [SplitterProcessor.cpp](../../src/processors/SplitterProcessor.cpp) | `(0, in.r)` |
| Hinweis `L`/`R` | `setChannelSide (ChannelSide::left/right)` | [SplitterProcessor.cpp](../../src/processors/SplitterProcessor.cpp) | Konstruktor; Anzeige-Metadaten (`getChannelSide()`) |