# FilterProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Multi-Mode-State-Variable-Filter (LP/BP/HP/Notch/AP) mit statischer Cutoff-
Einstellung und optionalem Frequenzeingang `cutoff_mod_in`, dessen Wert die
statische Cutoff vollständig ersetzt (kein Offset). TPT-Trapez-Integration —
stabil auch bei hoher Resonance, Cutoff wird pro Sample neu hergeleitet.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 16 | `GraphNodeRegistry::maxFilters` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Bis zu 16 Klangfarben-Blendregler pro Voice — je Signalfaden (Layer, Bus, Kopie) einer. |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `in` | Input | `signal` | liest 0 → das Filter läuft weiter, gibt aber Stille weiter | **Rohsignal.** Hier kommt das Signal herein, aus dem die gewünschten Frequenzbereiche herausgelockt werden — alle anderen Teile verwirft das Fenster. | `InputPort input` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |
| `cutoff_mod_in` | Input | `frequency` | unverbunden: der statische Cutoff-Regler gilt | **Die Stimme der Blendgrenze.** Verbunden ersetzt der Hz-Wert den Cutoff-Regler komplett (kein Offset, 20–20000 Hz) — Key-Tracking bzw. F-ADSR-Dives gliden die Klangfarbe hoch/runter. | `InputPort cutoffMod` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |
| `resonance_mod_in` | Input | `signal` | liest 0 → die statische Resonance wirkt unverändert | **Die Resonanz-Schwellkurbel.** Multipliziert die eingestellte Resonanz ($r\cdot(1+m)$, auf 0–1 geclampt): Perlin = organische Q-Atmung, Saw-LFO = zyklische Selbst-Oszillations-Rampen, ADSR = kurze Hammer-Pings. | `InputPort resonanceMod` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |
| `vel` | Input | `velocity` | liest 1.0 → neutral (byteidentisch) | **Anschlag öffnet die Blende.** Anschlagstärke multipliziert die wirksame Cutoff ($f_c \cdot v$, 20–20000): harte Anschläge sitzen heller, zarte dunkler — der Noise-Snap-Akzent der 808-Snare. | `InputPort velIn` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |
| `out` | Output | `signal` | — | **Durchlässige Signalfarben:** der gefilterte Klang nach dem gewählten Fenster (LP/BP/HP/Notch/AP). | `OutputPort output` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| Cutoff | `filter<N>Cutoff` | Float, 20–20000 Hz (log, skew 0.3), Default 1000 | Die Blendgrenze der Klangfarbe — wie ein Dimmer zwischen dunklem Fundament (tief) und hellen Spitzen (hoch); der Regler, solange `cutoff_mod_in` leer bleibt. | `std::atomic<float>* cutoff` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |
| Resonance | `filter<N>Resonance` | Float, 0–1, Default 0 | Das Anheben der Kante an der Blendgrenze: 0 = sanft zugehalten, nahe 1 = die Kante piept fast von selbst (Selbst-Oszillation, bleibt stabil). | `std::atomic<float>* resonance` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |
| Mode | `filter<N>Mode` | Choice 0–4 (LP, BP, HP, Notch, AP), Default 0 | Der Flügelsatz am Fenster: Lowpass (nur Tiefen), Bandpass (schmale Klangzunge), Highpass (nur Höhen), Notch (ein Loch im Spektrum), Allpass (Phasen-Dreher ohne Pegeländerung: Amplitude bleibt 1, die Phase rotiert 0°→−360°; Cutoff ist der Drehpunkt der Drehung, Resonanz ihre Kantigkeit — high = steile, konzentrierte Kippung, low = breite, flache Drehung). | `std::atomic<float>* mode` | [src/processors/FilterProcessor.h](../../src/processors/FilterProcessor.h) |
| Bypass | `filter<N>Bypass` | Bool, Default aus | Bypass-Hahn: 1 = Ausgang ist Bit für Bit der Eingang (neutraler Durchlass); die TPT-Integrator-Zustände (ic1eq/ic2eq) bleiben eingefroren — Un-Bypass setzt das Filtern exakt da fort, wo es stand. | `std::atomic<float>*` via `Processor::setBypassParam` | [src/processors/Processor.h](../../src/processors/Processor.h) |

`<N>` = Instanzindex 0–7.

`cutoff_mod_in` unverbunden: statischer `cutoff`-Parameter. Verbunden: der
Portwert (Hz) **ist** die Cutoff (auf 20–20000 Hz begrenzt).

`resonance_mod_in` unverbunden: statische `resonance`. Verbunden: der
Stereo-Mittelwert des Ports wird auf $[-1,+1]$ geclampt und multipliziert
die statische Resonanz — $r_{\text{eff}} = \mathrm{clamp}(r\cdot(1+m), 0, 1)$
(0–1). Damit ist die Resonanz selbst modulierbar: Perlin → organische
Q-Atmung, Saw-LFO → zyklische Selbstoszillations-Rampen, statischer Osc →
ADSR → Hammer-Envelopes für modale Pings.
Port-ID-Sync: `GraphNodes`-Spec, XSD-Annotation (`filter`-complexType) und
`SynthVoice::resolveInput` tragen denselben Namen `resonance_mod_in`.

