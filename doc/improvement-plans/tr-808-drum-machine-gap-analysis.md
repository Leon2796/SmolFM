# TR-808 vs. SmolFM — Lückenanalyse und 808-Patch-Report

> Referenz: `doc/research/Origin of 808 Drum Machine Design.md` (System-700-
> Entstehung, Kikumoto-Berichte) und `doc/research/Werner-Abel-Smith-808-approach.txt`
> (Service-Manual-Auflösung aller Voices, Werner/Abel/Smith-DAFx-Modelle für BD,
> Cymbal und Cowbell), `doc/formats/smolfm.xsd` (Graph v3 mit `velocity`-Port),
> `doc/processors/*.md`, `doc/techniques/*.yaml` (808-accent-*, Drum-Familien),
> `instruments/tr-808/*.smolfm`, `doc/improvement-plans/famous-synth-algorithms-gap-analysis.md`
> (§5 P1.3) und `doc/improvement-plans/stereo-effects-support-gap-analysis.md`.

Dieser Plan ist **zweigeteilt**:

**Teil A (Abschnitte 1–3):** Die beiden 808-Research-Dokumente liefern die
Referenzachse — anders als in der Synthesizer-Lückenanalyse nicht berühmte
Synths, sondern die **Hardware-Prinzipien der 808-Voices**: Bridged-T-Resonatoren,
die sechsstimmige Pulsoszillator-Bank, die Multi-Burst-Anregung des Claps und
die zentrale **Accent-Elektronik**. Pro Prinzip steht, ob der heutige SmolFM-Graph
es **ausdrücken kann**, nur **teilweise** oder **gar nicht** — mit dem konkreten
Processor-/XSD-Ansatz, der die Lücke schließen würde.

**Teil B (Abschnitt 4):** Die 808-Patches sind bereits verbessert worden (Accent-
Verdrahtung statt zentraler Velocity-Skalierung, Adder-Bus, Service-Notes-Bank).
Hier steht pro Voice, was implementiert ist, welche Technik es dokumentiert und
welche klangliche Differenz zur Schaltung offen bleibt; die offenen Punkte
fließen in die priorisierte Liste (Abschnitt 5) ein.

---

## 1. Fähigkeitsmodell von SmolFM (heute, nach der Accent-Erweiterung)

**Graph:** DAG (keine Zyklen), genau ein `note`-Knoten, genau ein `output`.
**Port-Domänen:** `frequency`, `signal` und neu **`velocity`** (0–1, ein Wert
pro Note). Monophon, 12-TET; Delay-Zeiten und statische Parameter bleiben
während einer Note unveränderlich.

| Knoten | Max. | Ports (Domäne) | Statische Parameter |
|---|---|---|---|
| `note` | 1 | out: frequency, **vel (velocity)** | 12-TET-Skala |
| `osc` | 16 | in: note_in (frequency); out: signal | pitch/static/lfo, 7 Wellenformen |
| `fm` | 8 | in: note_in, modulator_in, **vel**, amount_mod_in → out: frequency | index 0…10 |
| `fscale` | 16 | in/out: frequency | Faktor 0…10 |
| `fadsr` | 8 | in: note_in, **vel** → out: frequency | ADSR + up/down (Vel = Dive-Tiefe, Pull-to-Neutral) |
| `gain` | 16 | in: signal, **vel** → out: signal | Faktor 0…10 |
| `adsr` | 16 | in/out: signal | ADSR; **kein vel-Port, kein Zeit-Modulport** |
| `shape` | 16 | in: signal, **vel**, drive_mod_in → out: signal | drive 0…1, soft/hard/fold |
| `filter` | 16 | in: signal, **vel**, cutoff_mod_in, resonance_mod_in → out | TPT-SVF, 20–20000 Hz, Reso 0…1 |
| `delay` | 16 | in/out: signal | 1–2000 ms, Feedback ≤ 0,95, Mix, Sync |
| `ring` | 8 | in1/in2 → out: signal | y = a·b |
| `am` | 8 | carrier/mod → out: signal | depth 0…1 |
| `add` | 8 | in1/in2 → out: signal | reine Summierung, kanalweise |
| `split`/`merge` | je 8 | Kanal-Routing | ohne Summierung |
| `output` | 1 | in1…in8 (signal) | Master-Level |

