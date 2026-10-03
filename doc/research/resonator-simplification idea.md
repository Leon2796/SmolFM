Ja, **das ist möglich**. Und für dein Modell wäre das sogar eine klare alternative Architektur: Der Resonator bekommt nicht mehr ein Audiosignal als `in`, sondern einen **Energie-/Anregungsimpuls**, der aus `freq` und `vel` erzeugt wird.

Auf Basis deiner aktuellen Resonator-Spezifikation würde ich es so planen:

## Ziel

Bisher:

$$
\boxed{Audio\text{-}Input \rightarrow Resonator \rightarrow Ring}
$$

Neu:

$$
\boxed{freq + vel + Zeitfenster \rightarrow Energieimpuls \rightarrow Resonator \rightarrow Ring}
$$

Der Resonator selbst bleibt für **Frequenz, Schwingung und Decay** zuständig. Nur die Quelle der Anfangsenergie wird geändert.

---

# 1. Neue Bedeutung der Eingänge

### `freq`

Bestimmt die Resonanzfrequenz:

$$
f_0 = freq
$$

Das entspricht bereits dem aktuellen `tune`-Verhalten: Der Portwert wird als Resonanzfrequenz in Hz verwendet. 

### `vel`

Wird zur **Anregungsstärke**:

$$
E_\text{input} = F(vel)
$$

Zum Beispiel zunächst ganz einfach:

$$
E_\text{input}=vel
$$

mit

$$
0\leq vel\leq1
$$

Damit wäre:

```text
vel = 0.2  → kleiner Anschlag
vel = 0.5  → mittlerer Anschlag
vel = 1.0  → starker Anschlag
```

---

# 2. Zeitfenster für die Anregung

Das ist der entscheidende neue Teil.

Statt:

```text
in = Audiosignal
```

hast du beispielsweise:

```text
vel = 1
active = 10 ms
freq = 200 Hz
```

Während dieser 10 ms wird Energie in den Resonator gegeben.

Danach:

```text
active = 0
```

und der Resonator läuft **alleine weiter**.

Also:

```text
Anregung:

████████
████████
████████
        └──────────────────

              ↓

Resonator:

   /\/\/\/\/\/\/\/\/\/\/\
  /                       \____
```

Das entspricht viel stärker dem Bild:

> **Anschlag → Energie hinein → Resonator schwingt aus.**

---

# 3. Wie erzeugen wir aus `freq` einen sinnvollen Energieimpuls?

Hier würde ich **nicht einfach einen Gleichstromwert `vel`** in den Filter schicken.

Denn der Resonator soll gezielt bei `freq` angeregt werden.

Eine einfache Möglichkeit wäre, innerhalb des Zeitfensters eine kurze Sinus-Anregung zu erzeugen:

$$
x(t)=A(vel)\sin(2\pi f_0t)
$$

mit

$$
A(vel)=vel
$$

Dann wäre beispielsweise:

$$
freq=200\,Hz
$$

$$
vel=0.8
$$

und für 10 ms:

$$
x(t)=0.8\sin(2\pi 200t)
$$

Danach:

$$
x(t)=0
$$

Der Resonator hat zu diesem Zeitpunkt aber bereits Energie in seinen Zuständen gespeichert und schwingt weiter.

---

# 4. Noch besser: Energie statt Audio-Signal

Wenn dein eigentliches Ziel lautet:

> „Ich möchte gar keinen künstlichen Audiostream als Exciter erzeugen.“

dann würde ich sogar einen Schritt weitergehen.

Wir könnten beim Anschlag direkt den **Zustand des Resonators setzen**.

Also ungefähr:

```text
freq
  │
  ├──> Resonanzparameter
  │
vel
  │
  └──> Anfangsenergie
           │
           ↓
       ic1 / ic2
           │
           ↓
      Resonator
           │
           ↓
         OUT
```

Das wäre mathematisch sehr elegant.

Der Anschlag würde dann nicht erst ein Signal erzeugen, das anschließend durch den Resonator läuft.