`vel` unverbunden: statische Cutoff unverändert (Default 1.0). Verbunden:
der Stereo-Mittelwert (0–1) multipliziert die wirksame Cutoff —
$f_{c,\text{eff}} = \mathrm{clamp}(f_c \cdot v,\ 20..20000)$: akzentuierte
Anschläge sitzen heller, zarte Anschläge dunkler (Noise-Snap der
808-Snare). Port-ID-Sync: `GraphNodes`-Spec, XSD-Annotation
(`filter`-complexType) und `SynthVoice::resolveInput` tragen denselben
Namen `vel`.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| Bypass-Schalter | „Bypass": Ausgang = Eingang neutral, TPT-Zustand eingefroren | `DraggableComponent::bypassButton` — ⊘-Icon links oben in der Node-Titel-Leiste; engagiert = Node ausgegraut | [src/gui/DraggableComponent.cpp](../../src/gui/DraggableComponent.cpp) |
| *(keine)* | Cutoff-Rotary + Res-Rotary + Mode-Combo | `FilterComponent::cutoffSlider`, `resonanceSlider`, `modeBox` | [src/gui/components/FilterComponent.h](../../src/gui/components/FilterComponent.h) |
| Effektivwert-Anzeige (Ghost-Needle/Band) am Cutoff-Regler | zeigt bei Tastendruck den wirksamen Cutoff: ist `cutoff_mod_in` verdrahtet, ersetzt der Port den Regler ganz (`vel` bleibt bewusst Sound-Only, bewegt die Nadel nie) — der statische Reglerwert ist dann gar nicht mehr im Spiel, die Nadel zeigt den gültigen Wert (Telemetrie: Min/Max-Fenster ≈ 5 ms, die Nadel pendelt zwischen den echten Extremen; der Bogen zeigt Modulationstiefe/Polyphonie-Band) | `FilterComponent::cutoffOverlay` (Typ `ModIndicatorOverlay`, Quelle `FilterProcessor::getEffectiveCutoffRange`) | [src/gui/components/ModIndicatorOverlay.h](../../src/gui/components/ModIndicatorOverlay.h) |
| Effektivwert-Anzeige (Ghost-Needle/Band) am Res-Regler | zeigt bei Tastendruck die wirksame Resonanz — statischer Reglerwert, skaliert über `resonance_mod_in` (Telemetrie: Min/Max-Fenster ≈ 5 ms; Band = Min/Max über die spielenden Voices; der Daumen bleibt unberührt) | `FilterComponent::resonanceOverlay` (Typ `ModIndicatorOverlay`, Quelle `FilterProcessor::getEffectiveResonanceRange`) | [src/gui/components/ModIndicatorOverlay.h](../../src/gui/components/ModIndicatorOverlay.h) |

## Abschnitt 3 — Mathematische Beschreibung

Symboltabelle:

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $x_n$ | Eingangssample | Amplitude |
| $f_c$ | aktuelle Cutoff-Frequenz (stat. oder aus `cutoff_mod_in`) | Hz |
| $f_s$ | Sample-Rate | Hz |
| $r$ | Basis-Resonanz (0–1); wirksam $r_{\text{eff}} = \mathrm{clamp}(r\,(1+m_{\text{res}}), 0, 1)$ mit $m_{\text{res}}$ aus `resonance_mod_in` (unverbunden $= 0$) | linear |
| $k$ | Dämpfungsfaktor (SVF) | linear |
| $g$ | Integrator-Koeffizient $\tan(\pi f_c/f_s)$ | linear |
| $m$ | Mode-Index (0–4) | — |
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
x_n - k\,v_1 & m = 3 \text{ (Notch)}\\
x_n - 2k\,v_1 & m = 4 \text{ (Allpass: } |y_n| = 1,\ \varphi \text{ rotiert } 0^\circ \to -360^\circ)
\end{cases}
$$

Spezialfälle:
- $r = 0$: $k = 2$ (maximal gedämpft, $Q = 0.5$).
- $r \to 1$: $k \to 0.02$ (nahe Selbstoszillation, bleibt stabil).
- Im AP-Modus ($m=4$) bleibt die Amplitude an jeder Frequenz exakt 1
  (bilinear-exakter Allpass); nur die Phase rotiert — $f_c$ ist der Punkt
  der $180^\circ$-Wende, $r$ setzt die Kantigkeit der Drehung.
- `cutoff_mod_in` unverbunden: $f_c$ = statischer `cutoff`-Parameter.
- `startNote()` setzt beide Integrator-Zustände auf 0 (kein Rumbler aus dem
  vorherigen Note-Tail).

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x_n$ | `input.getSample()` → `x` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | Port lesen (Default 0.0f unverbunden) |
| $f_c$ (dynamisch) | `cutoffMod.isConnected()` → `cutoffMod.getSample()` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | jlimit 20–20000 |
| $f_c$ (statisch) | `cutoff->load()` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | Fallback ohne Mod-Port |
| $r$ | `resonance->load()` → `resBase` → `res` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | Basis jlimit 0–1; `resonance_mod_in` verbunden: $r \cdot (1+m)$, $m$ = Stereo-Mittel geclampt $[-1,+1]$, Ergebnis jlimit 0–1 |
| $m$ | `mode->load()` → `modeIndex` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | round + jlimit 0–4 |
| $g, k, a_1..a_3$ | `g`, `k`, `a1`, `a2`, `a3` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | pro Sample in `processSample()` |
| $v_1, v_2, v_3$ | `v1`, `v2`, `v3` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | TPT-Zwischengrößen |
| $\mathrm{ic1eq}, \mathrm{ic2eq}$ | `ic1eq`, `ic2eq` | [FilterProcessor.h](../../src/processors/FilterProcessor.h) | Zustandsfelder, Reset in `startNote()` |
| $y_n$ | `output.setSample(y)` | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | Mode-Switch auf `out` |
| $v$ | `velIn.getSample()` → Stereo-Mittel | [FilterProcessor.cpp](../../src/processors/FilterProcessor.cpp) | `vel` verbunden: $f_c \leftarrow f_c \cdot v$, dann jlimit 20–20000; unverbunden: Default 1.0 (neutral) |