Damit sind aus der Synthesizer-Lückenanalyse **erledigt**: **§5 P1.3** (Velocity-
Port: `gain.vel`, `shape.vel`, `fm.vel`, `filter.vel`, `fadsr.vel`) und
**§5 P1.1 teilweise** (der reine `add`-Bus existiert; der modulierbare
`mix`-/Crossfade-Knoten bleibt offen). Drumspezifisch bleibt dagegen:

1. **Keine Zyklen** → Schaltungs-Rückkopplungen (Multi-Feedback-Bridged-T,
   Dioden-Pfade) sind nicht modellierbar.
2. **Kein Impuls-Exciter** → Anregung nur als Wellenform hinter Envelope,
   „Impuls in Resonator" erscheint als gefiltertes, gehülltes Rauschen/Sinus.
3. **Envelope-*Zeiten* lassen sich nicht an Velocity binden** — ein Accent,
   der Dekay-Zeiten verändert, ist nicht ausdrückbar.
4. **Kein Puls-Modus** (nur Square-Näherung) und keine geteilte Oszillatorbank:
   die 6er-Bank muss pro Voice dupliziert werden.
5. **Ein `output`, intern mono** → Kit-Breite und Versätze laufen ins Leere
   (vgl. Stereo-Lückenanalyse Abschnitt 1).

---

## 2. Teil A — Hardware-Prinzipien der 808 im Detail

### A1 — Bridged-T-Resonantorkörper (BD, Toms, Congas, Snare-Bodies, Claves, Rimshot)

Service Manual und die Werner/Abel/Smith-Analyse lesen diese Voices als
**angeregte resonante Netzwerke**: ein Impuls kippt in ein Bridged-T-Netzwerk
und das Netzwerk antwortet abklingend pseudo-sinusförmig (BD: Multi-Feedback-
Bridged-T mit Rückkopplung/Damping — Resonanz ≈ 49,5 Hz gemessen gegen die
≈ 56 Hz Tuning-Angabe; Snare: zwei Resonatoren als 185/330-Hz-Körperpaar
neben dem Rauschband).

**Ausdrückbar (approximiert):** SmolFM ersetzt die Schaltungsantwort durch die
Envelope-Antwort — `sine-dive-kick` peakt die F-ADSR hoch und divet zurück
(Beater-Klick = Attackspitze, Korpus = Dive), `keyed-dual-resonator-drum` baut
das 185/330-Paar als zwei key-getrackte `fscale`-Körper nach,
`tuned-noise-percussion` ringt einen Hoch-Q-Bandpass. Werner/Abel/Smith
betonen ausdrücklich, dass der BD-Pitch-Drop **kein** nachgeschalteter
Pitch-Envelope ist, sondern die Reaktion der Schaltung — genau diese Ebene
bleibt bei der Envelope-Approximation weg: Einschwing- und Nachschwingverhalten
der Resonanz.

**XSD-Ansatz:** `resonator`-Knoten (in: signal; tune, Q, Dekay; optional Key-
Tracking; vel auf Dekay) oder Hoch-Q-Dekay-Modus am `filter` — der Knoten
erzeugt die Schwingantwort selbst, statt die Antwort mit Envelopes und
Wellenformen zu approximieren.

**Status:** implementiert als `ResonatorProcessor` (in: `in`/Exciter, `tune`
als Frequenz-Override-Port — Key-Tracking bleibt patchbar über
note → fscale → tune —, `q_mod_in`, `vel` auf Dekay; Parameter: tune, q,
decay/t60, spread nach dem Pull-to-Neutral-Muster). Die zweite Variante aus
der Diskussion (eigener `body`/`ring`-Doppelausgang bzw. Hoch-Q-Dekay-Modus
am `filter`) bleibt ausdrücklich offen.

### A2 — Accent-Elektronik (Loudness, Timbre, Zeitkonstanten)

