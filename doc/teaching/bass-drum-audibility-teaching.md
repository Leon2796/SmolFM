# Teaching: Warum der 808-Kick fast stumm war — Level, Klick und Velocity

> Stand-Notiz (03.10.2026, nach Hörbefund): Der hier beschriebene Direkt-Klick-Weg
> (`gain2`) und die Basis-Ebenen-Anhebung (0,5 → 2,0) wurden nach dem Anhören
> **wieder zurückgebaut** — die Kopie klang als weißer Rausch-Splatter, nicht als
> Klick (die driftbox-Kopie läuft durch einen Hochpass, den wir nicht nachgebaut
> hatten). Die Schleifen-Physik (Loop 0,22 / Korpus-Decay 0,30 s) blieb erhalten.
> Der Klick-Weg bleibt als spätere Option notiert: nur MIT Bandpass-Filter
> (Hochpass ~1200 Hz, driftbox-Wert) in der Klick-Leitung.

Diese Erklärung benutzt nur Bausteine, die es in SmolFM wirklich gibt
(`OscillatorProcessor`, `AdsrProcessor`, `GainProcessor`, `FeedbackProcessor`,
`ResonatorProcessor`, `WaveshaperProcessor`, `MasterOutputProcessor`). Fachworte
aus den 808-Quellen werden im Text übersetzt; ein Glossar hängt am Ende.

## 1. Was im Kick-Patch steckt (die Bausteine, wie sie da sind)

```
osc0      OscillatorProcessor (noise, static 6000 Hz) — das Rausch-Qelle
adsr0     AdsrProcessor (attack 0,001 / decay 0,008)  — schneidet daraus einen 8-ms-Schlag aus
gain1     GainProcessor (factor 2,5)                  — verstärkt den Schlag
feedback0 FeedbackProcessor (amount 0,22)             — Schleifenbrücke: gibt Korpus-Energie gestaucht zurück
resonator0 ResonatorProcessor (tune 49,5 / q 0,85 / decay 0,30 / spread 2,5) — der tönende Körper
shape0    WaveshaperProcessor (drive 0,25 soft)       — Sättigung (macht Wellenschwänze wärmer)
gain2     GainProcessor (factor 0,3)                  — Klick-Kopie (neu)
gain0     GainProcessor (factor 2,0)                  — Basis-Ebene (neu gesetzt)
output    MasterOutputProcessor (level 0,9)           — Mischung zum Ausgang
```

## 2. Warum du fast nichts gehört hast (der eigentliche Fehler)

Das ganze Tönen des Korps liegt bei **ungefähr 50–65 Hertz**. Das ist unter
dem Bereich, in dem gewöhnliche Lautsprecher (Laptop, kleine Boxen) überhaupt
Schall abgeben können. Analog gesprochen: ein echter Bassdrum-Körper brummt
tiefer als der musikalische Bass — auf Studiomonitoren hörbar, auf
Laptop-Boxen praktisch nicht.

Damit ein echter 808 trotzdem auf JEDER Box hörbar ist, schickt die
Originalschaltung zusätzlich einen **kurzen, hellen Rausch-Impuls nach vorne**:
der sogenannte „Impulsformer-Spitzenimpuls“ (in den Referenzen „pulse
shaper“) — digital gesprochen bei driftbox: ein kurzer 14-ms-Rausch-Impuls,
genau so etwas wie unsere 8-ms-`AdsrProcessor`-Leitung, nur **nicht durch den
Körper gefiltert**.

Unser Patch schickte den Anreger **nur in den Körper**. Der
`ResonatorProcessor` ist ein Band-Durchlass: er lässt nur die ~50-Hz-Schwingung
hinaus. Das helle „Tock“ blieb also auf der Strecke — übrig blieb der kaum
hörbare Sub-Brummler. Deshalb: Gain ×10, um den Sub-Brummler überhaupt über
die Anlage zu drücken — mit Verzerrung statt Punch.

