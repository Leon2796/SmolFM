# House Soundpack — Ideas & Requirements (nicht umsetzbar mit SmolFM 2)

Dieses Dokument sammelt Sound-Design-Ideen, die beim Erstellen des House-Packs
entstanden sind, aber mit den aktuellen SmolFM-Processoren **nicht** umsetzbar
waren. Sie sind konkrete Anforderungen an zukünftige Prozessor-Typen oder
Dateiformat-Erweiterungen.

---

## 1. Filter / State-Variable Filter (höchste Priorität)

**Gewünscht:** Multi-Mode-Filter (LP/BP/HP/Notch) mit Cutoff + Resonance +
optional Modulationseingang.

**Wo es fehlte:**
- `house-bass-deep` / `house-bass-rolling`: Der Growl-Bass soll auf dem Attack
  aufschließen (cutoff sweep von 200 Hz → 2 kHz über 50 ms). Aktuell nur über
  FM-Amount nachzubauen — klingt nach "FM-Ish", nicht nach "Filter-Ish".
- `house-pad-deepy` / `house-pad-shimmer`: Pads sollen dunkel anfangen und mit
  LFO aufschließen (auto-wah). Ohne Filter nur via FM/Sättigung simuliert —
  zu harsch.
- `house-fx-riser-noise`: Klassischer Riser = Noise → LP-Filter mit steigendem
  Cutoff. Aktuell über Pitch-Envelope (4x) approximiert — klingt anders.
- **Anforderung:** `filter`-Node: `in (signal) → out (signal)`, Parameter
  `cutoff` (20-20000 Hz, log), `resonance` (0-1), `mode` (0=LP,1=BP,2=HP,3=Notch),
  optional `cutoff_mod_in (frequenz)` für Envelope/LFO-Steuerung.

## 2. Feedback Loop im Graph

**Gewünscht:** Oszillator → Delay → zurück in denselben Oszillator (für
Karplus-Strong-artige Pluck-Sounds und dub-techno Feedback-Tails).

**Wo es fehlte:**
- Karplus-Strong Gitarren-/Marimba-Plucks (klassische House-Percussion).
- Selbstoszillierende Dub-Stabs mit wachsendem Feedback.
- **Blocker:** `SmolFmFile::load` + `applyConnectionPatch` vertragen Zyklen
  nicht sicher (Reihenfolge im `SignalGraph` = Insertion-Reihenfolge); ein
  Loop bräuchte einen Feedback-Delay-Node mit 1-Sample-Latenz.
- **Anforderung:** Entweder Topologie-Sortierung im `SignalGraph` oder ein
  `feedback`-Node (`in → out` mit 1-Sample Delay) für explizite Loops.

## 3. LFO → beliebiges Parameter-Ziel (Modulation Matrix)

**Gewünscht:** LFO-Ausgang auf Filter-Cutoff, FM-Amount, Delay-Feedback etc.
routen.

**Wo es fehlte:**
- `house-pad-shimmer`: Brightness-Waves nur via AM auf Amplitude umsetzbar.
  Gewünscht: LFO → Filter-Cutoff (wäre mit Nr. 1 kombiniert perfekt).
- `house-fx-riser-noise`: Delay-Feedback soll mit dem Riser steigen. Aktuell
  statisch.
- **Anforderung:** Modulation-Routing: Jeder `signal`-Ausgang soll auf
  *Parameter* (nicht nur Ports) modulieren können. Z. B.
  `<Mod from="osc2" fromPort="out" to="filter0" toParam="cutoff" depth="0.5"/>`
  im XML. Implementierung: Interpolierter Parameter-Lookup pro Sample statt
  atomar (oder Lock-freie Mod-Ringe pro Parameter).

## 4. Per-Note Mikro-Timing / Humanize

**Gewünscht:** Clap-Flam mit zufälligem Timing (±3 ms jitter) statt exakter
staggerter ADSRs.

**Wo es fehlte:** `house-clap-classic` nutzt drei ADSRs mit fixen Offsets —
klingt mechanisch. Echte Claps variieren pro Hit.
- **Anforderung:** Random-Offset-Parameter pro ADSR (`attackJitter` in ms) oder
  ein `humanize`-Node mit Seed-basiertem Per-Note-Jitter.

## 5. Pitch-Bend / Glide zwischen Noten

**Gewünscht:** Legato-Glide (Portamento) für 90s House-Leads und 808-Slides.

**Wo es fehlte:** `FAdsrProcessor` arbeitet nur per-Note (Phase-Resync bei
Note-On killt Legato). Ein Lead-Slide von Note A zu Note B braucht den
Frequenz-Übergang über ~80 ms.
- **Anforderung:** `glide`-Parameter am Oscillator (Static/Pitch-Mode) oder
  ein `portamento`-Node in der Frequenz-Domäne.

