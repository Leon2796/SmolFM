# FeedbackProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Begrenzter Zyklusbrecher nach dem TR-808-Plan P3.6 (A7): `out = in + amount·fb_in`.
Er macht Schaltungs-Rückkopplungen (BD-Damping, Loop-Entwürfe) im zyklenfreien
Graphen ausdrückbar: Jeder Graph-Zyklus trägt implizit 1 Sample Verzögerung,
weil der Rückkanal den vorherigen Tick des (später in der Render-Reihenfolge
stehenden) Erzeugers liest. `amount` dämpft je Umlauf (Stabilität bis 0.95);
`amount = 0` ist ein transparenter Durchlass.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 4 | `GraphNodeRegistry::maxFeedbacks` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `in` (Schleifeneingang, `PortType::signal`), `fb_in` (Rücklauf, `PortType::signal`) | `InputPort input`, `InputPort fbIn` | [src/processors/FeedbackProcessor.h](../../src/processors/FeedbackProcessor.h) |
| Output-Ports | `out` (Schleifenaustritt, `PortType::signal`) | `OutputPort output` | [src/processors/FeedbackProcessor.h](../../src/processors/FeedbackProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Amount | `feedback<N>Amount` | Float, 0–0.95, Default 0.3 | `std::atomic<float>* amount` | [src/processors/FeedbackProcessor.h](../../src/processors/FeedbackProcessor.h) |

`<N>` = Instanzindex 0–3.

Verdrahtung (typisch): Exciter-Kette → `feedback0.in`; `feedback0.out` →
Körper (`resonator0.in`, Shaper, ...); Rücklauf: spätes Glied des Körpers →
`feedback0.fb_in` (liest dessen Vorher-Tick-Frame = 1 Sample Verzögerung
pro Umlauf). Direkt `feedback0.out` → `feedback0.fb_in` ergibt einen
Ein-Pol-Loop ($y[n] = x[n] + a\,y[n-1]$, stabil für $a \le 0.95$).

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(keine)* | Amount-Rotary Bindung an den APVTS-Parameter | `FeedbackComponent::amountSlider` | [src/gui/components/FeedbackComponent.h](../../src/gui/components/FeedbackComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

Symboltabelle:

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $x_n$ | Schleifeneingang (`in`) | Amplitude |
| $r_n$ | Rücklauf (`fb_in`) — der vorherige Tick des Erzeugers, wenn dieser später in der Render-Reihenfolge steht | Amplitude |
| $a$ | wirksamer Umlauf-Anteil $= \mathrm{clamp}(\text{amount}, 0, 0.95)$ | linear |
| $y_n$ | Ausgang $= x_n + a\, r_n$ | Amplitude |

Funktionsablauf pro Sample: Lesen beider Eingänge, Skalar-Multiplikation des
Rücklaufs mit $a$, Summation, Ausgabe. Ohne eigenen Zustand lebt die
Schleifenenergie in der Verdrahtung (Ports/Körper-Zustände); sie verliert pro
Umlauf den Faktor $(1-a)$-ähnlich und stirbt dadurch immer aus — kein
Aufschaukeln. Ein-Pol-Sonderfall (Direkt-Schleife): Sustained-Gain
$x/(1-a)$-beschränkt durch das 0.95-Limit.

Spezialfälle:
- `amount = 0`: transparenter Durchlass ($y = x$) — als Zyklusbrecher
  einsetzbar ohne Klangeinfluss.
- `reset()` (neuer Patch): löscht nur `tailPeak`; die Schleife entleert sich
  über die Per-Durchlauf-Dämpfung.
- `hasEnergy()`: Voice-Lifetime — eine verdrahtete Schleife hält die Voice
  am Leben, solange sie noch Audio zirkuliert (wie Delay-Tail).

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x_n$ | `input.getSample()` | [FeedbackProcessor.cpp](../../src/processors/FeedbackProcessor.cpp) | Port lesen (Default 0) |
| $r_n$ | `fbIn.getSample()` | [FeedbackProcessor.cpp](../../src/processors/FeedbackProcessor.cpp) | Port lesen (Default 0); 1-Sample-Latenz ergibt sich aus der Render-Reihenfolge, nicht aus eigenem Zustand |
| $a$ | `amount->load()` → `fbAmount` | [FeedbackProcessor.cpp](../../src/processors/FeedbackProcessor.cpp) | jlimit 0–0.95 |
| $y_n$ | `y = x + fb*fbAmount`; `output.setSample(y)` | [FeedbackProcessor.cpp](../../src/processors/FeedbackProcessor.cpp) | kanalweise Summation |
| Tail-Energie | `tailPeak`, `hasEnergy()` | [FeedbackProcessor.h](../../src/processors/FeedbackProcessor.h) | Peak-Follower über die zirkulierenden Frames |