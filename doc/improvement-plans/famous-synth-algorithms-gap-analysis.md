# Berühmte Synthesizer-Algorithmen vs. SmolFM — Lückenanalyse

> Referenz: `doc/formats/smolfm.xsd` (Graph-Format v3), `doc/fm-frequency-routing.md`, `doc/NoteProcessor.md`, `doc/processors/*.md`.

Zweck dieser Analyse: systematisch durch die Algorithmen berühmter Synthesizer
gehen und festhalten, welche ihrer definierten Eigenschaften ein SmolFM-Graph
**ausdrücken kann**, welche nur **teilweise** und welche **gar nicht**. Für jede
nicht ausdrückbare Eigenschaft steht, welche XSD-/Engine-Erweiterung sie
ermöglichen würde. Daraus entsteht die Übersichtstabelle der fehlenden
Konzepte (Abschnitt 4) und die priorisierte Verbesserungsliste (Abschnitt 5).

---

## 1. Fähigkeitsmodell von SmolFM (aus dem XSD)

**Graph:** DAG (keine Zyklen), genau ein `note`-Knoten, genau ein `output`.
**Port-Domänen:** `frequency` (Hertz, Steuersignal) und `signal` (Audio, −1…+1);
Kanten nur zwischen gleichartigen Ports. Keine Automatisierung statischer
Parameter während einer Note. Monophon: genau eine Note gleichzeitig, 12-TET
feste Stimmung, Velocity/Aftertouch/Bend kommen nicht in den Graph.

| Knoten | Max. | Ports (Domäne) | Statische Parameter |
|---|---|---|---|
| `note` | 1 | out (frequency) | — (12-TET, ignoriert Velocity) |
| `osc` | 8 | out (frequency) | mode: pitch/static/lfo 0,01–50 Hz; 7 Wellenformen (sine, saw, square, tri, white, perlin, simplex) |
| `fm` | 4 | in: frequency+signal → out: frequency | index a ∈ 0…10 (proportional FM, durch-null-tauglich) |
| `fscale` | 8 | in/out: frequency | Faktor 0…10 (statisch) |
| `fadsr` | 4 | in/out: frequency | Attack/Decay/Sustain/Release + up/down-Faktor |
| `gain` | 8 | in/out: signal | Level 0…10, **statisch, nicht modulierbar** |
| `adsr` | 8 | in/out: signal | Attack (linear), Decay/Release (exponentiell), A/D ≤ 5 s, R ≤ 10 s |
| `shape` | 8 | in/out: signal | drive 0…1, soft/hard/fold, **nicht modulierbar** |
| `filter` | 8 | in: signal; cutoff_mod_in: frequency → out: signal | TPT-SVF 2-Pol, LP/BP/HP/Notch, 20–20000 Hz, Reso 0…1 |
| `delay` | 8 | in/out: signal | 1–2000 ms, Feedback ≤ 0,95, Mix, Sync-Optionen; **Zeit nicht modulierbar** |
| `ring` | 4 | carrier, mod (signal) → out (signal) | y = c·m |
| `am` | 4 | carrier, mod (signal) → out (signal) | depth 0…1, y = c·(1 − d + d·(m+1)) (biased AM, kein sauberer Crossfade) |
| `output` | 1 | in1…in8 (signal) | statischer Master-Level; **einziger Summierer im Graph** |

Daraus folgen die Grundbeschränkungen, auf die die Lücken zurückgehen:

1. **Keine Zyklen** → nichts kann rückgekoppelt werden (Operator-Feedback,
   Waveguide/Karplus-Strong, Comb-FM).
2. **Kein Add/Mix-Knoten vor dem Output** → zwei Signalketten können sich erst
   am (statischen, terminalen) Output vereinen; Crossfades, Parallelkaskaden,
   Pre-Filter-Layering sind nicht ausdrückbar.
3. **Keine dynamischen Crossfades** → `gain`/`shape`/`fscale`/Delay-Zeit/Drive
   sind statisch; AM ist vorgespannt (Bereich 0…2) und kein Crossfade.
4. **Keine Performance-Eingaben im Graph** → Velocity, Bend, Aftertouch,
   Portamento, Mehrstimmigkeit fehlen.
5. **Kein Sample-Speicher, kein Phasenzugriff** → Sampling, Wavetable, PWM,
   Phase-Distortion, Oszillator-Sync, gesampelte Zufallswerte (S&H) fehlen.