Stattdessen:

$$
\boxed{\text{Anschlag} \rightarrow \text{Resonatorzustand}}
$$

---

# 5. Was passiert beim Anschlag?

Angenommen:

```text
freq = 200 Hz
vel  = 0.8
```

Dann berechnen wir zunächst die Resonatorparameter für 200 Hz.

Anschließend setzen wir die internen Zustände entsprechend der gewünschten Energie:

$$
ic1 \leftarrow F(vel,freq)
$$

$$
ic2 \leftarrow G(vel,freq)
$$

Danach gibt es **keine weitere Anregung**.

Der normale Resonator-Code läuft weiter:

$$
(ic1,ic2)_n
\rightarrow
(ic1,ic2)_{n+1}
\rightarrow
(ic1,ic2)_{n+2}
\rightarrow \dots
$$

und erzeugt:

$$
y[n]
$$

als Ring.

---

# 6. Retrigger

Das ist für deinen Anwendungsfall besonders wichtig.

Deine bestehende Spezifikation sagt bereits:

> Retrigger kippt neue Energie in den noch klingenden Körper; es gibt keinen Neustart. 

Das können wir beibehalten.

Also:

```text
Anschlag 1
     ↓
   █████
      \___________
                  \____

Anschlag 2
              ↓
             ████
                  \________
```

Der zweite Anschlag ersetzt die alte Energie **nicht**.

Er addiert Energie zum bestehenden Zustand:

$$
state_\text{neu}
=
state_\text{alt}
+
state_\text{excitation}
$$

Das ist für einen Drum-/Percussion-Resonator sehr interessant.

---

# 7. Was machen wir mit `Decay`?

**Decay würde ich nicht verändern.**

Es bleibt die Eigenschaft des Resonators:

$$
A(t)=A_0e^{-t/\tau}
$$

Deine bestehende Spezifikation beschreibt genau dieses Prinzip: Decay ist die Abklingzeit des Resonators bzw. seines Polradius und keine separate Hüllkurve. 

Damit hätten wir eine sehr klare Trennung:

| Parameter           | Aufgabe                                         |
| ------------------- | ----------------------------------------------- |
| `freq`              | **Frequenz des Resonators**                     |
| `vel`               | **eingebrachte Anfangsenergie**                 |
| `excitation window` | **wie lange Energie eingebracht wird**          |
| `decay`             | **wie schnell die Energie wieder verschwindet** |
| `Q`                 | **Resonanz-/Schwingungsverhalten**              |

---

# 8. Implementierungsplan

Ich würde es in **vier Phasen** implementieren.

### Phase 1 – Exciter vom Audio-Input trennen

Aktuell:

```cpp
x = input.getSample();
```

Neu:

```cpp
excitation = calculateExcitation(freq, vel, time);
```

Der eigentliche Resonator bleibt zunächst unverändert.

---

### Phase 2 – Zeitfenster einführen

Neue Zustände beispielsweise:

```cpp
bool excitationActive;
int excitationSamplesRemaining;
float excitationVelocity;
float excitationFrequency;
```

Beim Trigger:

```cpp
startNote(freq, vel);
```

wird beispielsweise gesetzt:

```cpp
excitationSamplesRemaining = excitationTime * sampleRate;
```

Während dieser Samples wird Energie eingebracht.

Danach:

```cpp
excitation = 0;
```

---

### Phase 3 – `vel` wirklich zur Energie machen

Statt der jetzigen Bedeutung

$$
t_\text{eff}=t(1+v(spread-1))
$$

würde `vel` zunächst die Anregungsamplitude bestimmen. Die aktuelle Velocity→Decay-Kopplung müsste dann entfernt oder optional gemacht werden. Die bestehende Spezifikation definiert `vel` derzeit ausdrücklich als Faktor für die effektive Decay-Zeit. 

Ich würde zunächst ganz einfach starten:

$$
E=vel^2
$$

Das hat den Vorteil, dass Velocity eher wie **Energie** als wie lineare Amplitude wirkt.

