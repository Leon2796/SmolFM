# Teaching: Der ResonatorProcessor — wie ein Trommelkörper als Code funktioniert

Stand: 03.10.2026. Erklärt mit den Bausteinen, die es in SmolFM wirklich gibt
(`NoteProcessor`/note, `OscillatorProcessor`/osc, `AdsrProcessor`/adsr,
`GainProcessor`/gain, `ResonatorProcessor`, `MasterOutputProcessor`/output).
Anlass: die Frage „Warum hört man ab Decay ≈ 3,5 s gegenüber Decay 10 s keinen
Unterschied mehr?" Die Antwort hängt an einer einzigen klemmenden Zahl — und
führt mitten in die Funktionsweise des Knotens. Ein Glossar hängt am Ende.
Verwandt: [tuned-strike-exciter-teaching.md](tuned-strike-exciter-teaching.md)
(der Anreger und seine Kohärenz).

## 1. Was in der Kette steckt (Bausteine, wie sie da sind)

Debug-Patch `instruments/debug/resonator-demo.smolfm`:

```
note ──out (Note f)──→ osc0.note_in          osc0 = OscillatorProcessor (square)
note ──out (Note f)──→ resonator0.tune       die Note IST die Körperfrequenz
note ──vel──────────→ resonator0.vel         Accent → längerer Ring (spread 2,0)
note ──vel──────────→ gain0.vel              Accent → lauter
osc0 ──out──→ adsr0.in                       adsr0 = 18-ms-Klick (a 0,001 / d 0,018 / s 0 / r 0,002)
adsr0 ──out──→ resonator0.in                 DER ANREGER SCHLÄGT DEN KÖRPER
resonator0 (tune 220*, q 0,9, decay 0,5, spread 2,0) ──out──→ gain0 (1,6) ──→ output (level 0,85)
   (* tune 220 gilt nur unverbunden; hier überschreibt der tune-Port sie mit der Note)
```

Rollen: osc0 + adsr0 = **Anreger** (Exciter), resonator0 = **Körper**,
gain0 (mit vel) = **Lautheit mit Accent**, output = Ziel.

## 2. Was beobachtet wurde (das Symptom)