## 3. Die Reparatur (nur mit vorhandenen Bausteinen)

- **Direkter Klick-Weg:** ein zweiter `GainProcessor` (factor 0,3) nimmt
  dieselbe 8-ms-`AdsrProcessor`-Leitung und geht **direkt** in einen zweiten
  Eingang des `MasterOutputProcessor`. Klangwirkung: das „Tock“ läuft am
  Körper vorbei — analog: die Impulsformer-Spitze und die Schwingung sitzen
  zusammen am Ausgang.
- **Basis-Ebene:** `gain0` 0,5 → 2,0. Reine Pegel-Tischlerei: die Kette
  liefert immer Ton (Laufprobe: Effektivwert vor 0,13, nach 0,50), nur lag der
  hörbare Anteil im falschen Frequenzbereich.
- **Schleifen-Physik:** `FeedbackProcessor` 0,35 → 0,22 und
  `ResonatorProcessor` decay 0,8 → 0,30 s. Referenzprinzip: „der Körper klingt
  kurz aus, alles darüber hinaus trägt die Schleife“ — vorher stieg der Ring
  sogar ohne neuen Schlag an (der „spinnende“ Schleifeneffekt).

## 4. Velocity — und warum „Gain-Auflösung“ hier keine Rolle spielt

Anschlagstärke (velocity 0–1, der „wie hart habe ich die Taste getroffen“-Wert)
wirkt im Kick an drei Stellen:

| Stelle | Baustein | Wirkung |
|---|---|---|
| Lautheit | `gain0` (factor × Anschlag) | direkte Lautstärke |
| Klangfarbe | `shape0` drive × Anschlag | härtere Sättigung bei hartem Treffer — das ist das Analog-Pendant zur Diode, die bei höherer Trigger-Spannung härter leitet |
| Ausklingdauer | `resonator0` spread × Anschlag | der Ring hält länger |

Gemessen (Laufprobe): bei nur einem Viertel Anschlag ist das Niveau nur ca.
3 dB leiser — die Lautheit hängt also **fast nicht** am Anschlag; der Anschlag
verändert vor allem **Klangfarbe und Ausklingdauer**. Genau so verhält sich
auch die echte Akzent-Schaltung des Originals (die Trigger-Spannung ändert
Puls-Form und Ton-Weg, nicht nur die Lautheit).

**„Gain-Auflösung“** (die Schrittweite/Bereich eines Reglers, hier 0–10) hat
mit dem Problem **nichts** zu tun und wurde **nicht** geändert. Eine feinere
Schrittweite hätte denselben Regler nur smoother drehbar gemacht. Geändert
wurden nur Basis-Pegel und Klick-Leitung — beides Patch-Ebene; die Engine blieb
unangetastet.

## 5. Wie die Bausteine im 808-Umfeld abweichen (ehrliche Lücke-Liste)

Was wir brauchten und **mit vorhandenen Bausteinen lösen konnten:**
- Basis-Pegel und Klick-Weg: `GainProcessor` + freie `MasterOutputProcessor`-
  Eingänge — keine Engine-Änderung nötig.

Was die Engine (noch) **nicht** hat, das die Referenzen voraussetzen:
- **Burst-Hüllkurve (Re-Trigger-Hüllkurve):** die Referenzen lösen das Snare-
  Rasseln (4 kurz aufeinanderfolgende Anschläge) und die Clap-Schlagkaskade
  über **mehrfach neu ausgelöste Hüllkurven**. SmolFM kennt keine; am Patch
  geht es nur per Verzögerung (`DelayProcessor`) — und ein Echo auf Rauschen
  klingt metallisch (genau das war der Phasen-Layer-Fehler in Snare/Clap).
- **Freilaufende Phasenbank:** `OscillatorProcessor` setzt die Phase bei jedem
  Treffer zurück; die echte Bank läuft weiter, jeder Treffer erwischt sie
  woanders (deshalb klingen zwei aufeinanderfolgende echte Hats nie identisch).
  Bei uns: identische Treffer bei schnellen Patterns.