---

### Phase 4 – später den künstlichen Exciter entfernen

Wenn das funktioniert, würde ich prüfen, ob wir überhaupt noch

$$
x[n]=A\sin(2\pi f_0t)
$$

brauchen.

Dann können wir direkt die Resonatorzustände anregen:

$$
\boxed{state \leftarrow state + excitation(vel)}
$$

Das wäre die sauberste Version deines Konzepts.

---

## Mein Vorschlag für die Architektur

Ich würde letztlich auf dieses Modell hinauswollen:

```text
             NOTE / TRIGGER
                   │
          ┌────────┴────────┐
          │                 │
        FREQ               VEL
          │                 │
          ↓                 ↓
   Resonanzfrequenz    Anfangsenergie
          │                 │
          └────────┬────────┘
                   ↓
             ANREGUNG
          (kurzes Fenster)
                   │
                   ↓
          ┌─────────────────┐
          │   Resonator     │
          │                 │
          │ komplexes       │
          │ Polpaar         │
          │                 │
          │ + Dämpfung      │
          └────────┬────────┘
                   ↓
                  OUT
```

**Das Entscheidende:** `freq` und `vel` würden dann nicht selbst die Schwingung ersetzen. Sie **initialisieren den Resonator**. Danach erzeugt das Polpaar die Schwingung und der Polradius bestimmt das Ausschwingen.

Das ist genau die Trennung, die du vorher intuitiv beschrieben hast: **Energie hinein → loslassen → Resonator schwingt selbstständig.**

---

# 9. Neubewertung (03.10.2026, nach dem Referenz-Deep-Dive)

Vorbemerkung: obige Sektionen halten den ursprünglichen Vorschlag unverändert; die Urteile der vorherigen Session stehen in `808 bass drum simulation.md` §4. Diese Sektion fügt die Neubewertung dem Plan selbst hinzu — auf Basis des Referenz-Teacherings (`reference-simulations-teaching.md`, 5 Umsetzungen ausgewertet), des korrigierten Kernbefunds (Pulse-Shaper **nicht** Voltage-exklusiv) und des am HEAD befindlichen Tuned-Strike-Patches.

**Gesamturteil: die drei Urteile der Vorsesession halten — Phase 2 umsetzen, Phase 3 als Addition, Phase 4 abgelehnt.** Was sich ändert: die Beleglage (plausible Physik → gemessen/live geprüft), die Schnittstellenbeschreibung und die Phasenlandkarte (Phase 1 entfällt als eigener Schritt).

| Phase | Vorheriges Urteil | Neubewertung (03.10) |
|---|---|---|
| 1 — Exciter vom Input trennen (`calculateExcitation()` statt `in`) | offen | **überholt**: auf Patch-Ebene seit `fb4d341` erreicht (Sinus-Impuls statt Rauschen als Energie); im Engine-Code noch nicht umgesetzt — geht direkt in Phase 2 auf |
| 2 — Zeitfenster / interner, note-getrackter Strike | umsetzen | **umsetzen — jetzt referenzbelegt, Schnittstelle verschärft** |
| 3 — E = vel² | Addition | **Addition — jetzt referenzbelegt, Geltungsbereich beschränkt auf den Strike-Pfad** |
| 4 — Anregung ohne Exciter (`state ← state + excitation`) | abgelehnt | **abgelehnt — jetzt mit benanntem Gegner (`bd_check`)** |

## 9.1 Phase 1 — überholt, nicht umgesetzt

Das Ziel (Exciter vom Körper entkoppelt, Anreger auf Korpusnote) hat der Patch `fb4d341` bereits geliefert: Sinus-Impuls (25 ms) in den Körper, Rauschen nur noch leise Attack-Farbe. Der Engine-Code liest den `in`-Port unverändert (`ResonatorProcessor.cpp:139`) — „überholt" heißt also nicht „erledigt": Ein eigener Engine-Schritt `calculateExcitation()` wäre nach dem jetzigen Stand Doppelarbeit; was er enthielte, wird Teil von Phase 2.