6. **Delay-Zeit starr** → Chorus/Flanger/Phaser/BBD-Effekte ausdruckslos.

---

## 2. Die berühmten Algorithmen (Kurzübersicht)

| # | Synth / Algorithmus | Kernidee |
|---|---|---|
| 1 | Yamaha DX7 | 6-Operator-FM, Konnektivitätsmatrix, Operator-Feedback, 4-stufige Level/Rate-Envelopes, Velocity-/Keyscaling |
| 2 | Minimoog / Moog | Subtraktiv: 24 dB/oct Ladder mit Drive, Oszillator-Sync, PWM, Glide |
| 3 | XPander / MS-20 | Filter-FM, Resonanz-Experimente, Filter-Morphing |
| 4 | TB-303 | Accent, Slide, geprägte Filter-Envelopes, 18 dB/oct Resonanz |
| 5 | Prophet-5 / Juno-106 | Polysynthese, PWM, Chorus, Velocity auf Cutoff |
| 6 | Roland JP-8000 | Supersaw (Unison-Detune-Makro) |
| 7 | ARP 2600 / Buchla | Sample&Hold-Zufall, Fold, Ring-Mod, Complex FM, Low-Pass-Gate |
| 8 | Casio CZ | Phase Distortion (gesteuerter Phasenwarp) |
| 9 | PPG Wave / Waldorf | Wavetable-Scan/Morphing über Frames |
| 10 | Prophet VS | Vector-Synthese: 4 Quellen, dynamischer Kreuzschwenk |
| 11 | Fairlight / M1 | Sampling, Multisamples |
| 12 | Kurzweil VAST | Tiefkaskadierte Shaper/Filter pro Layer |
| 13 | Yamaha VL1 / Prophecy | Physikalische Modellierung (Waveguide, Feedback) |
| 14 | GS-1 / Synergy | Additiv: Partial-Oszillatoren mit eigenen Envelopes |
| 15 | Solina / Juno | Modulierter Delay-Mix: Chorus/Ensemble, Flanger, Phaser |
| 16 | E-mu / Lexicon | Reverb / Multi-Effekte |
| 17 | SH-101 / ARP Sequencer | Sequencer-/Arp-Gates, Tempo-Sync |
| 18 | Seaboard / MPE | Per-Note-Expression (Bend, Aftertouch, Velocity) |
| 19 | Buchla / Partch | Mikrotöne, nicht-12-TET-Stimmungen |

---

## 3. Lückenanalyse pro Algorithmus

### 1) Yamaha DX7 (FM)

**Ausdrückbar:** 2–5-Operator-Kaskaden (8 `osc` + 4 `fm` decken die meisten
DX7-Architekturen ab), proportionaler, durch-null-tauglicher FM-Index,
pro-Operator-Amplituden-Envelopes (ADSR auf der Operator-Kette), statisches
Detuning über `fscale`, Keytracking über `cutoff_mod_in`.

**Nicht ausdrückbar:**
- **Operator-Feedback** (Rückkopplung eines Operators auf sich oder den Index):
  Zyklen sind im DAG verboten; `delay` sitzt im Signalpfad, nicht in der
  Frequenz-Domäne.