- Decay 3,9 s vs. Decay 10 s: **kein hörbarer Unterschied** (Hörbefund 03.10.2026).
- Der Ring „friert" ein: je nach gespielter Note passiert ab ~1–4 s nichts mehr.
- Messzahlen dazu, aus der letzten `SmolFM_GraphTests`-Runde (03.10.2026,
  Test „Resonator ring + decay"):
  - Ring bei Tune 440 Hz: **109 Nulldurchgänge in 0,125 s** ≈ 436 Hz — der Ring
    singt auf der eingestellten Frequenz weiter (Toleranz ±30 %).
  - **RMS früh/spät = 5598** — die Rechnung e^(0,375/τ) mit τ = t60/ln(1000)
    sagt ≈ 5500 voraus; Messung und Modell decken sich auf unter 1 %.

## 3. Die Erklärung (analog ↔ digital, Baustein für Baustein)

### 3.1 Die Analog-Idee: ein Körper, kein Oszillator

Im TR-808 sind Kick-/Tom-/Conga-/Claves-Körper **Bridged-T-Schaltungen**:
Bandpass-artige Netzwerke aus Kondensatoren und Widerständen mit genau einem
schwingenden Modus. Man schlägt sie an (kurzer Trigger-Impuls), sie antworten
mit einem abklingenden Fast-Sinus auf ihrer Modus-Frequenz. Kein separates
Envelope-Modul — **das Abklingen steckt in der Schaltung selbst**.

Im Code ist das ein **komplexes Polpaar**. Polpaar = zwei Speicherzustände, die
ihre Energie gegenseitig hin und her werfen (Schwingung) und dabei an Dämpfung
verlieren (Abkling). Der Anreger kippt Energie in die Speicher; danach geben
sie sie als Sinus wieder ab — der Zustandsvektor schrumpft, fertig.

### 3.2 Zwei Bandpässe in Kaskade — Stage A (Kopplung) und Stage B (Ring)

Der Knoten ist **zwei** TPT-Bandpässe hintereinander (derselbe Kern wie im
`FilterProcessor`, aber fester Modus Bandpass und pro Abtastwert neu gerechnet):

```
in ──→ Stage A (tune f0, Dämpfung kA aus q) ──→ Stage B (tune f0, Dämpfung kB aus decay) ──(×kB)──→ out
        Kopplungsstufe:                          Ringstufe:
        wieviel Energie des Anregers             wie lange die gespeicherte
        ins Polpaar fällt (Bandbreite, „Tinte")  Energie als Sinus zurückkommt
```

**Stage A**: `kA = 2 · (1 − 0,99 · q)`.
- q = 0 → kA = 2 (maximal breit, kaum Färbung);
- q = 0,5 (Default) → kA ≈ 1 (Q ≈ 1);
- q = 0,9 (Demo) → kA ≈ 0,22 → Bandbreite ≈ kA·f0 ≈ 48 Hz bei 220 Hz;
- q = 1 → kA = 0,02 (nahe Selbstresonanz, bleibt stabil).

**Stage B**: `kB = ln(1000) / (π · f0 · decay)` — umgeleitet aus der Forderung
„Pegel fällt in `decay` Sekunden auf 0,1 % (−60 dB)". Die Zeitkonstante ist
**τ = decay / ln(1000) ≈ decay / 6,91** (pro τ sinkt der Pegel um −8,7 dB).
Decay ist also **t60**, nicht das e-Faltungs-τ.

Demo bei A3 (f0 = 220, decay 0,5): kB ≈ 0,020, τ ≈ 0,072 s (aus dem Code
gerechnet). Nach 0,25 s sind 3 % des Rings übrig, nach 0,5 s genau 0,1 % —
t60 versprochen, t60 gehalten.

### 3.3 Jeder Abtastwert: der TPT-Schritt (warum das Polpaar sauber bleibt)

Pro Abtastwert, beide Stufen (gekürzt, ResonatorProcessor.cpp:22-32):

```
v3 = Eingang − ic2            Abstand gegen den gespeicherten Stand
v1 = a1·ic1 + a2·v3           Bandpass-Ausgang — DAS ist der Klang
v2 = ic2 + a2·ic1 + a3·v3     Tiefpass-Ausgang — intern, wird hier nicht genutzt
ic1 = 2·v1 − ic1              neuer Speicher, ohne eine Abtastung Versatz
ic2 = 2·v2 − ic2
```

- `g = tan(π · f0 / fs)`, und daraus `a1 = 1/(1 + g(g+k))`, `a2 = g·a1`,
  `a3 = g·a2` — **pro Sample neu berechnet**. Deshalb glidet der Körper
  zipperfrei, wenn ein `fadsr` (Frequenz-Envelope) auf den `tune`-Port schreibt.
- `ic1` ≈ die Geschwindigkeit des Schwingers, `ic2` ≈ die Auslenkung
  (Feder-Masse-Analogie: genau diese beiden Zahlen sind das Polpaar).
- Das „2·x − alt" ist die trapezoidale Integration (TPT — topology-preserving
  transform): die Rückkopplung sitzt ohne Verzögerung in der Schleife, die
  Abklingkurve stimmt dadurch auch bei tiefen Frequenzen mit der Analogrechnung
  überein.
- Stereo: die Koeffizienten sind gemeinsam, nur die vier Zustandspaare
  (ic1/ic2 × Stufe A/B) sind links/rechts getrennt.

### 3.4 Peak-Normierung: warum der Ring nicht mit kB leiser wird

Ein Bandpass hat bei f0 einen Gewinn von ~1/kB (je schmaler, desto lauter am
Peak). Die Multiplikation `y = kB · v1B` balanciert das exakt aus: ein
Dauerton auf f0 geht mit Gewinn 1 durch. Konsequenz: **decay ändert nur die
Ringdauer, nicht die Lautheit** — die Energie pro Schlag kommt aus dem Anreger,
die Dauer aus kB.

### 3.5 Die Klemme — der Grund für deine Hörbeobachtung

Code (ResonatorProcessor.cpp:126-127):

```cpp
const float kB = juce::jlimit (0.005f, 2.0f,
    6.9077553f / (juce::MathConstants<float>::pi * f0 * t60));
```

Die Unterklemme 0,005 beendet die Skalierung: die maximale Ringdauer ist

    t60_max = ln(1000) / (π · f0 · 0,005)  ≈  440 / f0  Sekunden

(aus dem Code gerechnet, keine Messung):

| Note | f0 | t60_max |
|---|---|---|
| A4 | 440 Hz | **1,0 s** |
| A3 (Demo-Startton) | 220 Hz | **2,0 s** |
| C3 | 131 Hz | 3,4 s |
| A2 | 110 Hz | **4,0 s** |
| C2 | 65 Hz | 6,7 s |
| A1 (808-Kick-Körper) | 55 Hz | 8,0 s |