## 9.2 Phase 2 — umsetzen, Schnittstelle verschärft

**Beleg gestiegen:** Die Referenzform existiert offen und mehrfach — 8W8 (`sc808_circuit_common.h`: „the pulse shaper, the op-amp clip, the bridged-T, and now the multiple-feedback bandpass"), Chowdhurys `TR_808/`-Fragment (`PulseShaper`, `OutputFilter`), Voltage 80 (Wright-Omega pro Sample). Der Plan schlägt exakt die fehlende Baustein-Hälfte vor: definierte Spannung hinein, das Netzwerk schwingt allein.

**Verschärfungen der Schnittstelle:**
- `strikeLevel`/`strikeTime` wie die bestehenden vier Parameter als APVTS-Paare (`resonator<N>StrikeLevel`, `resonator<N>StrikeTime`), Defaults 0 / 0 s → Strike aus = **byteidentisch** (gleicher Neutralitäts-Vertrag wie der vel-Port, `ResonatorProcessor.h:23–28`).
- Der Strike geht **in Stufe A** (Eingang `x_eff = in + strike`), nicht in die Zustände — die Kopplungsstufe (Bandbreite = „Tinte") bleibt Teil der Physik, und der Retrigger-Vertrag (Tail bleibt, Energie addiert sich, `ResonatorProcessor.cpp:64–69`) bleibt automatisch gewahrt.
- **Note-Tracking:** Strike-Frequenz = laufendes f0 (wie Stufe A/B pro Sample neu gerechnet — koherenter Transfer auch bei Tuning-Sweeps); Pegel = `strikeLevel · vel`, vel beim Anschlag erfasst — Musterprecedenz: Parameter in `startNote()` lesen (so macht es der AdsrProcessor, vgl. `doc/processors/AdsrProcessor.md:69`).
- **Referenzwerte:** 14 ms (Pulse-Shaper-Messung) bzw. 25 ms (Patch-Praxis) für die Impulslänge; die Rauschen-Farbe bleibt Patch-Sache (Hochpass ~1200 Hz, leise) — Farbe, nie Energiekanal.
- **Umsetzung (03.10.2026):** Phase 2+3 gelandet — `resonator<N>StrikeLevel`/`resonator<N>StrikeTime` (Defaults 0 → byte-identisch), der Hieb in Stufe A (`x_eff = in + strike`, der Nulltest bestätigt: Hieb aus = stummer Körper ohne Anschluss), Pegel `strikeLevel · vel` über den eigenen ROH-Anschluss `strike_vel` (keine Kurve im Knoten → Amplitude ∝ vel, Energie ∝ vel² im selben Zug — die vel-Port-/sqrt-Welt des Patch bleibt unberührt). Die Kraftgesetz-Probe ((c)) fiel mit in die Phase 2: Ring-RMS 2.000000 bei vel 1,0 vs 0,5; die Determinismus-Probe ((b)): zwei identische Treffer → exakt identisches Ring-RMS (0.032875). Die Patch-BD trägt `strikelevel 0.4077, striketime 0.008 s` — aus der Proben-Kalibrierung gelöst (Korn-RMS 0.328925 vs Sinus-Boden 0.032357); die Austausch-Bande misst +21 % Körper-RMS (der kohärente in-Phase-Anteil, Band ±30 %).
- **Test-Kultur** (Row 10, nach dem `bd_check`-Prinzip): (a) Default-Neutralitäts-Probe — strikeTime 0 → byteidentisch gegen den alten Pfad; (b) Determinismus-Probe — zwei identische Hits → identische Ring-RMS (kein Streufaktor ~0,3–2); (c) nach Phase 3: Pegelgesetz-Probe — Ring-RMS bei vel 1,0 vs 0,5 ≈ Faktor 2. Der veraltete `runBassDrumChainTest`-Kommentar (Anreger ×2,5, Bridge 0,35, Out ×0,5) wurde am 03.10.2026 vorab angehoben — er nennt jetzt die realen Proben-Werte (0,22/×10, decay 0,30, spread 2,5) und den Snapshot-Stand `06519f9`; **die komplette Anhebung der Probe auf den Tuned-Strike-Stand ist mit Phase 2 am selben Tag erledigt** (die Probe spiegelt jetzt die Live-Strecke + Austausch-Messung + die eigene Hieb-Probe).

## 9.3 Phase 3 — Addition, jetzt physikalisch belegt

Der Accent der Referenzen ist eine **Trigger-Spannung**: 8W8 misst 4 V vs 14 V (lauter **und** das Spektrum wandert — Diodenpfad), Voltage 80: „Accent starts as trigger voltage". Energie ∝ Spannung² ist die konsistente Skalierung. Heute liefert der Patch über `vsc0` (sqrt) vor den Accent-Verbrauchern effektiv **E ∝ vel** — Phase 3 steilt die Strike-Domäne auf **E ∝ vel²** (Amplitude ∝ vel). Zwei Regeln: (1) Das Gesetz gilt nur für den internen Strike-Pfad; die vsc-Kette der drei Verbraucher (spread/gain/shape) darf nicht doppelt kurven. (2) Der vel→decay-Spread (Row 4, „Decay über VIN") bleibt unangetastet — auch er ist Referenzverhalten, keine Konkurrenz zur Energie-Domäne.

**Umsetzung (03.10.2026):** beide Regeln beachtet und gelandet — der Hieb liest seine Spielstärke über den eigenen Anschluss `strike_vel` (rohe Anschlags-Note, kein vsc-Fenster davor, unverbunden = 1.0); die Probe mess die Kraft direkt: Amplitude ∝ Spielstärke, Ring-RMS **2.000000** bei vel 1,0 vs 0,5.

## 9.4 Phase 4 — Ablehnung jetzt schärfer

Der Zustands-Sprung (`ic ← ic + excitation`) ist exakt die Fehlklasse **„envelope with a sine on it"**, gegen die 8W8s `tools/bd_check.cpp` testet: keine Kopplungsstufe, Velocity als reine Amplitude (erledigt Phase 3), harter Sprung statt Netzwerk-Hub. Das ursprüngliche Motiv („kein künstlicher Audiostream") entfällt überdies — nach Phase 2 lebt der Strike im Knoten, nicht als Patch-Signal. Und der Plan war hier konservativer als die Referenzen (§4–5 „Zustand setzen"): real injiziert jedes Referenz-Modell eine **Spannung in ein Netzwerk**, nie Zustände. Aus der Roadmap gestrichen; der Status bleibt hier dokumentiert.

## 9.5 Was außerhalb des Plans bleibt (Grenzen, keine Phasen)

- **Loop-Physik:** Die Referenz-Decay-Form paart den Eigenring (~80 ms) mit dem Loop-Gain („Decay is loop gain") — bei uns bleibt die Loop die Patch-Bridge (aktuell 0,22); der interne Strike ist nur einer der zwei Bausteine.
- 45,7/49,5 Hz (ungeladen/geladen) weiterhin unmodelliert (Row 9); Machine-Gunning/Lücke ④ (Row 8) außerhalb dieses Plans — deterministische Strikes machen Wiederholungen **identischer**, nicht variierter; Humanize ist bewusste Würz-Domäne (808-synth).

Sequenzierung der Umsetzung: Phase 2 und Phase 3 in einem Zug (gleiche Schnittstelle, E = vel² im Strike-Pfad), Phase 1 aus der Landkarte nehmen, Phase 4 ist abgeschlossen. — **Stand 03.10.2026: Phasen 2+3 erledigt (Hieb + Kraftgesetz gelandet, Patch-BD umgebaut, Proben grün); Phase 1 bleibt überholt; Phase 4 abgeschlossen.** Offen aus dieser Landkarte: nur die Schaltungs-Feinheit des Pulse-Shaper-Netzwerks (Wright-Omega) als künftige Stufe.
