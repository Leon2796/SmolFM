# FilterProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Multi-Mode-State-Variable-Filter (LP/BP/HP/Notch) mit statischer Cutoff-
Einstellung und optionalem Frequenzeingang `cutoff_mod_in`, dessen Wert die
statische Cutoff vollständig ersetzt (kein Offset). TPT-Trapez-Integration —
stabil auch bei hoher Resonance, Cutoff wird pro Sample neu hergeleitet.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 8 | `GraphNodeRegistry::maxFilters` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `in` (`PortType::signal`), `cutoff_mod_in` (`PortType::frequency`) | `InputPort input`, `InputPort cutoffMod` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |
| Output-Ports | `out` (`PortType::signal`) | `OutputPort output` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Cutoff | `filter<N>Cutoff` | Float, 20–20000 Hz (log, skew 0.3), Default 1000 | `std::atomic<float>* cutoff` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |
| Resonance | `filter<N>Resonance` | Float, 0–1, Default 0 | `std::atomic<float>* resonance` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |
| Mode | `filter<N>Mode` | Choice 0–3 (LP, BP, HP, Notch), Default 0 | `std::atomic<float>* mode` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |

`<N>` = Instanzindex 0–7.

`cutoff_mod_in` unverbunden: statischer `cutoff`-Parameter. Verbunden: der
Portwert (Hz) **ist** die Cutoff (auf 20–20000 Hz begrenzt).

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(keine)* | Cutoff-Rotary + Res-Rotary + Mode-Combo | `FilterComponent::cutoffSlider`, `resonanceSlider`, `modeBox` | [src/gui/components/FilterComponent.h](../../src/gui/components/FilterComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

Symboltabelle:

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $x_n$ | Eingangssample | Amplitude |
| $f_c$ | aktuelle Cutoff-Frequenz (stat. oder aus `cutoff_mod_in`) | Hz |
| $f_s$ | Sample-Rate | Hz |
| $r$ | Resonance (0–1) | linear |
| $k$ | Dämpfungsfaktor (SVF) | linear |
| $g$ | Integrator-Koeffizient $\tan(\pi f_c/f_s)$ | linear |
| $m$ | Mode-Index (0–3) | — |
| $y_n$ | Ausgangssample | Amplitude |
| $\mathrm{ic1eq}, \mathrm{ic2eq}$ | TPT-Integrator-Zustände | Amplitude |

Koeffizienten (pro Sample neu):

$$g = \tan\!\left(\pi \cdot \frac{\mathrm{jlimit}(0.0005, 0.49,\; f_c/f_s)}{}\right), \qquad k = 2\,(1 - 0.99\,r)$$

$$a_1 = \frac{1}{1 + g(g + k)}, \qquad a_2 = g\,a_1, \qquad a_3 = g\,a_2$$

Zustandsupdate (trapezoidal):

$$v_3 = x_n - \mathrm{ic2eq}, \qquad v_1 = a_1\,\mathrm{ic1eq} + a_2\,v_3, \qquad v_2 = \mathrm{ic2eq} + a_2\,\mathrm{ic1eq} + a_3\,v_3$$

$$\mathrm{ic1eq} \leftarrow 2v_1 - \mathrm{ic1eq}, \qquad \mathrm{ic2eq} \leftarrow 2v_2 - \mathrm{ic2eq}$$

Ausgang nach Mode $m$:

$$
y_n = \begin{cases}
v_2 & m = 0 \text{ (LP)}\\
v_1 & m = 1 \text{ (BP)}\\
x_n - k\,v_1 - v_2 & m = 2 \text{ (HP)}\\
x_n - k\,v_1 & m = 3 \text{ (Notch)}
\end{cases}
$$

Spezialfälle:
- $r = 0$: $k = 2$ (maximal gedämpft, $Q = 0.5$).
- $r \to 1$: $k \to 0.02$ (nahe Selbstoszillation, bleibt stabil).
- `cutoff_mod_in` unverbunden: $f_c$ = statischer `cutoff`-Parameter.
- `startNote()` setzt beide Integrator-Zustände auf 0 (kein Rumbler aus dem
  vorherigen Note-Tail).

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x_n$ | `input.getSample()` → `x` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | Port lesen (Default 0.0f unverbunden) |
| $f_c$ (dynamisch) | `cutoffMod.isConnected()` → `cutoffMod.getSample()` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | jlimit 20–20000 |
| $f_c$ (statisch) | `cutoff->load()` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | Fallback ohne Mod-Port |
| $r$ | `resonance->load()` → `res` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | jlimit 0–1 |
| $m$ | `mode->load()` → `modeIndex` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | round + jlimit 0–3 |
| $g, k, a_1..a_3$ | `g`, `k`, `a1`, `a2`, `a3` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | pro Sample in `processSample()` |
| $v_1, v_2, v_3$ | `v1`, `v2`, `v3` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | TPT-Zwischengrößen |
| $\mathrm{ic1eq}, \mathrm{ic2eq}$ | `ic1eq`, `ic2eq` | [FilterProcessor.h](../../src/processors/FilterProcessor.h) | Zustandsfelder, Reset in `startNote()` |
| $y_n$ | `output.setSample(y)` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | Mode-Switch auf `out` |