Decay darüber hinaus = **Dead Zone**: das Signal ist identisch. Mit `spread 2`
und vollem Accent (v = 1) verdoppelt sich das wirksame t60 — der Regler
saturiert dann schon bei 220/f0 (A3: **1,0 s**!). Deshalb hört man „3,5 vs. 10"
nichts.

Warum es die Klemme gibt, drei Gründe:
1. **Numerik**: kB → 0 heißt Polradius → 1; die float-Zustände driften über
   viele Sekunden, und Retriggern lässt die Energie kumulieren. 0,005 hält
   einen Sicherheitsabstand.
2. **Normierung**: kleinere kB macht den Ring NICHT lauter (die
   Gegenbewegung ~1/kB im Bandpass gleichen sich aus) — sie macht ihn nur
   länger.
3. **Drum-Praxis**: reale Körper ringen selten hörbar über ~1 s; der Anlass
   hier war 808-Nähe, nicht Ambient-Loops.

Ehrlich benannt: das ist auch eine **Design-Lücke** — der Parameterbereich
verspricht 0,01–10 s (PluginProcessor.cpp:657), realisierbar sind die vollen
10 s nur bei f0 ≤ ~44 Hz. Auch die Oberklemme 2 wirkt als Dead Zone (kurze
Ende): bei f0 = 220 Hz ist jeder decay ≤ 5 ms dasselbe Signal.

### 3.6 Q und Decay regeln VERSCHIEDENE Dinge

- **Q (Stage A)** = Eintritt: Bandbreite der Kopplung, Tinte des Anschlags,
  wieviel des Exciters in den Modus fällt.
- **Decay (Stage B)** = Austritt: die Ringdauer.

Hohe Q macht den Anschlag schmaler und farbig — **nicht** länger. (Indirekt
kann mehr eingefangene Energie den Ring lauter beginnen lassen; die Dauer sitzt
in kB.) Im Analog-Prototyp ist „Q ≈ π · f0 · τ" — hohe Q und langer Ring sind
für denselben Pol dieselbe Zahl; hier sind die beiden Dämpfungen bewusst
getrennt parametriert (kA für die Kopplung, kB für den Ring).

### 3.7 Der Rest des Knotens: vel/spread, Retrigger, Stimmensleben

