# Stereo-Effekte & Stereo-Support — Lückenanalyse und Verbesserungsvorschläge

> Referenz: `doc/improvement-plans/famous-synth-algorithms-gap-analysis.md`
> (dort Abschnitt 5, Punkt 11: „Stereo/Panning, Reverb als Ausgabekette"),
> `src/processors/MasterOutputProcessor.h`, `doc/processors/MasterOutputProcessor.md`,
> `doc/formats/smolfm.xsd`, `instruments/house/ideas.md`.

Analog zur Synthesizer-Lückenanalyse, aber mit anderem Fokus: Welche
Stereo-Effekte kann ein SmolFM-Graph heute **nicht** ausdrücken, und welche
Engine-/XSD-Erweiterung bräuchte jeder einzelne davon? Als Referenz dienen —
anders als in der Synthesizer-Analyse — nicht berühmte Instrumente, sondern
die **Effekte selbst** (Effektname statt Geräte- oder Synthesizername).

Der P3-Punkt 11 der Synthesizer-Analyse wird hier aufgespalten: Die
Grundlagen (Kanal-Domäne, Panner) sind kleine, isolierte Änderungen und
sind Vorbedingung für alles weitere — deshalb stehen sie vorne.

---

## 1. Ist-Zustand (warum alles mono ist)

1. **Keine Kanaldimension:** Ports tragen nur `frequency` (Hertz) und
   `signal` (Audio −1…+1); kein Knoten des Graphen kennt L/R.
2. **Mono-Endstufe:** `MasterOutputProcessor` mischt bis zu acht Inputs zu
   **einem** Ausgangssample: `y_n = L · Σ x_{k,n}`. Es gibt einen `output`,
   kein `outL`/`outR`; er ist zugleich der einzige Summierer im Graphen.
3. **Extern stereo, intern mono:** Das Plugin meldet dem Host ein
   Stereo-Buslayout (`AudioChannelSet::stereo()`), aber das Voice-Signal —
   ein Float pro Sample — landet identisch auf beiden Kanälen. Der Graph
   selbst kann keinen Unterschied zwischen L und R ausdrücken.
4. **Folge:** Jeder „Stereo-Effekt" kollabiert auf die Mitte. Schon in
   `instruments/house/ideas.md` festgehalten: Ping-Pong-Delays
   (`house-lead-pluck`) laufen in einen gemeinsamen Summen-Input — der
   Stereo-Image-Effekt geht verloren.

---

## 2. Übersichtstabelle — fehlende Stereo-Konzepte

Je Zeile ein fehlendes Konzept mit Klangwirkung und konkretem
Processor-Vorschlag (P1 = höchste Priorität, Details in Abschnitt 3).
Verweise in Klammern betreffen Abhängigkeiten zur Synthesizer-Lückenanalyse.

| Fehlendes Konzept | Beschreibung | Bekannter Effekt | Processor-Vorschlag | Klangeigenschaften |
|---|---|---|---|---|
| Stereo-Domäne / L-R-Bus | `signal` ist kanallos; nichts kann L/R unterscheiden | True-Stereo-Bus | **P1**: Kanal-Dimension für `signal` (oder Splitter-/Merger-Knoten); `output` mit `outL`/`outR` | Basis für alle Breiteneffekte; getrennte Ketten ohne Auslöschung |
| Statisches Panorama | Equal-Power-Position −1…+1 pro Kette | Pan-Pot | **P1**: `panner`-Knoten (in/out: signal) | Platzierung im Stereobild, Separation der Ketten |
| Auto-Pan (modulierbares Pan) | Position folgt einer Steuerquelle (LFO, ADSR) | Auto-Panner | **P1**: `pan_mod`-Port am `panner` | Bewegung im Stereobild, Rotation, rhythmische Seitenwechsel |
| Ping-Pong-Delay | Echos alternieren zwischen L und R, internes Feedback kreuzt die Kanäle | Ping-Pong-Delay | **P2**: `pingpong`-Modus am `delay` (Kreuz-Feedback im Knoten — per Graph nicht komponierbar, da zyklenfrei) | Springende, wandernde Echos (Dub) |
| Haas-Effekt / Micro-Delay | 1–30 ms Versatz auf einem Kanal, beide Signale bleiben korreliert | Haas-Widener | **P2**: `width`-Knoten (Micro-Delay + Pan); mit reinen Bausteinen schon komponierbar — `delay` ab 1 ms plus zwei Outputs | Breite ohne Schwebung, Klang „aus dem Off" |
| Stereo-Chorus | Mehrere modulierte Delaylinien mit LFO-Phasenversatz (90°/180°) | Stereo-Chorus (Ensemble) | **P2**: `chorus`-Knoten (2–4 Zungen, `spread`); braucht modulierbare Delay-Zeit (vgl. Synthesizer-Lückenanalyse §5 P1.2) | Schwebende Breite, Ensembleschmelz |
| True-Stereo-Delay | Zwei unabhängige Delaylinien, je Kanal eigene Zeit | Stereo-Delay | **P2**: Stereo-Bus + `delay` je Kanal; optional `channel`-Parameter am `delay` | Räumlich getrennte Echos, komplexe Echo-Muster |
| Cross-Feedback L↔R | Feedback kreuzt zwischen den Kanälen statt zurück auf denselben | Cross-Feedback-Delay | **P2**: Kreuz-Feedback-Pfade im `pingpong`-/Stereo-Delay-Modus | Wandernde Echokaskaden, unendliche Dub-Weiten |
| Mid/Side-Verarbeitung | M/S-Matrix (encode/decode), Side-Gain regelbar | M/S-Widener | **P2**: `midSide`-Knoten (in/out: signal, `width` 0…2) | Feine Breitenregelung, Mono-Kompatibilität kontrollierbar |
| Stereo-Phaser / -Flanger | Dekorrelierte Allpass- bzw. Delaykaskaden je Kanal | Stereo-Phaser | **P2**: LFO-Phasenoffset am künftigen `allpass`-Knoten (vgl. Synthesizer-Lückenanalyse §4, Zeile „Phaser/Allpass") | Wirbelnde, breite Phasing-Swirls |
| Rotary-Speaker | Rotation mit Doppler-Verschiebung, Horn/Bass-Split | Rotary Speaker | **P3**: komponierbar aus Auto-Pan + moduliertem Delay (Doppler) + BP/LP-Split am `filter` | Wirbelnde Rotation, Doppler-Chorus, drei-dimensionale Bewegung |
| Per-Voice-Pan / Random-Pan | Jede Voice eigene (Zufalls-)Position im Stereobild | Voice-Spread (Random-Pan) | **P3**: Pan-Zufallsspread pro Voice; an Polyphonie gekoppelt (vgl. Synthesizer-Lückenanalyse §5 P3.10) | Ensemble-Breite, weniger Maskierung zwischen Noten |
| MicroPitch / Detune-Spread | Leicht verstimmte, kurz verzögerte Delays je Kanal | MicroPitch | **P3**: Unison-Spread auf L/R-Verstimmung mappen (vgl. Synthesizer-Lückenanalyse §4, Zeile „Unison/Supersaw") | „Pitch-Drift"-Breite, vokaler, warmer Charakter |
| Stereo-Reverb (dekorreliert) | Raumsimulation mit unterschiedlicher L/R-Charakteristik | Stereo-Hall | **P3**: Ausgabeketten-Effekt (vgl. Synthesizer-Lückenanalyse §5 P3.11) mit L/R-Dekorrelation | Räumlichkeit und Tiefe ohne Phasenlöcher in der Mitte |
| Mono-Kompatibilitäts-Check | Downmix-Schalter plus Korrelationsanzeige in der UI | Mono-Summe / Korrelationsmeter | **P3**: `mono`-Option am `output`; Korrelationsmeter im Master-Panel | Phasenprobleme und Breitenverlust früh sichtbar machen |

---

## 3. Priorisierte Vorschläge

P1 — Grundlagen, kleine XSD-/Engine-Änderung, größter Zuwachs:

1. **Stereo-Domäne** (Kanal-Dimension für `signal` bzw. Splitter-/Merger-Knoten
   plus `outL`/`outR` am `output`): Vorbedingung für alle anderen Zeilen der
   Tabelle. Erst danach können zwei Ketten getrennt ausgespielt werden.
2. **`panner`-Knoten** mit `pan_mod`-Port: statisches Panorama und Auto-Pan
   in einem Schritt, Equal-Power-Gesetz gegen Lautstärkesprünge.

P2 — die klassischen Stereoeffekte:

3. **Ping-Pong-Modus** am `delay` inklusive Kreuz-Feedback (Dub-Echos).
4. **`width`-Knoten** (Haas/Micro-Delay) für billige, artefaktarme Breite.
5. **`chorus`-Knoten** mit Spread und LFO-Phasenversatz — abhängig von
   modulierbarer Delay-Zeit (Synthesizer-Lückenanalyse §5 P1.2).
6. **`midSide`-Knoten** und True-Stereo-Delay-Option (`channel`-Parameter).

P3 — Abhängigkeiten und Feinschliff:

7. Rotary-Komposition aus den P1/P2-Bausteinen (Auto-Pan + Doppler-Delay + Filter-Split).
8. Per-Voice-Pan (braucht Polyphonie) und MicroPitch (braucht Unison-Spread).
9. Stereo-Reverb in der Ausgabekette, Mono-Check-Meter in der UI.