## 6. Mehr Output-Kanäle / Stereo-Width

**Gewünscht:** Echter Stereo-Output (L/R-Busse) für Ping-Pong-Delays und
Stereo-Pads.

**Wo es fehlte:** `MasterOutputProcessor` ist mono (8 Inputs, ein Summen-Signal).
Ping-Pong-Delays (`house-lead-pluck`) laufen in einen gemeinsamen Summen-Input —
der Stereo-Image-Effekt geht verloren.
- **Anforderung:** Zwei MasterOutputs (L/R) oder ein `pan`-Node
  (`in → outL/outR`) mit Position-Parameter; Output-Konzept auf Stereo erweitern.

## 7. Sample-basierte Transienten

**Gewünscht:** Echte Clap-Transienten (Vorstufe aus mehreren Mikro-Klicks mit
rauschmoduliertem Timing).

**Wo es fehlte:** `house-clap-classic` ist FM/Noise-basiert — gut, aber nicht
100 % identisch zu einem gesampelten Clap.
- **Anforderung:** Optionaler Sample-Player-Node (eine WAV pro Instanz, im
  `.smolfm` referenziert) für Transienten, die FM nicht ersetzen kann.

## 8. LFO-Rate im Sync-Modus (BPM-Locked)

**Gewünscht:** LFO-Rate als Noten-Division (1/8, 1/16, 1/8T) statt Hz — für
tempo-stabile Gates.

**Wo es fehlte:** `house-hihat-open` und Gates nutzen fixe Hz-Raten; bei
Tempo-Änderung im Host verschiebt sich der Puls gegen die Beats. Delay hat
bereits Sync — LFO nicht.
- **Anforderung:** `sync`-Parameter am Oscillator-LFO-Mode + `division`
  (wie DelayProcessor). Rate-Bereich dann in Notenwerten dargestellt.

## 9. Kompressor / Sidechain-Node

**Gewünscht:** Ducking per Kompressor mit Key-Eingang (statt AM-Emulation).

**Wo es fehlte:** Sidechain-Ducking in `house-pad-deepy` wäre klassisch über
einen Kompressor gelöst, der auf die Kick triggert. AM-Emulation klingt nach
"Tremolo statt Pump".
- **Anforderung:** `compressor`-Node: `in (signal)`, `key_in (signal)`,
  Parameter `threshold`, `ratio`, `attack`, `release`.

## 10. Mehr FM-Stufen (Feedback-Operator)

**Gewünscht:** DX7-Style 6-Operator mit Feedback-Operator (Modulator auf sich
selbst).

**Wo es fehlte:** Chained FM (`fm0 → fm1`) erlaubt 4 Stufen, aber kein
Selbst-Feedback (siehe Nr. 2). Ein Feedback-Operator erzeugt Sägen aus Sinus
und ist Kern vieler "House-Lead"-Sounds (DX Bass, DX Keys).
- **Anforderung:** `fm%Feedback`-Parameter (0-1) am FMModulationProcessor, der
  den Modulator-Eingang auf sich selbst zurückführt (intern, 1-Sample Delay).

---

## Bereits umgesetzte Ideen (Referenz)

Diese Ideen WAREN begrenzt, liessen sich aber mit den aktuellen Prozessoren
lösen:

- **Multi-Tap-Clap** → drei parallele ADSRs auf Noise (`house-clap-classic`)
- **Noise-Textures** → Static-Mode-Oscillator mit Noise-Waveform
  (`house-fx-vinyl-texture`, `house-snare-noise`)
- **LFO-Gates** → LFO-Mode-Oscillator → AM (`house-hihat-open`)
- **Pitch-Envelopes** → FAdsrProcessor (`house-kick-*`, `house-tom-deep`)
- **Waveshaper-Grit** → Hard/Fold-Shape (`house-bass-acid`, `house-ride-shimmer`)
- **Tempo-Delays** → Sync-Delays mit Division (`house-lead-pluck`)

## Priorisierung für SmolFM 3

1. **Filter** (Nr. 1) — größter Klanggewinn, Basis für alle weiteren Ideen
2. **Mod-Matrix** (Nr. 3) — macht LFO/Envelope auf alles routbar
3. **Stereo/pan** (Nr. 6) — moderner Klang
4. **LFO-Sync** (Nr. 8) — Produktions-tauglich
5. **Feedback-Operator + Feedback-Node** (Nr. 2, 10) — DX-Style-Erweiterung
6. **Kompressor** (Nr. 9) — Mixing-Level
7. **Glide** (Nr. 5) — Spielbarkeit
8. **Humanize** (Nr. 4) — Feinschliff
9. **Sample-Player** (Nr. 7) — optional, letzte Wahl (FM-first-Philosophie)