- **vel (verbunden)**: `t_eff = t · (1 + v · (spread − 1))`, v = Stereo-Mittel
  auf [0,1]. Demo: spread 2,0 → der Accent verdoppelt die Ringdauer (808:
  „Decay über VIN"). Unverbunden: Port-Default 1,0 → byteidentisch neutral.
- **q_mod_in (verbunden)**: `q_eff = clamp(q · (1 + m), 0, 1)` mit
  m = Stereo-Mittel auf [−1, +1].
- **tune (verbunden)**: ersetzt das statische tune **komplett** (kein Offset —
  derselbe Vertrag wie der Filter mit cutoff_mod_in). Key-Tracking (note →
  tune), F-ADSR-Dives und FM-Ketten gliden den Körper so zipperfrei.
- **Retrigger**: `startNote()` behält die Zustände — ein neuer Treffer kippt
  Energie in den **nochklingenden** Körper (wie eine echte Trommel). Nur
  `reset()` (neuer Patch / neue Sample-Rate) löscht — kein Altschall
  über Instrument-Grenzen.
- **Stimmensleben**: `tailPeak` ist ein langsamer Peak-Follower über den
  Ausgang; `hasEnergy()` = tailPeak > 0,001. Die Stimme bleibt nach
  Key-Release so lange am Leben, bis der Ring unter −60 dB fällt — der
  decay-Parameter bestimmt also wirklich, **wann die Stimme endet**.
- **In der Stimme**: bis zu 8 Resonatoren parallel
  (`GraphNodeRegistry::maxResonators`), verdrahtet in `SynthVoice.cpp:575ff`.

## 4. Wie die Bausteine abweichen (ehrliche Liste)

- **Gelöst mit vorhandenen Bausteinen**: Körper + Anreger sind ein reguläres
  Patch (`resonator-demo.smolfm`, 5 Knoten); Accent auf Länge (vel → resonator)
  und Lautheit (vel → gain) parallel, wie im 808-Referenzmuster.
- **Design-Lücke (Dead Zone)**: die 0,005-Unterklemme erlaubt t60 max.
  440/f0 s, während das UI bis 10 s anbietet. Optionaler Fix: Unterklemme auf
  0,001 senken (~2200/f0 s) — numerisch noch sehr komfortabel.
- **Der Ring singt auf konstantem f0**: die leichte Frequenzsenkung des echten
  808-Körpers während des Ausklangs gibt es nicht — nur über eine
  tune-Port-Modulation (fadsr) nachbaubar.
- **Kein Bridged-T-/WDF-Netzwerk, kein Pulse-Shaper (Wright-Omega)**: ersetzt
  durch die TPT-Bandpass-Kaskade (dieses Teaching) bzw. eine kurze
  `AdsrProcessor`-Impulslinie (siehe tuned-strike-exciter-teaching.md).

## 5. Analog ↔ Digital (Übersetzungstabelle)

| Analog (TR-808) | Bedeutung | Digital in SmolFM |
|---|---|---|
| Bridged-T-Kette (Körpertyp) | gedämpft schwingender Körper | `ResonatorProcessor` Stage B (kB aus `decay`) |
| Kopplung des Trigger-Impulses | Bandbreite/Tinte, wieviel Energie in den Modus fällt | Stage A (kA aus `q`) |
| „Decay über VIN" (Accent) | Akzent verlängert die Zeitkonstante | `vel`-Port + `spread` (Demo: 2,0) |
| Tune-Potentiometer | Modus-Frequenz | `tune`-Port (Note) bzw. stat. `tune` |
| Ausklang des Körpers | −60-dB-Zeit | `resonator<N>Decay` = t60 (0,01–10 s, Default 0,3) |

## 6. Glossar (die neuen/unverständlichen Worte)

| Begriff | Bedeutung (einfach) | In SmolFM |
|---|---|---|
| Anreger (Exciter) | die kurze Energie, die den Körper zum Klingen bringt | osc0 + adsr0 (18-ms-Klick) in `resonator0.in` |
| komplexes Polpaar | die beiden Schwingungsspeicher, die nach dem Anschlag frei abklingen | TPT-Zustände ic1B/ic2B in `ResonatorProcessor.cpp` |
| Bandpass | Filter, das nur um die Resonanzfrequenz herum durchlässt | beide Stufen des Resonators (Filterkern-Modus BP) |
| TPT / trapezoidal | Integrationsverfahren, das analogtreu bleibt und ohne Versatz rückkoppelt | `tptBandpassStep`, g = tan(π·f0/fs) |
| Dämpfung k (kB) | wie stark die Schwingung pro Zeiteinheit geschwächt wird | `kB = jlimit(0.005, 2, ln1000/(π·f0·t60))`, ResonatorProcessor.cpp:126 |
| t60 | Zeit, bis der Pegel auf −60 dB (0,1 %) abgeklungen ist | `resonator<N>Decay`, Range 0,01–10 s, Default 0,3 |
| τ (Zeitkonstante) | Zeit, pro der der Pegel um Faktor e (−8,7 dB) sinkt | τ = decay/ln(1000) ≈ decay/6,91 |
| Peak-Normierung | ein Dauerton auf f0 geht mit Gewinn 1 durch | y = kB·v1B im Code |
| Klemme (jlimit) | erzwungener Unter-/Oberwert | kB-Klemme [0,005; 2] — Ursache des Einfrierens |
| Dead Zone | Reglerbereich, in dem nichts passiert | decay > 440/f0 s (bzw. < 1,1/f0 s) |
| spread (Vel Spread) | Accent-Stärke, um die Decay verlängert wird | `resonator<N>VelSpread`, 0,25–4, Demo 2,0 |
| Retrigger | neuer Treffer in einen nochklingenden Körper → Energie addiert sich | `startNote()` behält die Zustände |
| tailPeak / hasEnergy | Schätzung, ob noch Ring da ist; hält die Stimme am Leben | `ResonatorProcessor.h`, `SynthVoice::hasReasonToLive` |
| Peak-Follower | laufender Schätzer des Momentan-Maximums, der langsam zerfällt | `tailPeak`, wird pro Sample × 0,99998 zerfallen lassen |

## 7. Belegliste (Quellen der Zahlen)

- `src/processors/ResonatorProcessor.cpp:126-127` — kB-Formel und Klemmen.
- `src/PluginProcessor.cpp:655-657` — Parameterbereich decay (0,01–10 s, skew 0,5).
- `instruments/debug/resonator-demo.smolfm` — Kette und Werte (tune 220, q 0,9, decay 0,5, spread 2,0).
- `src/processors/ProcessorGraphTests.cpp:882-980` („Resonator ring + decay"):
  gemessen 03.10.2026 — 109 Nulldurchgänge in 0,125 s; RMS früh/spät = 5598.
- `src/processors/ProcessorGraphTests.cpp:982-1070` („Resonator vel-on-decay"):
  Grenze rmsAccented > 2,5 × rmsPlain, Rechnung sagt ~10× — Test bestanden (03.10.2026).