- **Gemessene Ausgleichswerte (Trim):** Pegel-Einstellungen sind bisher
  geschätzt; ein Offline-Durchlauf mit Messwerten fehlt.

## 6. Analog ↔ Digital (die Übersetzungstabelle)

| Analog (echte 808-Schaltung) | SmolFM-Baustein | Anmerkung |
|---|---|---|
| Trigger-Impuls (Schlegel) | `AdsrProcessor` am Rausch-`OscillatorProcessor` | 8 ms kurz |
| Impulsformer-Spitze (Klick) | dieselbe 8-ms-Leitung, kopiert über eigenen `GainProcessor` → Master | der neue Direkt-Klick-Weg |
| Ausgangsverzerrung (Diode/Op-Amp) | `WaveshaperProcessor` (soft) | Sättigung |
| Brücken-T-Netzwerk (Körperschwingung) | `ResonatorProcessor` (tune/q/decay) | Abstimmung = gespielte Note |
| Rückkopplungs-Verlängerung des Ausklingens | `FeedbackProcessor` (amount) | trägt den langen Schwanz |
| Akzent (Trigger-Spannung) | vel-Eingänge (gain/shape/resonator) | Lautheit + Klangfarbe + Abklingzeit |

## 7. Glossar (die neuen/unverständlichen Worte)

| Begriff | Bedeutung (einfach) | In SmolFM |
|---|---|---|
| RMS / Effektivwert | „Mittel-Lautheit“ eines Signals — aussagekräftiger als der Spitzenwert für das, was man als dauerhafte Lautheit hört | gemessen in den Lauf-Tests (Laufprobe-Zahlen) |
| Peak / Spitzenwert | der allergrößte Momentanwert | `MasterOutputProcessor`-Anzeige (peakLevel) |
| Sub-Brummler / Sub-Bass-Ring | die ~50–65-Hz-Körperschwingung — unter dem musikalischen Bassbereich | das `ResonatorProcessor`-Ausgang |
| Anreger (Exciter) | die kurze Schläge-Energie, die einen Körper zum Schwingen bringt | die 8-ms-`AdsrProcessor`-Leitung |
| Anregerkopie (×0,3) | dieselbe 8-ms-Leitung über einen zweiten `GainProcessor` (factor 0,3), parallel zum Ausgang | der neue Direkt-Klick-Weg |
| Impulsformer (Pulse Shaper) | Original-Schaltungsstufe, die aus dem Trigger-Impuls den hörbaren Klick macht | bei uns: der kopierte 8-ms-Rausch-Ausgang |
| Klick / Klick-Pfad | das kurze helle Anfangsgeräusch und die Leitung, die es zum Ausgang trägt | der Direkt-`GainProcessor`-Weg |
| „phasig“ / Kammfilter | wenn eine verzögerte Kopie zur Original gemischt wird: einzelne Frequenzen löschen sich gegenseitig aus → hohler, metallischer Ton | der Snare/Clap-Misstand; `DelayProcessor` = mathematisch korrekt, nur für diesen Zweck ungeeignet |
| Decay-als-Schleifenverstärkung | das Referenzprinzip: der Körper klingt kurz, die Schleife trägt den langen Rest | `FeedbackProcessor` (amount) + kurzer `ResonatorProcessor`-decay |
| Auflösung (Regler) | Schrittweite/Bereich eines Reglers (hier 0–10) | unberührt — keine Rolle in diesem Fall |
| Basis-Ebene | der Grund-Pegel-Stand einer Stimme, bevor Anschlag/Schleifen wirken | `gain0` (0,5 → 2,0) |
| Trim | Ausgleich der Stimmenpegel im Kit nach dem Messen | aktuell geschätzt; Offline-Messlauf = offene Lücke |