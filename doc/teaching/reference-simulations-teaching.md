# Teaching: Wie die Referenzen den TR-808 simulieren — die fünf Wege aus `808 simulations.txt`

Stand: 03.10.2026. Anlass: die Tabellen-Sammlung in
[`808 bass drum simulation.md`](../research/808%20bass%20drum%20simulation.md)
sagte den Ablauf nicht mehr her — hier wird jede Referenz einzeln erklärt, mit
ihrem eigentlichen Bauplan („Wie macht sie den 808?"), und am Ende steht die
Antwort auf unsere Kernfrage: **wie kommt die Energie vom Anschlag in den
Körper — und warum fühlte sich Velocity beim Noise-Exciter „random" an.**
Verwandt: [tuned-strike-exciter-teaching.md](tuned-strike-exciter-teaching.md)
(unser Exciter-Weg),
[resonator-processor-teaching.md](resonator-processor-teaching.md) (der Körper
als Polpaar). Ein Glossar hängt am Ende.

## 1. Das gemeinsame Rezept aller Schaltungs-Referenzen

WDR-8, 8W8, Voltage 80 und Roland ACB bauen dieselbe analoge Kette nach — das
Kick-Signal-Diagramm:

```
Anschlag = TRIGGER-SPANNUNG (keine separate „Velocity-Spur“)
  → Pulse-Shaper (C → 2 R → Klemmdiode): formt den Impuls; seine Höhe und
    sein Spektrum hängen von der Spannung ab
  → Bridged-T-Resonator als Körper, in einer Op-Amp-Rückkopplungsschleife
    (Eigenring des Netzwerks ≈ 80 ms; alles danach trägt die Schleife)
  → Nichtilinearität/Ausgang (Op-Amp-Clip, Damping-Dioden)
```

Die entscheidende Lehre steckt im Wort **Spannung**: die Referenzen kennen
keine „Velocity steuert Gain"-Spur. Der Anschlag ist die Trigger-Spannung
**vor** der Synthese; weil die Schaltung nichtlinear ist (Diode, Op-Amp-Clip),
verändert eine größere Spannung auch Klang und Dauer — Pegel, Attack-Timbre
und Decay wandern zusammen. Velocity ist bei ihnen ein
**Übertragungsgesetz** der Schaltung, kein Lautheits-Knorren danach.

## 2. Die Referenzen im Einzeldurchgang

### 2.1 NTR-808 (FL-Studio-Patcher-Preset) — Klangbastelei, keine Schaltung

Ein `.flp`-Paket („These files are for Fl-Studio only!"), erste Patcher-Arbeit
des Autors, „a lot of trial and error". Kein Beispiel-Code — es sind
FL-Generatoren, die je Stimme synthetisch klingen. Der Bass Drum hat Knobs:
Gain, Tone (= Lowpass-Frequenz), Punch, Decay, **„Detune: BD decay will slide
down in pitch"** (= klassischer Pitch-Drop am Decay-Knopf), Overdrive, Sub
(= Hochpass!), Key-Selector — plus Sidechain-Gewürze (BD duckt unter SD/CP).
Quellen: Handbuch, elektronische Zeichnungen und **Werners Cymbal-Paper** —
aber nur als Lektüre, nicht als Model. Velocity: General-MIDI-Drum-Map über
Port 10; keine Accent-Elektronik. Lehre: das ist unsere „sine-dive-kick"-Klasse
— klingt nach 808, ist aber ein Synth-Kick mit Knobs.

### 2.2 WDR-8 (VCV Rack, WDF) — echte Schaltungsteile, aber (noch) keine BD

`plugin.json` kennt **exakt ein Modul: `WDR8sd`** — die Snare. Die Methodik
ist die härteste der offenen Referenzen: **Wave Digital Filters** — die
Schaltung wird als Netzwerk aus adaptiv rechnenden Bauteil-Modellen aufgebaut
(hier: Env-Generator Q47 als WDF, beide Shell-Resonatoren IC14, Dioden-Clipper
als VCA-Q48). Die Shell-Resonatoren sind **Jatin Chowdhurys
`WaveDigitalFilters/TR_808/SnareResonator`**-Port; der Hochpass (Q49/C66/C67)
wird ehrlich *ersetzt*: „a matching 2-pole Butterworth HPF from ChowDSP is
used". Kalibriert wird gegen **LTspice**: „The programming of this module was
the easiest part, most of the time was spent running electronics simulation".
Lehre: volle Schaltungsnachbildung = Monate; die Energieschnittstelle (WDF-
Wellen statt Samples und Envelopes) ist **deterministisch by design**; und
gemessene *Ersatzstufen* (Butterworth statt Q49) sind ein anerkanntes Werkzeug
— genau unsere Klasse „TPT-Polpaar statt komplettes Netz", nur mit
LTspice-Nachweis statt Ohr.

### 2.3 schwung 8W8 — die stärkste offene Referenz (und Korrektur zu uns)

„Sixteen voices, all synthesised, no samples. Fifteen of them are models of
the 808's actual circuits" — zwei Engines: **Engine A** = Transkription der
Sonic-Pi-`sc808`-SynthDefs (sample-genau gegen SuperCollider geprüft,
„worst null −63,7 dB"), **Engine B** = Circuit Models, von denen neun Voices
einen haben. Klick und Snare stammen aus den **Werner/Abel/Smith-Analysen**
(DAFx-14), Toms/Conga/Clap aus den Service-Notes-Bauteilwerten.

Gemeinsame Schaltungsblöcke, einmal gebaut, in `sc808_circuit_common.h`:
> „the pulse shaper, the op-amp clip, the bridged-T, and now the
> multiple-feedback bandpass."

**Korrektur zu unserem Kernbefund:** der Pulse-Shaper existiert also **nicht
nur in Voltage 80** — 8W8 hat ihn ebenfalls, als Impuls-Exciter vor dem
Körper.

Die Bass Drum (`sc808_bd_circuit.h`) in einem Satz des Projekts:
> „A bridged-T network in an op-amp feedback loop, so Decay is loop gain, not
> an envelope — the network's own ringing is about 80 ms, and everything past
> that is the feedback buffer failing to lose the signal."

Genau diese eine Tatsache erzeugt laut DESIGN.md die vier Verhaltensweisen,
die eine Hüllkurven-Imitation „never quite gets": der Pitch-Sigh, ein echter
Oktav-Sprung zentriert am Attack, kein Maschinengewehr bei schnellen Repeats,
und — Velocity! — „accent as a trigger voltage into a diode rather than a
gain". `tools/bd_check.cpp` hält die Voice an die Paper-Behauptungen
(Werner: VR6-Decay = Loop-Gain, k ∈ [0,001; 1,0]); sein Kommentar benennt die
Failure-Mode, die es existieren lässt:
> „Each one fails if the model quietly stops being a bridged-T in a feedback
> loop and becomes an envelope with a sine on it — which is exactly the
> failure mode this voice exists to avoid, and which would still sound like a
> kick drum."

Der Accent-Test fährt 4 V vs. 14 V Trigger: „a harder trigger is louder" und
„it is not only louder — the harmonic content moves too" — das Timbre-Gesetz
ist **im Test abgeklopft**, nicht behauptet.

### 2.4 Voltage 80 — closed, aber dokumentiert bis zur Bauteilformel

„The behaviour lives in the circuit." Closed Source, Rust-Kern, „There are 0
recorded one-shots in the sound engine. Each hit is synthesized when you play
it" — vollständig deterministische Renders; die Website nutzt sie als Proof.
Die Kick:
- **Pulse-Shaper exakt**: „A capacitor feeds 2 resistors through a clamp
  diode, and a closed-form Wright-omega equation solves that network for each
  sample."
- **Resonator mit Last-Fall**: „Textbook component math puts the kick's
  resonator at 45.7 Hz; the published loading analysis says the surrounding
  resistors pull it up to 49.5 Hz. The model adopts the loaded value, and the
  documented reference unit rings within six cents of it."
- **Accent**: „Accent starts as trigger voltage … On nonlinear circuits, a
  bigger pulse reshapes the sound: the kick diode network changes its click,
  while the tom pitch bend reaches farther" — mit Service-Manual-
  Triggerhöhen-Tabelle je Voice und eigenen Accent-Messungen.

### 2.5 Roland TR-808 Software (ACB) — die verschlossene Mutter

Rolands eigenes Plug-in ist die einzige Kommerzial-Fertigversion: ACB
(„Analog Circuit Behavior") modellieren Schaltverhaltensstufen anhand
Original-Hardware, Schaltplänen und historischer Daten (Quelle:
`808 simulations.txt`). Wie bei Voltage gilt das Trigger-Spannungs-Gesetz aus
dem Service Manual; was drin steckt, ist nicht einsehbar. Nutze es als
Ohr-Referenz, nicht als Lehrbuch.

## 3. Die Kernfrage: wie kommt die Energie vom Anschlag in den Körper?

Alle Referenzen, die eine Bass Drum wirklich „schlagen", machen es **deterministisch**:

| Referenz | Anreger (Exciter) | Körper | Was die Anschlag-Stärke (Velocity/Accent) tut |
|---|---|---|---|
| 8W8 | Pulse-Shaper (Impuls-Exciter aus der Werner-Analyse) | Bridged-T + Multi-Feedback-Bandpass + Op-Amp-Loop | Trigger-**Spannung** in eine Diode: lauter **und** Obertöne wandern (bd_check: 4 V vs. 14 V) |
| Voltage 80 | Pulse-Shaper, Wright-Omega lösst das Netz pro Sample | Bridged-T, geladen 49,5 Hz | Trigger-Spannung (Service-Manual-Tabelle): Diode verändert den Click, Decay-Reichweite wächst |
| WDR-8 | WDF-Triggerstufen (nur Snare!) | WDF-Resonatoren (Chowdhury-Port) | über die WDF-Wellenquelle; VCA = Dioden-Clipper im Modell |
| Roland ACB | closed | closed | Trigger-Spannungsgesetz (Service Manual), nicht einsehbar |
| NTR-808 | Synth-Kick (Patcher-Generatoren) | keiner (Pitch-Drop per „Detune"-Knob) | Knobs; MIDI nur Notenzuordnung — Accent-Gesetz fehlt ganz |
| 808-synth | Sine + Phasen-Sweep + 900-Hz-Click | keiner (Envelope) | accent-boolean ×1,35; **Humanize** (Tune ±0,5 %, Decay ±3 %, Jitter <0,7 ms) |
| driftbox | Sine-Body + Attack ×2,2–4,4 + Noise-Click (HP 1200) | keiner (Envelope) | Peak-Floor 0,62 + 0,38·accent (affin) |
| SmolFM gestern (`06519f9`) | **rohes Rauschen, 8 ms, als Energiequelle** | TPT-Polpaar 49,5 Hz + Loop 0,35→0,22 | roh-linearer Gain auf dem Rauschen |
| SmolFM heute (`fb4d341`) | **Tuned Strike**: Sinus-Impuls (25 ms) auf Korpusnote + leise Noise-Lage | Polpaar 49,5 Hz + Loop 0,22 | vsc0 (sqrt) vor den 3 Accent-Verbrauchern (spread/gain/shape) |

Die unteren drei Zeilen sind die Geschichte. Zitate der beiden Commits:

- `06519f9` — „feat(808): bass drum loop physics + gain base 10.0": Resonator-
  Decay 0,8→0,30 s und Feedback 0,35→0,22 — „Referenz-Physik nach dem 8W8-
  Prinzip ‚Decay is loop gain' — der Körper klingt kurz aus, die Schleife
  trägt den langen Rest. Vorher stieg der Ring ohne Anregung an (Laufprobe
  0,128→0,195 über 2 s)". Der **Exciter war noch Rauschen**.
- `fb4d341` — „feat(808): revoice tr-808 banks": „bass drum: tuned-strike
  excitation (sine impulse into the body, noise now a quiet attack layer) +
  vsc0 (sqrt) ahead of the accent consumers".

**Warum sich Velocity beim Noise-Exciter „random" anfühlte** (bereits
nachgemessen in tuned-strike-exciter-teaching.md): ein 8-ms-Fenster Rauschen
durch ein schmalbandiges Korpus-Filter (Bandbreite ~15–30 Hz) hat das
Zeit-Bandbreite-Produkt B·T ≈ 0,1–0,25 — **unter einem Freiheitsgrad**. Der
Körper „zieht" aus jeder Hit eine andere Stichprobe des Zufalls: die
Ring-Energie streut hit-für-hit um Faktor ~0,3–2, **trotz identischer
Anschlagstärke**. Velocity lief nur als Gain auf dem Rauschen — mehr oder
weniger Würfel statistisch, nie ein geführtes Gesetz. Keine der Referenzen
hat dieses Problem, weil in ihrem Anregungspfad **keine Zufallsquelle als
Energieübertragungskanal** steht: dort überträgt ein definiertes Netzwerk
(Pulse-Shaper) eine definierte Spannung, und Zufall — wenn überhaupt
(808-synth Humanize) — ist Gewürz auf Tune/Decay/Timing, nie der Kanal.

## 4. Was das für SmolFM heißt (die kurze Liste)

- Unsere **Polpaar-Wahl ist die korrekt reduzierte Form** dessen, was Voltage
  80 und 8W8 als Bridged-T-Körper modellieren: der Schwingungsmodus eines
  Bridged-T **ist** ein komplexes Polpaar. Noch kein Referenz-Repo hat
  „nur ein Polpaar" — sie haben Polpaar (bzw. Netzwerk) **plus Loop-Gain
  Physik**, und genau die haben wir mit der externen Loop-Bridge 0,22
  übernommen. Das bleibt.
- Der fehlende Schritt ist der **Impuls-Exciter als Baustein**: Phase 2
  (`strikeLevel`/`strikeTime` im Resonator, default-neutral) macht das
  Tuned-Strike-Prinzip zur Engine-Eigenschaft — Referenzform: die Pulse-
  Shaper-Referenzen. Phase 3 (E = vel²) ist Addition, nicht Ersatz.
  *(03.10.2026: beide gelandet — `strike_vel` trägt die Roh-Spielstärke in
  den Hieb, die Probe misst Ring-RMS-Verhältnis 2.000000 bei vel 1,0/0,5.)*
- Der Rauschen-Click wird **Farbe, nicht Energie** — und braucht dann den
  Hochpass (~1200 Hz) und die 14-ms-Empfehlung der Pulse-Shaper-Messung
  statt 8 ms (driftbox 808.bd-Klick als Vorlage).

## 5. Glossar (die neuen/unverständlichen Worte)

| Begriff | Bedeutung (einfach) | Wer es benutzt |
|---|---|---|
| Trigger-Spannung | der Anschlag ist eine Spannungshöhe, die **vor** der Klangerzeugung in die Schaltung geht | alle Schaltungs-Referenzen; Service-Manual-„trigger height" |
| Pulse-Shaper | C → 2 R → Klemmdiode: formt den Trigger-Impuls zu einem definierten Hub | 8W8 (`sc808_circuit_common.h`), Voltage 80 (Wright-Omega), SmolFM nur angenähert (AdsrImpuls) |
| Wright-Omega | geschlossene Formel, die das Kondensator-/Klemmdiode-Netz **pro Sample exakt** löst | Voltage 80 |
| Eigenring | das Eigenklingeln des Bridged-T-Netzwerks (~80 ms), bevor die Schleife den Rest trägt | 8W8 |
| Loop gain / „Decay is loop gain" | die Decay-Höhe ist die Verstärkung der Rückkopplungsschleife, kein Hüllkurven-Wert | 8W8 (VR6), uns übernommen als Loop-Bridge 0,22 |
| Bridged-T / Multi-Feedback | resonante R-C-Netzwerke in der 808-Op-Amp-Schleife (Körper der BD/Toms/Congas/Claves) | Service Manual, 8W8, Werner-Analysen |
| WDF (Wave Digital Filters) | Schaltnachbau als Netzwerk Wellen-reflektierender Bauteile (adaptoren-basiert) | WDR-8 (Snare), Chowdhury-WDF-Repo |
| Null-Test | Render in Referenz-Engine, Render hier, subtrahieren → −63,7 dB = identisch | 8W8 vs. SuperCollider sc808 |
| bd_check | Verhaltens-Tests, die fehlschlagen, sobald die Voice „envelope with a sine on it" wird | 8W8, `tools/bd_check.cpp` |
| accent diode law | größere Trigger-Spannung verändert Diodenpfad → Pegel UND Spektrum UND Klick wandern | 8W8 (4 V/14 V-Test), Voltage 80, Service Manual |
| Humanize | bewusstes Zufalls-Gewürz auf Tune/Decay/Timing — **nicht** im Energiepfad | 808-synth |
| B·T (Zeit-Bandbreite-Produkt) | <1: das Anregungsfenster ist nur eine Stichprobe des Signals → Streuung; >1: verlässlicher Mittelwert | unsere Noise-Diagnose |
| kohärente Anregung | Anreger auf Körperfrequenz → voller, berechenbarer Energietransfer | unser Tuned Strike |

## 6. Belegliste

- **Eigene Git-Historie:** `06519f9657db0bd01d23b84b0d19c60561b21c6f`
  („bass drum loop physics + gain base 10.0" — decay 0,8→0,30, fb 0,35→0,22,
  Selbstanschwingen 0,128→0,195 beseitigt, Noise-Exciter noch aktiv);
  `fb4d341d332780de54b1f2050a5bd907de434e46` („revoice tr-808 banks" —
  tuned-strike excitation, noise als Attack-Lage, vsc0 sqrt).
  Patch: `instruments/tr-808/tr808-bass-drum.smolfm` (49,5 Hz, q 0,85,
  decay 0,30, spread 2,5, shape soft 0,25, gain 10,0, output 0,9).
- **NTR-808:** github.com/NielsTriple/NTR-808, README (FL-only, Patcher,
  BD-Knobs Gain/Tone/Punch/Decay/Detune/Overdr./Sub/Key, Sidechain,
  GM-Drum-Map), 13 Commits, 2 Sterne.
- **WDR-8:** github.com/Simon-L/WDR-8-rack (GPL-3.0) — README-Modelltext
  (Q47-Env als WDF, IC14-Resonatoren aus
  jatinchowdhury18/WaveDigitalFilters `TR_808/SnareResonator`, Dioden-Clipper
  Q48, ChowDSP-Butterworth-Ersatz, LTspice-Kalibrierung) +
  `plugin.json` (**nur Modul WDR8sd** — keine BD), 19 Commits, 35 Sterne.
- **schwung 8W8:** github.com/athousanddetails/schwung-8W8 (GPL-3.0) —
  README, `docs/DESIGN.md` (zwei Engines, gemeinsame Blöcke „the pulse
  shaper, the op-amp clip, the bridged-T, and now the multiple-feedback
  bandpass", BD-Zitate, 4-V/14-V-Accent-Test), `tools/bd_check.cpp` (Tests
  gegen Werner DAFx-14-Behauptungen), 0 Sterne, 67 Commits.
- **Voltage 80:** voltagedrums.com — „The behaviour lives in the circuit",
  Pulse-Shaper/Wright-Omega, 45,7→49,5 Hz („rings within six cents"),
  „Accent starts as trigger voltage", 0 recorded one-shots; closed Source.
- **Roland TR-808 Software:** roland.com/de/products/rc_tr-808 (ACB,
  „anhand von Original-Hardware, Schaltplänen und historischen Daten") —
  zitiert nach `808 simulations.txt`.
- **Lokal verifizierte Parametric-Repos:**
  `doc/research/GitHub-808-Simulations-Analysis.md` (mic2016z/808-synth,
  emmettl/driftbox — Voice-Tabellen §2, Velocity-Domänen §8) +
  Schnappschüsse in `doc/research/code-reference/`.
- **Unsere Diagnose:** `doc/teaching/tuned-strike-exciter-teaching.md`
  (B·T-Streuung, Faktor 0,3–2), `doc/teaching/resonator-processor-teaching.md`
  (Polpaar/TPT), `doc/research/resonator-simplification idea.md`
  (Phasen 2–4, Urteil: 2 umsetzen, 3 Addition, 4 abgelehnt).