- **Velocity-Sensitivity pro Operator:** Velocity erreicht den Graphen nicht.
- **Keyscaling von FM-Index und Envelope-Leveln:** `fscale` ist statisch.
- **4-stufige Level/Rate-Envelopes** (DX7-„EgS") mit freien Kurvenformen,
  Halten und Loops: ADSR/FADSR haben fixe Kurvenformen.

**XSD-Ansatz:** Feedback über einen Frequenz-Domänen-`delay` (n Samples) plus
FM-Index-Modulport am `fm`-Knoten; `note` um Velocity-/Keyscale-Ports erweitern.

### 2) Minimoog / Moog (subtraktive VA)

**Ausdrückbar:** Osc-Mix über statische Gains am Output, LP/BP/HP mit
Keytracking, Hüllkurven, Noise, Vibrato via LFO→`fm`.

**Nicht ausdrückbar:**
- **24 dB/oct-Ladder-Charakter mit Drive und Selbstoszillation** (`filter` ist
  ein 2-Pol-SVF ohne Drive; `shape` kann nur statisch vorgeschaltet werden).
- **Oszillator-Sync** (kein Reset-Eingang am `osc`).
- **PWM** (`square` hat keinen Duty-Parameter, Phase nicht modulierbar).
- **Glide/Portamento** (nur Attack-Glide über `fadsr`-up/down, kein Legato).

**XSD-Ansatz:** Filter-Modus „ladder" mit Drive und Reso ≥ Selbstoszillation;
`osc` um `sync_in` (frequency) und modulierbares `duty` erweitern.

### 3) XPander / MS-20 (Filter-Experimente)

**Ausdrückbar:** Filter-FM (Hertz-Signal auf `cutoff_mod_in`), Notch/BP,
Resonanz bis nahe Selbstoszillation, Keytracking.

**Nicht ausdrückbar:** Comb/Peak/Morph-Filtertypen, Filter-Panning (Stereo),
modulierter Drive.

**XSD-Ansatz:** Filtertyp-Erweiterung (comb/peak/morph) und Drive-Mod-Port.

### 4) TB-303

**Teilweise:** Resonanzfilter mit Keytracking und `fadsr`→`cutoff_mod_in`
deckt die typische Filter-Envelope ab; 18 dB/oct-Resonanz und Drive-Kompression
nicht.

**Nicht ausdrückbar:** **Accent** und **Slide** (hängen an Velocity-/Note-
Semantik, die den Graphen nicht erreicht).

**XSD-Ansatz:** Velocity-Port am `note`-Knoten als Skalierer auf beliebige
statische Parameter (Envelope-Level, Filter-Amount, FM-Index).

### 5) Prophet-5 / Juno-106

**Teilweise:** Die Mono-Version (Oscs + Filter + Env + LFO + Noise) ist
ausdrückbar.

**Nicht ausdrückbar:** Polyphonie (ein `note`, keine Voice-Zuweisung), echtes
PWM, Chorus, Velocity→Cutoff.

**XSD-Ansatz:** Mehrfach-`note`-Instanzen bzw. Voice-Gruppen; PWM wie oben.

### 6) JP-8000 Supersaw

**Teilweise:** 8 Oszillatoren mit statischem Detune via `fscale` und Output-
Layering kommen nahe an die Idee.

**Nicht ausdrückbar:** Unison-Makro mit Phasen-Randomisierung/Spread; Chorus
als Glättung (Delay-Zeit starr, min. 1 ms, mono).

**XSD-Ansatz:** Unison-Parameter am `osc` (voices/detune/spread); modulierbare
Delay-Zeit für Chorus.

### 7) ARP 2600 / Buchla (Westcoast & Random)

**Ausdrückbar:** Fold (`shape`: fold), Ring-Modulation, Complex FM (Audio auf
FM-Index), Low-Pass-Gate (Filter + ADSR), Noise.

**Nicht ausdrückbar:**
- **Sample&Hold / clocked random:** `white`/`perlin`/`simplex` sind per-Sample
  bzw. ungeclockt; kein LFO-Typ „sample&hold", kein Clock-Eingang.
- **Dynamische Mischung (Quadratur-Crossfade):** kein Add/Mix-Knoten.

**XSD-Ansatz:** LFO-Wellenform `sample&hold` bzw. `s&h`-Knoten (signal in,
clock in, signal out); Mix/Add-Knoten (Abschnitt 5, P1).

### 8) Casio CZ (Phase Distortion)

**Nicht ausdrückbar.** Das `osc`-Element hat ein historisches Feld
`phaseModulation`, es ist aber immer 0; Phasen-Reading über Tables oder
Phasen-Warp fehlt. FM ist kein adäquater Ersatz (andere Spektralstruktur).

**XSD-Ansatz:** Wavetable am `osc` (siehe 9) oder echter `phaseMod`-Eingang
(frequency-Domäne, Phasenoffset in Radiant).

### 9) PPG Wave / Waldorf (Wavetable)

**Nicht ausdrückbar:** Kein Wavetable-Memory, kein Frame-Scan, kein Morph;
Wellenformen sind fest per-Sample generiert.

**XSD-Ansatz:** `wavetable`-Knoten (Table-ID, Frame-Position als
Modulationssignal in der Frequenz-Domäne).

### 10) Prophet VS (Vector-Synthese)

**Nicht ausdrückbar.** Vektorsynthese = dynamischer Kreuzschwenk zwischen
Quellen; ohne Mix-Knoten mit modulierbaren Gewichten unmöglich (AM ist
vorgespannt und kein Crossfade, `gain` ist statisch).

**XSD-Ansatz:** `mix`-Knoten: 2–4 signal-Eingänge, Modulationssignal
(signal-Domäne) als Position entlang der Achse(n).

### 11) Fairlight / M1 (Sampling)

**Nicht ausdrückbar:** Kein Sample-Knoten, keine Multisamples, kein Looping —
grundsätzlich außerhalb des FM-Fokus.

**XSD-Ansatz:** Optionaler `sample`-Knoten (Sample-ID, Pitch via
frequency-Port, Start/Gain statisch).

### 12) Kurzweil VAST

**Teilweise:** Kaskadierte `shape`+`filter`-Stufen pro Layer sind ausdrückbar
(8 Instanzen je) und decken einen Teil der VAST-Topologien ab.

**Nicht ausdrückbar:** Pro-Layer-Samples (11), modulierbare Shaper-Amounts,
Keyrange-Mapping über mehrere Layer.

### 13) VL1 / Prophecy (physikalische Modellierung)

**Nicht ausdrückbar:** Waveguide-/Bowed-/Reed-Modelle brauchen Feedback-Kreise
(Delay → Nichtlinearität → zurück). Im DAG fundamental unmöglich; auch
Karplus-Strong braucht einen geschlossenen Delay-Loop.

**XSD-Ansatz:** Ein begrenzter `feedback`-Knoten (Zyklus mit 1-Sample-Delay)
bricht das DAG-Modell — nur mit Bedacht als P3.

### 14) GS-1 / Synergy (additiv)

**Ausdrückbar:** Bis zu 8 Partial-Oszillatoren mit individuellen ADSRs, exakte
Harmonische über `fscale`, Summierung am Output. „Kleines Additiv" gut
ausdrückbar.

**Nicht ausdrückbar:** Mehr als 8 Partials, Kopplung/AM zwischen Partials,
formantartige Resonanz über mehr als 4 Bänder.

### 15) Solina / Juno (Chorus, Flanger, Phaser)

**Nicht ausdrückbar:** Chorus/Ensemble braucht kurze **modulierte** Delays
(LFO → Delay-Zeit); die Delay-Zeit ist statisch. Flanger braucht externe
Feedback-Modulation, Phaser braucht Allpass-Kaskaden (fehlt beides).

**XSD-Ansatz:** `time_mod`-Port (frequency-Domäne) am `delay`; optional
`allpass`-Knoten. Hebel für Chorus, Flanger, Vibrato, Dub-Delay, BBD.

### 16) Reverb

**Nicht ausdrückbar:** Kein Reverb-Knoten; er wäre ein Ausgabeketten-Effekt,
kein Noten-Graph-Knoten.

### 17) Sequencer / Arpeggiator

**Nicht ausdrückbar:** Ein Graph beschreibt genau eine Note; kein
Schritt-Muster, kein Clock-Sync von LFOs (nur Delay hat Sync-Optionen).

**XSD-Ansatz:** Tempo-Sync-Option am `osc`-LFO (wie beim Delay); Sequencer/
Arp bleiben Host-Aufgabe außerhalb des Noten-Graphen.

### 18) MPE / Seaboard

**Nicht ausdrückbar:** Velocity, Aftertouch, Pitch-Bend, Per-Note-Expression
erreichen den Graphen nicht; Monophonie schließt MPE aus.

**XSD-Ansatz:** `note` um `velocity`- (Skalierer) und `expression`-Ports
(signal-Domäne) erweitern; Portamento/Bend als FADSR-Verhalten.

### 19) Mikrotöne / Tuning-Tabellen (Buchla, Partch)

**Teilweise:** `fscale` erlaubt beliebige statische Verhältnisse, aber die
Note selbst ist 12-TET; Stimmungstabellen/Just Intonation pro Note nicht
ausdrückbar.

**XSD-Ansatz:** Skala-Referenz im `note`-Knoten (Tuning-Table-ID).

---

## 4. Fehlende Konzepte — Übersichtstabelle

Zusammenfassung aller im Abschnitt 3 identifizierten Lücken: je Zeile ein
fehlendes Konzept mit Klangwirkung und konkretem Processor-Vorschlag
(P1 = höchste Priorität, Details in Abschnitt 5).

| Fehlendes Konzept | Beschreibung | Bekannter Synthesizer | Processor-Vorschlag | Klangeigenschaften |
|---|---|---|---|---|
| `mix`/Crossfade-Knoten | Mehrere Quellen mit modulierbaren Gewichten mischen und kreuzschwenken | Prophet VS (Vector), ARP/Buchla | **P1**: neuer `mix`-Knoten (2–4 signal-Eingänge, Modulationsport) | Vector-Morphing, Layering, Parallelkaskaden |
| Modulierbare Delay-Zeit | LFO/Steuerquelle moduliert die Delaylänge im Millisekundenbereich | Juno-106, Solina | **P1**: `time_mod`-Port (Hertz-Domäne) am `delay` | Chorus, Ensemble, Flanger, Vibrato, Dub-Delay |
| Velocity als Modulationsquelle | Anschlagsstärke erreicht den Noten-Graphen | DX7, TB-303, Seaboard | **P1 erledigt**: `velocity`-Port-Typ am `note`-Knoten (`vel`-Ausgang); Receive-Pins an `gain`, `shape`, `fm`, `filter`, `fadsr` (Accent-Tiefe); ADSR folgt | Dynamik, Accent, Ausdrucksstärke |
| Operator-Feedback | Operator-Ausgang speist in den eigenen FM-Index oder den eines anderen zurück | DX7 | **P2**: Frequenz-Domänen-`delay` (n Samples) + FM-Index-Modulport | Metallisch, sägend, angereicherte Spektren |
| PWM | Modulierbare Pulsbreite der Rechteckschwingung | Minimoog, Prophet-5 | **P2**: `duty`-Parameter + Modulport am `osc` | Wärme, Bewegung, Nasalität |
| Oszillator-Sync | Slave-Oszillator wird vom Master per Reset gekoppelt | Minimoog | **P2**: `sync_in` (frequency-Domäne) am `osc` | Aggressive, brillante Lead-Sounds |
| Ladder-/24-dB-Filter mit Drive | 24 dB/oct-Resonanzfilter mit Overdrive und Selbstoszillation | Minimoog, TB-303 | **P2**: Filtermodus „ladder" + Drive-Mod-Port | Fett, sägend, klassisch saftig |
| Comb/Peak/Morph-Filter | Weitere Filtertypen und Filter-Morphing | Oberheim XPander, MS-20 | **P2**: Filtertyp-Erweiterung + Drive-Mod-Port | Kammfilterresonanzen, exotische Formanten |
| Sample&Hold | Zufallswerte werden auf Takt gesampelt | ARP 2600, Buchla | **P2**: `s&h`-Wellenform bzw. -Knoten mit Clock-Eingang | Steppende Zufallsbewegung („R2D2") |
| Wavetable-Synthese | Durchscanbare Wellenform-Frames mit Morphing | PPG Wave, Waldorf Microwave | **P2**: `wavetable`-Knoten, Frame-Position modulierbar | Digitale, bewegte, morphende Klänge |
| Phase Distortion | Gesteuerte Phasenverzerrung statt klassischer FM | Casio CZ | **P2**: `phaseMod`-Eingang bzw. Wavetable am `osc` | Bläser- und Reeds-artige digitale Spektren |
| Unison/Supersaw | Mehrere detunede Oszillatorphasen pro Voice | Roland JP-8000 | **P2**: Unison-Parameter am `osc` (voices/detune/spread) | Breite, fette Supersaw-Wände |
| Keyscaling | Parameter folgen der Tonhöhe | DX7, Prophet-5 | **P2**: Keyscale-Ports am `note`-Knoten | Gleiche Balance über den ganzen Tastenumfang |
| Freie mehrstufige Envelopes | Level/Rate-Stufen, Halten, Loops, freie Kurven | DX7, Ensoniq SQ-80 | **P2**: erweiterter `env`-Knoten | Percussive bis organisch modulierte Hüllkurven |
| Phaser/Allpass | Modulierte Allpass-Kaskade | Solina, Juno | **P2**: `allpass`-Knoten | Wirbelnde Phasing-Effekte |
| Physikmodell-Feedbackkreis | Zyklen im Graph (Waveguide, Karplus-Strong) | Yamaha VL1, Prophecy | **P3**: begrenzter `feedback`-Knoten | Streicher, Bläser, perkussive Zupfklänge |
| Polyphonie/Voice-Gruppen | Mehrere Noten gleichzeitig mit Voice-Zuweisung | Prophet-5, Juno-106, M1 | **P3**: Mehrfach-`note`-Instanzen bzw. Voice-Gruppen | Akkorde, Pads, Ensembles |
| Sampling | Wiedergabe gesampelter Wellenformen | Fairlight CMI, Korg M1 | **P3**: `sample`-Knoten | Realistische, naturalistische Instrumente |
| Pitch-Bend/Portamento/MPE | Glissando und Per-Note-Expression | CS-80, Seaboard | **P3**: `expression`-Port am `note`; Bend als FADSR-Verhalten | Fließende Übergänge, individuelles Vibrato |
| Mikrotöne/Tuning-Tabellen | Nicht-12-TET-Stimmungen pro Note | Buchla, Partch | **P3**: Tuning-Table-ID am `note`-Knoten | Just Intonation, Obertonskalen |
| Reverb | Raumsimulation im Ausgangspfad | Lexicon, SPX90 | **P3**: Ausgabeketten-Effekt (außerhalb des Noten-Graphen) | Räumlichkeit, Tiefe |
| LFO-Clock-Sync/Sequencer | Tempo-getaktete Modulation und Schrittsequenzen | SH-101, TB-303 | **P3**: Tempo-Sync am `osc`-LFO; Sequencer im Host | Rhythmische Gate- und Arpeggio-Bewegung |

---

## 5. Priorisierte Verbesserungen

P1 — größter Expressivitätszuwachs, kleine XSD-Änderungen:

1. **`mix`-Knoten** (2–4 signal-Eingänge, 1 Modulationsport, statische
   Basispegel): eröffnet Crossfades, Vector-Synthese, Parallelkaskaden,
   Pre-Filter-Layering. Hebt 7, 10, 12.
2. **Modulierbare Delay-Zeit** (`time_mod` in Hertz-Domäne): Chorus, Flanger,
   Vibrato, Dub-Delay. Hebt 6, 15.
3. **Velocity-Port am `note`-Knoten** als Skalierer auf FM-Index,
   Envelope-Level, Filter-Amount. Hebt 1, 4, 5, 18.
   *Erledigt für Gain (`gain.vel` = Accent-Faktor), Waveshaper (`shape.vel`
   = Accent-Drive), FM-Index (`fm.vel`), Filter-Cutoff (`filter.vel`) und
   F-ADSR (`fadsr.vel` = Accent-Tiefe, Pull-to-Neutral: Velocity zieht beide
   Anker von Neutral 1.0 zu den konfigurierten Faktoren, `F' = 1 + v·(F − 1)`;
   Tiefe statt Kurvenform — Ankerabstand `gap = v·(up−down)`, A/D/S/R-Zeiten
   unangetastet, `v = 0` byteidentisch zum unverdrahteten Verhalten);
   Port-Typ `velocity`, Quelle `note.vel`. ADSR-Gain-Anker offen (Stufe 2:
   `gain_up` am Attack-Peak, `gain_down` am Sustain, Notenstart/Release-Ende
   hart null).*

P2 — einzelne berühmte Algorithmen erreichbar machen:

4. **Wavetable-Knoten** (9, 8) oder echter Phasenmod-Eingang am `osc`.
5. **Filter-Erweiterungen**: 24 dB/Ladder-Modus mit Drive, Comb,
   Drive-Mod-Port (2, 3, 4, 12).
6. **PWM** (`duty` + Modulport) und **Osc-Sync** (`sync_in`) (2, 5).
7. **S&H-Wellenform bzw. `s&h`-Knoten** mit Clock-Eingang (7).
8. **Operator-Feedback**: Frequenz-Domänen-Delay (n Samples) +
   FM-Index-Modulport (1).

P3 — Architektur-/Host-Fragen, bewusst spät:

9. Begrenzter Zyklus im Graph (physikalische Modellierung, 13).
10. Polyphonie/Voice-Gruppen (5, 18).
11. Stereo/Panning, Reverb als Ausgabekette (3, 16).
12. Tuning-Tabellen (19), Sequencer/Arp (17).