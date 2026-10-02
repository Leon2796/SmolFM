# ResonatorProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Angeregter, gedämpfter Resonator zweiter Ordnung (komplexes Polpaar) nach dem
TR-808-Plan P2.2 (A1): der Exciter-Eingang kippt Energie in den Resonanzmodus,
das Netzwerk antwortet abklingend pseudo-sinusförmig. Dekay ist die Abklingzeit
des Polradius — keine überlagerte Hüllkurve. Mit langem Dekay ist der Knoten
die Ring-Quelle (BD, Toms, Congas, Claves), mit kurzem Dekay der Körperfilter
(Snare-Bodies 185/330); derselbe Kern in zwei quantitativen Lagen, daher ein
Knoten mit einem `out` (die zweite Variante aus der Plan-Diskussion bleibt
offen).

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 8 | `GraphNodeRegistry::maxResonators` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `in` (Exciter, `PortType::signal`), `tune` (`PortType::frequency`), `q_mod_in` (`PortType::signal`), `vel` (`PortType::velocity`) | `InputPort input`, `InputPort tuneIn`, `InputPort qMod`, `InputPort velIn` | [src/processors/ResonatorProcessor.h](../../src/processors/ResonatorProcessor.h) |
| Output-Ports | `out` (`PortType::signal`) | `OutputPort output` | [src/processors/ResonatorProcessor.h](../../src/processors/ResonatorProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Tune | `resonator<N>Tune` | Float, 20–8000 Hz (log, skew 0.3), Default 100 | `std::atomic<float>* tune` | [src/processors/ResonatorProcessor.h](../../src/processors/ResonatorProcessor.h) |
| Q | `resonator<N>Q` | Float, 0–1, Default 0.5 (≙ $Q\approx1$) | `std::atomic<float>* q` | [src/processors/ResonatorProcessor.h](../../src/processors/ResonatorProcessor.h) |
| Decay | `resonator<N>Decay` | Float, 0.01–10 s (skew 0.5), Default 0.3 (t60) | `std::atomic<float>* decay` | [src/processors/ResonatorProcessor.h](../../src/processors/ResonatorProcessor.h) |
| Vel Spread | `resonator<N>VelSpread` | Float, 0.25–4, Default 1 (neutral) | `std::atomic<float>* velSpread` | [src/processors/ResonatorProcessor.h](../../src/processors/ResonatorProcessor.h) |

`<N>` = Instanzindex 0–7.

`tune` unverbunden: statischer `tune`-Parameter. Verbunden: der Portwert (Hz)
**ist** die Resonanzfrequenz (20–8000 Hz, kein Offset) — Key-Tracking bleibt
patchbar (note → fscale → tune), F-ADSR-Dives gliden den Körper (fadsr →
tune; tune ist Frequenz-Domäne, der Glide braucht keinen eingebauten Quirk).
Port-ID-Sync: `GraphNodes`-Spec, XSD-Annotation (`resonator`-complexType) und
`SynthVoice::resolveInput` tragen denselben Namen `tune`.

`q_mod_in` unverbunden: statisches `q`. Verbunden: der Stereo-Mittelwert wird
auf $[-1,+1]$ geclampt und multipliziert das statische q —
$q_{\text{eff}} = \mathrm{clamp}(q\cdot(1+m), 0, 1)$ (Breite der Kopplungsstufe).

`vel` unverbunden: neutral (Port-Default 1.0, Spread-Default 1) — byteidentisch.
Verbunden: $t_{\text{eff}} = t \cdot (1 + v\,(spread - 1))$ mit dem Stereo-Mittelwert
$v\in[0,1]$ — akzentuierte Schläge klingen länger (Accent-Elektronik A2:
„Decay über VIN", Spread-Muster aus P1).

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(keine)* | Tune-/Q-/Decay-/VelSpread-Rotaries Bindung an die vier APVTS-Parameter | `ResonatorComponent::tuneSlider`, `qSlider`, `decaySlider`, `spreadSlider` | [src/gui/components/ResonatorComponent.h](../../src/gui/components/ResonatorComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

Symboltabelle:

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $x_n$ | Exciter-Sample (`in`) | Amplitude |
| $f_0$ | aktuelle Resonanzfrequenz (stat. oder aus `tune`) | Hz |
| $f_s$ | Sample-Rate | Hz |
| $q$ | Basis-Q (0–1); wirksam $q_{\text{eff}} = \mathrm{clamp}(q\,(1+m_{\text{q}}),0,1)$ mit $m_{\text{q}}$ aus `q_mod_in` | linear |
| $t_{60}$ | Basis-Dekay (stat.); wirksam $t_{60,\text{eff}} = \mathrm{clamp}(t_{60}\,(1+v\,(spread-1)),\,0.005,\,10)$ | s |
| $g$ | TPT-Integrator-Koeffizient $\tan(\pi f_0/f_s)$ | linear |
| $k_A$ | Dämpfung der Kopplungsstufe $2\,(1-0.99\,q_{\text{eff}})$ | linear |
| $k_B$ | Dämpfung der Ringstufe $\mathrm{clamp}(\ln 1000/(\pi f_0\, t_{60,\text{eff}}),\ 0.005,\ 2)$ | linear |
| $\mathrm{ic1}_{A,B},\ \mathrm{ic2}_{A,B}$ | TPT-Zustände je Stufe (Stereo-Paar) — das Polpaar | Amplitude |
| $y_n$ | Ausgangssample $= k_B\, v_{1,B}$ | Amplitude |

Koeffizienten (pro Sample, TPT/Simper wie im Filter):

$$g = \tan\!\big(\pi\cdot \mathrm{jlimit}(0.0005, 0.49,\; f_0/f_s)\big), \qquad a_1 = \frac{1}{1+g(g+k)}, \qquad a_2 = g\,a_1, \qquad a_3 = g\,a_2$$

Zustandsupdate je Stufe $S\in\{A,B\}$ (Bandpass-Ausgang $v_1$):

$$v_3 = e_n - \mathrm{ic2}_S,\quad v_1 = a_1\mathrm{ic1}_S + a_2 v_3,\quad v_2 = \mathrm{ic2}_S + a_2 \mathrm{ic1}_S + a_3 v_3,\quad \mathrm{ic1}_S \leftarrow 2v_1 - \mathrm{ic1}_S,\quad \mathrm{ic2}_S \leftarrow 2 v_2 - \mathrm{ic2}_S$$

Kaskade: $e = v_{1,A}(x)$; Ausgang $y = k_B\, v_{1,B}(e)$ (Peak-Normalisierung:
ein Dauerton bei $f_0$ passiert mit Gain ≈ 1). Mit $\tau = t_{60}/\ln 1000$
gilt $A(t) = A_0 e^{-t/\tau}$ — Dekay ist der Polradius, und hohe Q und langer
Ring sind dieselbe Zahl ($Q \approx \pi f_0 \tau$).

Spezialfälle:
- `in` unverbunden (0): beide Stufen bleiben still — der Knoten ist inert.
- `q = 1`: $k_A \to 0.02$, Kopplungsstufe nahe Selbstresonanz (stabil, wie
  `filter` bei Resonance 1).
- `startNote()` hält den Schwanz (Retrigger kippt Energie in den noch
  schwingenden Körper); `reset()` (neuer Patch / Rate-Wechsel) löscht die
  Zustände und `tailPeak`.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x_n$ | `input.getSample()` → `x` | [ResonatorProcessor.cpp](../../src/processors/ResonatorProcessor.cpp) | Port lesen (Default 0.0f unverbunden) |
| $f_0$ | `tuneIn.isConnected()` ? `tuneIn.getSample().l` : `tune->load()` | [ResonatorProcessor.cpp](../../src/processors/ResonatorProcessor.cpp) | jlimit 20–8000 |
| $t_{60,\text{eff}}$ | `velIn.getSample()` + `velSpread->load()` | [ResonatorProcessor.cpp](../../src/processors/ResonatorProcessor.cpp) | $t\,(1+v\,(spread-1))$, jlimit 0.005–10; unverbunden neutral |
| $q_{\text{eff}}$ | `qMod.getSample()` + `q->load()` → `res` | [ResonatorProcessor.cpp](../../src/processors/ResonatorProcessor.cpp) | $q(1+m)$, jlimit 0–1 |
| $k_A,\ k_B$ | `kA`, `kB` | [ResonatorProcessor.cpp](../../src/processors/ResonatorProcessor.cpp) | pro Sample; $k_B$-Formel oben |
| $g, a_1..a_3$ | `g`, `a1A..a3A`, `a1B..a3B` | [ResonatorProcessor.cpp](../../src/processors/ResonatorProcessor.cpp) | pro Sample |
| TPT-Schritt | `tptBandpassStep (x.l, a1A, a2A, a3A, ic1A.l, ic2A.l)` | [ResonatorProcessor.cpp](../../src/processors/ResonatorProcessor.cpp) | Stufe A je Kanal, dann Stufe B ×`kB` |
| $y_n$ | `output.setSample(y)` | [ResonatorProcessor.cpp](../../src/processors/ResonatorProcessor.cpp) | Ring + getöntes Signal auf `out` |
| Tail-Energie | `tailPeak`, `hasEnergy()` | [ResonatorProcessor.h](../../src/processors/ResonatorProcessor.h) | Peak-Follower; Voice-Lifetime (`SynthVoice::hasReasonToLive`) |