Die 808-Accent-Spur ist kein Velocity-Fader, sondern ein eigener Trigger: Sie
öffnet die Loudness (VCA), drückt die Analogstufen tiefer ins Übersteuern
(Timbre — heller, aggressiver) und **verändert Zeitkonstanten**: Beim
BD-Resonator wächst die Dekay mit dem Accent (Service Manual: „Decay über
VIN; Accent verändert die Zeitkonstante" über Q41–Q43).

**Ausdrückbar (Loudness/Timbre — erledigt):** `note.vel` als Signalfluss-Draht:
`gain.vel`, `shape.vel`, `filter.vel`, `fm.vel`, `fadsr.vel` — dokumentiert in
`808-accent-gain`, `808-accent-drive-shaper`, `808-accent-filter-snap`,
`808-accent-fm-brightness` und schichtweise verdrahtet in allen 16 Patches
(Abschnitt 4). Welchen Layers der Accent erreicht, entscheidet die Verdrahtung,
nicht eine zentrale Voice-Skalierung.

**Nicht ausdrückbar (Zeiten):** Kein vel-Eingang skaliert Envelope-Zeiten —
akzentuierte Schläge klingen lauter/heller, aber nie *länger*. Und die bereits
geplante ADSR-Gain-Anker-Stufe (`gain_up` am Attack-Peak, `gain_down` am
Sustain; Synthesizer-Lückenanalyse §5 P1.3, Stufe 2) ist weiterhin unbestückt.

**XSD-Ansatz:** Zeit-Anker am `adsr`/`fadsr` nach dem eingeführten
Pull-to-Neutral-Muster: `t_eff = t · (1 + v·(spread − 1))`, Spread 0,25…4,
byteidentisches Verhalten ohne Verdrahtung — derselbe Sprachstil wie
`fadsr.vel`.

### A3 — Pulsoszillator-Bank (Cowbell „2 aus 6", CH/OH, Cymbal)

Cowbell, Hi-Hats und Cymbal speisen sich aus einer **sechs-Puls-Oszillator-Bank**
(205,3 / 304,4 / 369,6 / 522,7 / 540 / 800 Hz — Service-Notes-Bank); das Cowbell
aktiviert zwei dieser Oszillatoren, Hats und Cymbal alle sechs; dahinter liegen
Bandpässe und (beim Cymbal) eine mehrstufige Filter-/Envelope-Struktur.

**Ausdrückbar (approximiert):** Statische Squares in exakt den Bankfrequenzen
(`metal-hat-squares`), auf dem `add`-Bus gefaltet (`osc-sum-bus`) und durch
eine Bandpass-Kette geschleust — alle Metal-Voices in `instruments/tr-808`
arbeiten so, das Cymbal sogar mit der vollen Bank. Nicht ausdrückbar: die
echte Pulswellenform (Squares approximieren), die „2 aus 6"-Selektion
(patch-fest eingezeichnet) und eine **geteilte Bank** — der monophone,
zyklenfreie Graph dupliziert die sechs Oszillatoren pro Voice und verbraucht
dafür ein Drittel des osc-Budgets je Patch.

**XSD-Ansatz:** `pulse`-Wellenform bzw. Duty-Modus am `osc` (0…1) — kleine,
isolierte Änderung (P2). Eine bankgeteilte Kit-Sicht brauchte Voice-Gruppen
und liegt auf der Host-Ebene (P3; vgl. Polyphonie, Synthesizer-Lückenanalyse
§5 P3.10).

### A4 — Multi-Burst-Envelope (Hand-Clap)

Der 808-Clap braucht keine Echo-Geometrie: sein **spezieller Multi-Burst-Envelope
zündet den Noise-Hit mehrfach hintereinander** (~11-ms-Raster mit kürzer
werdenden Intervallen); die Einzelschläge summieren sich im VCA.

**Ausdrückbar (approximiert):** `multi-burst-clap` komponiert das Retriggern über
zwei serielle Delays, deren Taps innerhalb einer 45-ms-One-Shot-Envelope liegen.
Klangliche Differenz: Ein echter Envelope-Retrigger sammelt **jedes Mal neues
Rauschen**; Delays wiederholen **dieselbe** Rauschsammlung — die Taps klingen
strukturell gleich und wirken „kopiert" statt „geklatscht".

**XSD-Ansatz:** `burst`-Retrigger-Modus am `adsr` (Anzahl, Rasterzeit, kürzende
Intervallmuster) — knotenlokal, keine Strukturänderung am Graph (P2).

### A5 — Mehrstufige Filter-/Envelope-Struktur (Cymbal-Kette)

Das Cymbal ist die komplexeste Voice: die Oszillatorbank durchläuft eine
**mehrstufige Filter- und Envelope-Struktur** — das Spektrum wandert zeitlich
durch die Dekay-Kette.

**Ausdrückbar (baugleiche Familie):** `band-envelope-choreography` (eine
Rauschquelle → Bandpässe → ADSR-Gruppe mit Dekay-Treppe) reproduziert die
zeitliche Spektrumsmigration ohne bewegte Cutoffs; die Cymbal-Patches nutzen
bereits eine Wash-Envelope-Stufe. Die exakte Schaltungsstufung ist keine 1:1-
Abbildung, aber auf Klangwirkungsebene deckt die Dekay-Treppe denselben Effekt
ab. P3-Feinschliff, sobald ein `resonator` existiert (A1).

### A6 — Digitale Seite: Common Trigger und Instrument Select

Die Hardware trennt digital (Pattern → Common Trigger → Instrument Select →
AND-Gates) von analog (Sound Generators). Auf SmolFM übersetzt: Ein **Kit** als
ein ausdrückbares Objekt — mehrere Voices hinter Key-Zuordnung, gemeinsamer
Trigger, gemeinsame Accent-Logik. Heute lebt das Kit als 16 Einzelpatches;
der Graph kennt einen `note`-Knoten und kein Key-Mapping.

**Nicht ausdrückbar (Graph-Ebene):** Kit-Datei (Sound → Taste, Mute/Balance).
**XSD-Ansatz:** Host-/Preset-Ebene, kein Graph-Änderungsbedarf (P3).

### A7 — Schaltungscharakter: Rückkopplung, Nichtlinearität, Kikumoto-Vereinfachung

Beide Research-Dokumente machen denselben Punkt: Die 808 ist kein System-700 in
klein — Kikumoto beschreibt, dass jede Voice „ungefähr die Synthesizer-Möglich-
keiten eines System-700 bekommen sollte" und das für Kosten und Baugröße auf
Zweckschaltungen gekürzt wurde (BD-Netzwerk mit Rückkopplung statt klassischer
VCO→VCF→VCA-Kette). Genau diese Rückkopplungs- und Nichtlinearitätsstrukturen
kann der zyklenfreie Graph nicht ausdrücken; `shape.drive` (Accent-verdrahtet,
`808-accent-drive-shaper`) bleibt globale Annäherung.

**XSD-Ansatz:** begrenzter `feedback`-Knoten (P3; vgl. Synthesizer-Lückenanalyse
§5 P3.9) — bewusst spät, aber erst er macht die Werner/Abel/Smith-Modelle 1:1
ausdrückbar.

**Status:** implementiert als `FeedbackProcessor` (`out = in + amount·fb_in`;
jeder Graph-Zyklus trägt implizit 1 Sample Verzögerung über die
Render-Reihenfolge; amount 0…0.95 begrenzt pro Durchlauf).

---

## 3. Teil A — Übersichtstabelle

Je Zeile ein Hardware-Prinzip mit Recherche-Beleg, heutigem SmolFM-Status und
konkretem Processor-Vorschlag (P1 = höchste Priorität; „erledigt" markiert
Lieferung).

| Hardware-Prinzip | 808-Beleg (Recherche) | SmolFM heute | Processor-Vorschlag | Klanglücke ohne Änderung |
|---|---|---|---|---|
| Bridged-T-Resonatoren | DAFx-2014-Modelle, Service Manual, 49,5-vs-56-Hz-Detail | Envelope-Approximation (Dive, Doppelkörper, Q-Ring) | **P2**: `resonator`/Hoch-Q-Dekay | Resonanz-Ring und Einschwingverhalten fehlen |
| Accent — Loudness/Timbre | Analogstufen drücken tiefer ins Übersteuern | **erledigt**: `note.vel` + 4 accent-Techniken | — | — |
| Accent — Zeitekonstanten | BD-Dekay über VIN (Q41–Q43) | fehlt: kein vel-Eingang auf ADSR-Zeiten | **P1**: Zeit-Anker (Spread 0,25…4) + ADSR-Gain-Anker (Stufe 2) | Akzente klingen anders — aber gleich lang |
| Puls-Oszillator-Bank | 6er-Bank 205,3–800 Hz; Cowbell „2 aus 6" | Squares + `add`-Bus + BP, Bank je Voice | **P2**: `pulse`-Modus am `osc` | Square statt Puls; sechsfache Duplizierung |
| Multi-Burst-Envelope | Clap ~11-ms-Raster | Delays als Tap-Geometrie | **P2**: `burst`-Modus am `adsr` | Taps wiederholen dieselbe Rauschsammlung |
| Mehrstufige Filter-/Env-Kette | Cymbal mehrstufige Struktur | `band-envelope-choreography` (Dekay-Treppe) | P3-Feinschliff (via `resonator`) | gering — Klangwirkung deckt sich |
| Kit / Instrument-Select | Common Trigger + AND-Gates | 16 Einzelpatches, je ein `note` | **P3**: Kit-Preset auf Host-Ebene | Kein Kit als ausdrückbares Objekt |
| Schaltungs-Feedback | Rückkopplung/Damping im BD-Netzwerk | nicht ausdrückbar (zyklenfrei) | **P3**: begrenzter `feedback`-Knoten | Circuit-Charakter bleibt approximativ |

---

## 4. Teil B — 808-Patch-Report: Was verändert wurde (16 Voices)

Global verändert (lesbar am Technik-Katalog und den Patch-Attributen):

1. **Accent wurde zum Signalfluss-Draht.** Die zentrale Velocity-Skalierung der
   Stimme ist entfernt; `note.vel` (Port-Typ `velocity`, 0–1) liefert die
   Anschlagstärke, und jede Schicht entscheidet per Verdrahtung, ob sie
   mitgeht (`gain.vel`, `shape.vel`, `filter.vel`, `fm.vel`, `fadsr.vel`).
   Unverdrahtete Pins lesen neutral (1,0) — Bestandspatches klingen
   byteidentisch weiter.
2. **Vier Techniken dokumentieren die Accent-Hälften** (`808-accent-gain`,
   `808-accent-drive-shaper`, `808-accent-filter-snap`,
   `808-accent-fm-brightness`): Loudness, Drive, Cutoff-Snap, FM-Brightness —
   zusammen die Loudness/Timbre-Hälfte des 808-Accents. Die Zeiten-Hälfte
   fehlt (Teil A, A2/P1).
3. **Die Voices waren auf die Schaltungsrezepte umgezeichnet:** die
   Service-Notes-Squares-Bank (205,3–800 Hz) für alle Metal-Voices, das
   185/330-Paar als `fscale`-Intervall im Snare, gemeinsame Adder-Busspur vor
   je einem Bandpass (`osc-sum-bus` statt eines Filters je Kette).

Stand je Voice (Kernrezept + Accent-Verdrahtung):

| Voice | Techniken | Kernrezept | Accent-Verdrahtung |
|---|---|---|---|
| bass-drum | sine-dive-kick | Sinus-Korpus über F-ADSR-Dive, langer Boom (A1–B1) | fadsr.vel (Dive tiefer) + gain.vel (lauter) |
| snare | tuned-noise-percussion | 185/330-Hz-Zweiton-Korpus + 2,6-kHz-Rauschband als dominante Schicht | gain.vel auf allen drei Schichten |
| hand-clap | multi-burst-clap | Rausch-Band 1,05 kHz, Taps im 14/31-ms-Raster (Tap-1 ohne Feedback), Envelope nach den Taps + diffuser 200-ms-Tail | gain.vel |
| claves | tuned-noise-percussion | 2,5-kHz-Sinus-Ping (120 ms) + Rausch-Tick als Holz-Attack | gain.vel |
| conga-low/mid/high | sine-dive-kick | Sinus-Korpus, Dive up 1,25, kompakte Dekay (G3/C4/E4) | fadsr.vel (Dive tiefer) + gain.vel |
| tom-low/mid/high | sine-dive-kick | Dive-Attack-Korpus (98/123/165 Hz) | fadsr.vel + gain.vel |
| cowbell | osc-sum-bus | 540/800-Hz-Squares → Adder → 2,5-kHz-BP (320 ms) | filter.vel (heller) + gain.vel (lauter) |
| rimshot | osc-sum-bus | 335/480-Hz-Squares → 1,4-kHz-BP (55 ms) | filter.vel + gain.vel |
| hihat-closed | pulse-oscillator-bank, osc-sum-bus, 808-accent-gain | 6er-Square-Bank 205,3–800 Hz → 10-kHz-Bandpass, 50-ms-Tick (Pulse-Approximation, Phase-Lock bewusst akzeptiert) | gain.vel (Band-Centrum statisch) |
| hihat-open | pulse-oscillator-bank, osc-sum-bus, 808-accent-gain | gleiche Bank → 10-kHz-Bandpass, 450-ms-Dekay (eine andere Envelope) | gain.vel (Band-Centrum statisch) |
| cymbal | pulse-oscillator-bank, osc-sum-bus, band-envelope-choreography, 808-accent-gain | 6er-Square-Bank → BP 5,2/9,4 kHz, Dekay-Treppe 0,35/0,65/1,25 s, gemeinsame HP-Tone-Stufe | gain.vel (Band-Centrum statisch) |
| maracas | tuned-noise-percussion | 4,8-kHz-Bandpass, 130 ms | filter.vel + gain.vel |

Offene klangliche Baustellen — je Voice der direkte Verweis in Teil A:

- **BD, Toms, Congas:** Envelope-Dive statt Resonanzring → `resonator` (A1, P2).
  *(Umgesetzt: bass-drum als Rauschklick in den feedback-umwickelten Körper
  (Loop-Form, A7-Damping), tom-low/mid/high + conga-low/mid/high direkt mit
  Quadrat-Puls-Anreger, key-getrackt über `resonator.tune`, Dekay-Akzent über
  `resonator.vel` (spread 2.5/2.0/1.5). Technik `resonator-body-strike` im
  Katalog ergänzt.)*
- **Alle Voices:** Accent verändert nicht die Hit-Länge → Zeit-Anker (A2, P1);
  am BD am deutlichsten (Original: Dekay wächst mit Accent), dann bei den Toms.
- **Hand-Clap:** Delay-Taps wiederholen dieselbe Rauschsammlung → `burst`-Modus
  (A4, P2). *(Revidiert 2026-10-01: Tap-1 ohne Feedback, Envelope hinter die
  Taps, Band auf 1,05 kHz aufgeweitet — das fb-0,8-Kammfilter-Stottern und der
  gepitchte Ring-Tail sind beseitigt; der `burst`-Modus als echte
  Neuauslösung bleibt offen.)*
- **Metal-Voices:** Squares statt Pulswellen → `pulse`-Modus (A3, P2);
  bankgeteilt über das Kit nur auf Host-Ebene (A6, P3). *(Revidiert
  2026-10-01: hihat open/closed + cymbal auf `noise-metal-wash` umgestellt —
  die deterministische 6er-Square-Bank klang phase-lock-tonal: statische
  Phasen pro Note, kein Drift, und der Vel-Multiplikator am Cutoff zog den
  10-kHz-Bandpass die Harmonik-Leiter hinunter ins tonale Band. Squares
  bleiben in cowbell/rimshot bis `pulse`.)* *(Revidiert 2026-10-02: hihat
  open/closed + cymbal bewusst auf die Pulsbank zurückgebaut — Squares als
  getreue Pulse-Approximation (A3 „Ausdrückbar (approximiert)"), 10-kHz-Bandpass
  bzw. Dekay-Treppe am Cymbal, vel nur an gains. Die Phase-Lock-Farbe bleibt
  als dokumentierter Kompromiss bis zum `pulse`-Modus (P2.4); Technik
  `pulse-oscillator-bank` im Katalog ergänzt.)*
- **Accent-Verdrahtung `filter.vel`:** der Port multipliziert den Cutoff
  (`fc × vel`, FilterProcessor.cpp:66–70) — Akzent verdunkelt statt erhellt,
  `vel → 0` tötet das Band (Cutoff bei 20 Hz). Betrifft weiterhin
  cowbell/rimshot/maracas; hihat/cymbal sind auf `gain.vel`/`resonator.vel`
  umverdrahtet. → P1-Kandidat: Hell-Akzent als `fc × (1 + v·spread)`,
  Pull-to-neutral.
- **Cymbal:** mehrstufige Struktur über die 6er-Square-Bank, drei
  Band-Gate-Stufen (Dekay-Treppe 0,35/0,65/1,25 s) und gemeinsame HP-Tone-Stufe
  gedeckt (A5; schaltungsgetreu seit 2026-10-02) — Feinschliff nach Hören offen.
- **Breite/Versatz im Kit:** intern mono, ein `output` — vgl. Stereo-Lücken-
  analyse Abschnitt 3 (dort P1: Kanal-Dimension/Panner).

---

## 5. Priorisierte Verbesserungen (aus Teil A)

P1 — schließt die fehlende Accent-Hälfte, kleine Engine-Änderung:

1. **Zeit-Anker an Velocity** (`decay_vel` bzw. Zeit-Spread-Port an `adsr` und
   `fadsr`, Pull-to-Neutral, Spread 0,25…4, byteidentisch ohne Verdrahtung):
   akzentuierte Schläge dürfen — wie auf der Hardware — länger oder knapper
   klingen. Ergänzt die erledigte Loudness/Timbre-Hälfte (Synthesizer-
   Lückenanalyse §5 P1.3) und schafft die Grundlage für die dort offene
   Stufe 2 (`gain_up`/`gain_down` am `adsr`).

P2 — Annäherung an die Schaltungsrezepte:

2. **`resonator`-Knoten** oder Hoch-Q-Dekay am `filter` (Exciter-Eingang, tune,
   Q, Dekay; vel auf Dekay; optional Key-Tracking): Resonanzring für
   BD/Toms/Congas/Snare-Bodies statt Envelope-Approximation.
   *(Implementiert: `resonator`-Prozessor, siehe Status unter A1; die
   zweite Varianten-Diskussion bleibt offen.)*
3. **`burst`-Retrigger-Modus am `adsr`** (Anzahl, Rasterzeit, kürzende
   Intervalle): echter Clap-Multi-Burst statt Delay-Tap-Geometrie.
4. **`pulse`-Wellenform bzw. Duty-Modus am `osc`** (0…1): echte Pulsbank für
   Cowbell/Hats/Cymbal statt Square-Näherung.

P3 — Architektur- und Host-Ebene, bewusst spät:

5. **Kit-Ebene** (Instrument-Select-Nachbau): Preset mit Voice-Liste, Key-Map,
   gemeinsamem Trigger — Host-Aufgabe, keine Graph-Änderung.
6. **Begrenzter `feedback`-Knoten** (vgl. Synthesizer-Lückenanalyse §5 P3.9):
   Schaltungs-Rückkopplungen modellierbar, Werner/Abel/Smith-Modelle 1:1.
   *(Implementiert: `feedback`-Prozessor, siehe Status unter A7.)*
7. **Cymbal-Feinschliff** (zweite `resonator`-/VCA-Stufung) nach P2.2;
   Kit-Breite folgt der Stereo-Lückenanalyse (dort §3 P1/P2).

Querverweise: Velocity (Loudness, Drive, Index, Cutoff, Dive-Tiefe) ist aus der
Synthesizer-Lückenanalyse §5 P1.3 **erledigt**; der reine `add`-Knoten deckt
dort §5 P1.1 teilweise ab (Summieren ohne Crossfade — der modulierbare
`mix`-Knoten bleibt offen); die Mono-Grenze des Kits behandelt die Stereo-
Lückenanalyse (Abschnitte 1 und 3).