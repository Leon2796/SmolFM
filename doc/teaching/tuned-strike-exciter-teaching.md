# Teaching: Der abgestimmte Anreger (Tuned Strike) — warum der echte 808-Anreger berechenbar ist

Stand: 03.10.2026. Erklärt mit den Bausteinen, die es in SmolFM wirklich gibt
(`OscillatorProcessor`, `AdsrProcessor`, `GainProcessor`, `AdderProcessor`,
`FilterProcessor`, `ResonatorProcessor`). Der Anlass: der Kick klang in seiner
Lautheit "fast schon random" — manchmal stark, manchmal schwach bei gleicher
Anschlagstärke. Die Diagnose: das ist keine Anschlag-Frage, sondern die Frage,
**was** den Korpus anregt. Ein Glossar hängt am Ende.

## 1. Abschnitt 1 — Was ein „tuned strike exciter" ist

Ein Anreger (Exciter) ist die kurze Energie, die einen schwingenden Körper
(`ResonatorProcessor`) überhaupt erst zum Klingen bringt. Bis jetzt schickt
unser Kick-Patch `noise` (rohes Rauschen, jede Abtastung ein neuer Zufallswert)
für 8 ms (`adsr0`, decay 0,008) in den Korpus. Der Korpus ist aber
schmalbandig: er akzeptiert nur Schwingungsanteile nahe seiner
Abstimmfrequenz (~49–65 Hz, effektiv ca. 15–30 Hz Bandbreite).

Daraus entsteht das "Random"-Gefühl: Ein 8-ms-Fenster Rauschen enthält die
Band-Energie **nur als Stichprobe**. Bandbreite × Fensterlänge (B·T) ist bei
uns grob 0,1–0,25 — unter einem Freiheitsgrad. Eine so kurze Stichprobe eines
Zufallssignals durch ein so schmales Filter = die eingefangene Ring-Energie
streut stark von Treffer zu Treffer (Faktor ~0,3 bis ~2), **trotz identischer
Anschlagstärke**. Digital gesprochen: wir tasten den Zufall zu kurz ab, als
dass sein Mittelwert zuverlässig ankommen würde.

Der **tuned strike exciter** dreht das um: der Anreger ist ein **kurzer
Sinus-Impuls auf der Abstimmfrequenz des Körpers** — also derselben Note, auf
die der `ResonatorProcessor` eingestimmt ist. Sinus in, Resonanzfrequenz
überein: Der Ring übernimmt die Energie **kohärent** (ausgerichtete Schwingung),
jede Anregung überträgt denselben Anteil. Hit-für-Hit berechenbar.

In SmolFM-Bausteinen (das Referenz-Muster, wie driftbox ihre Toms baut —
„sine + pitch envelope … plus noise head"):

```
note ──out (Note)──→ osc0: OscillatorProcessor (sine, pitch)   ← gleiche Note wie der Korpus
osc0 ──out──→ adsr0: AdsrProcessor (attack 0,001 / decay ~0,02) ← kurzer Impuls
adsr0 ──out──→ gain1: GainProcessor (Faktor ~2,0)               ← Impulspegel
gain1 ──out──→ resonator0 (in)                                  ← KOHERENTE Anregung
gain1 ──out──→ gain2 (0,3) → output                             ← Klick-Kopie (nur durch HP!)
```

Das Rauschen bleibt als **nur Attack-Klebeschicht** (kurz, mit eigenem
`FilterProcessor`-Hochpass ~1200 Hz, leise ~×0,2) oder fällt ganz weg — es ist
dann Klangfarbe-Gewürz, nicht mehr die Energie-Quelle.

## 2. Abschnitt 2 — Der Anreger in der Hardware (analog): warum die echte
Schaltung „zufallsfrei" ist

Die echte TR-808-Kick-Schaltung wird **nicht von Rauschen angeregt**. Sie wird
von einem **Trigger-Impuls** geschlagen: ein definierter Spannungsimpuls, der
durch die sogenannte Impulsformer-Stufe läuft. Voltage Drums beschreibt genau
diese Stufe:

> „A capacitor feeds 2 resistors through a clamp diode, and a closed-form
> Wright-omega equation solves that network for each sample."

Übersetzt: Ein Kondensator entlädt über zwei Widerstände, eine
Klemmdiode begrenzt die Spitze — und dieses Netzwerk ist so regelmäßig, dass
es sich **formel-exakt pro Abtastwert** berechnen lässt (die Wright-Omega-
Funktion ist die geschlossene Lösung dieser Lade-/Entladekurve). Das ist die
schärfste Form von Determinismus: keine Zufallsquelle im Anregungspfad, jede
Schwankung der Ausgangslautheit kommt NUR von der Anschlagspannung (Akzent-
Gesetz), nicht vom Zufall.

Analog vs. digital in einer Tabelle:

| Analog-Welt |Unsere SmolFM-Welt | Folge |
|---|---|---|
| Trigger-Impuls = definierte Ladekurve (Impulsformer) | 8-ms-Ausschnitt aus `noise` | wir tasten Zufall; echte Schaltung hat keinen |
| Ausgangsschwankung kommt von Bauteile-Toleranz (Stück zu Stück, nicht Hit zu Hit) | unsere Schwankung kommt aus der Anregungs-Stichprobe (Hit zu Hit) | deshalb „random" — bei Hardware wäre das ein Trimm-Thema, kein Per-Treffer-Spiel |
| Wright-Omega: exakte Formel pro Abtastwert | unsere Bausteine sind reine DSP-Gleichungen | deterministisch NUR wenn die Eingänge deterministisch sind |

Wichtig zum Begriff „deterministisch": Digital-Signalverarbeitung ist an sich
deterministisch — dieselben Eingabewerte erzeugen dieselben Ausgaben. Unser
"Random" steckt allein im `noise`-Baustein (jede Abtastung ein neuer
Zufallswert). Alles andere (Resonator, Schleife, Sättigung) rechnet exakt.

## 3. Abschnitt 3 — Die einfachsten Bauplan-Varianten: stabil, skalierbar, oder beides

Drei Varianten, alle NUR mit vorhandenen Bausteinen — von „immer gleich“ bis
„Zufall als Regler“:

**Variante A — der abgestimmte Sinus-Impuls (Zufallsanteil = 0, die drei
kleinsten Bausteine):**

```
note → OscillatorProcessor (sine, pitch) → AdsrProcessor (decay ~0,02 s) → GainProcessor → Korpus
```

Der Sinus-Teil sitzt exakt auf der Abstimmfrequenz (die Note taktet beide).
Kohärente Anregung heißt: der Korpus übernimmt jeden Impuls mit demselben
Wirkungsgrad — Hit-für-Hit dieselbe Ring-Energie, die Lautheit folgt nur dem
Anschlag. Das ist die einfachste zufallsfreie Version und das Referenz-Muster
(driftbox-Toms; 808-synth’s Klick = ebenfalls ein definierter Sinus-Impuls).
Ehrliche Kehrseite: bei Sechzehntel-Loops klingen die Treffer identisch
(bis die Anschlag-Akzente sie färben).

**Variante B — der Misch-Anreger (zufallsskalierbar — der eigentliche
Wunsch-Bau):**

```
   ┌→ OscillatorProcessor (sine, pitch) → AdsrProcessor (Impuls) → GainProcessor (1−r) ─┐
note                                                                                     → AdderProcessor → Korpus
   └→ OscillatorProcessor (noise)      → AdsrProcessor (Impuls) → FilterProcessor (Hochpass ~1200) → GainProcessor (r) ─┘
```

Zwei Anreger auf derselben Anschlag-Ebene, ein regelmäßiger und ein
zufälliger; die beiden `GainProcessor`-Faktoren setzen das Mischverhältnis.
r = 0 heißt Variante A (jede Lautheit exakt berechenbar), r ≈ 0,3 heißt
„Hand des Trommlers" (natürliche Streuung), r = 1 heißt der heutige Zustand
(voll zufällig). Die Streuung wächst hier **quadratisch mit r** (die
Rausch-Energie mischt sich mit r²), das Mischungsregler-Paar ist also die
Zufalls-Regelung — ein „Randomness"-Regler aus zwei Faktor-Werten, keine
Engine-Änderung.

**Variante C — das bandgleiche Rauschen (Zufall behalten, Streuung brechen):**

```
note → (cutoff_mod_in!)  OscillatorProcessor (noise) → AdsrProcessor (decay 0,03–0,06 s)
     → FilterProcessor (Bandpass, cutoff_mod_in ← die Note = Bandmitte = Korpus-Ton)
     → GainProcessor → Korpus
```

Der Rauschen-Impuls wird erst durch einen Bandpass **auf die Korpus-Frequenz
gefaltet** (dieser `FilterProcessor` nimmt den Cut off als Port — die Note
taktet ihn dynamisch!) und länger gelassen (30–60 ms). Jetzt gilt B·T ≈ 1–3:
Die Statistik des Anregungsfensters konvergiert, die Lautheits-Streuung fällt
auf wenige Prozent, das Rauschen aber bleibt Rauschen (die Schläge klingen
nicht identisch). Der Mittelweg „natürlich, aber berechenbar“.

Variante-A/B/C im Direktvergleich:

| | Streuung der Treffer-Lautheit | Identische Schläge? | Bausteine |
|---|---|---|---|
| heute (roh-Rauschen, 8 ms) | groß (Faktor ~0,3–2 zufällig) | unidentisch | so wie jetzt |
| A — abgestimmt | 0 | ja (identisch!) | osc sine + adsr + gain |
| B — Misch-Anreger | skalierbar (r²) | je nach r | + noise-Hochpass + Gain ×2 + Adder |
| C — bandgleich | klein (wenige %) | nein, natürlich | + Bandpass-Filter (mit note→cutoff!) + längerer Burst |

## 4. Wie die Bausteine abweichen (ehrliche Liste)

- Alle drei Varianten sind **mit vorhandenen Bausteinen baubar** — keine
  Engine-Änderung nötig; A und C sind klein (je +2 Leitung), B ist der
  umfangreichste (+4 Bausteine).
- **Was die Engine (noch) nicht kann:** ein *pro Treffer neu würfelndes*
  Mischverhältnis (r würde pro Hit neu auswürfeln) — die beiden
  `GainProcessor`-Faktoren in Variante B sind statisch; pro-Hit-Zufall in der
  Patch-Domäne kommt vom `noise`-Baustein selbst, nicht von einem Regler.
- Der analoge Impulsformer selbst (Wright-Omega-Netzwerk) existiert als
  Baustein nicht — seine Klangeeigenschaft (der Spitzen-Hub) nähern wir über
  die kurze `AdsrProcessor`-Impulslinie nach.

## 5. Glossar (die neuen/unverständlichen Worte)

| Begriff | Bedeutung (einfach) | In SmolFM |
|---|---|---|
| Anreger (Exciter) | die kurze Energie, die einen schwingenden Körper zum Klingen bringt | die 8-ms-`AdsrProcessor`-Leitung ins `ResonatorProcessor`-Eingang |
| Tuned Strike / abgestimmter Anreger | Anreger auf derselben Frequenz wie der Körper → koherent, berechenbar | `OscillatorProcessor` (sine, pitch-Modus) → `AdsrProcessor` → Korpus |
| deterministisch / stochastisch | gleiches Ergebnis bei gleicher Eingabe / enthält Zufall | Sinus-`OscillatorProcessor` = deterministisch; `noise`-Wellenform = stochastisch |
| B·T (Zeit-Bandbreite-Produkt) | Bandbreite × Beobachtungszeit: unter 1 = Stichprobe, über 1 = verlässlicher Mittelwert | unser Anregungsfenster (8 ms) × Korpus-Bandbreite (~15–30 Hz) |
| kohärente Anregung | die Anregungsschwingung läuft in Phase mit der Körperresonanz → voller Energietransfer | Sinus-Impuls auf der Korpus-Abstimmfrequenz |
| Streuung / Rayleigh-Verhalten | die statistische Hit-zu-Hit-Spreizung der eingefangenen Energie | das gemessene „manchmal laut, manchmal leise“ |
| Impulsformer (Pulse Shaper) | analoge Stufe: Kondensator → 2 Widerstände → Klemmdiode, macht aus dem Trigger den definierten Hub | bei uns angenähert als kurze `AdsrProcessor`-Impulslinie |
| Wright-Omega | die geschlossene mathematische Funktion, die dieses Lade-/Entlade-Netzwerk pro Abtastwert exakt löst | Voltage-80's Formel — bei uns nicht als Baustein nötig |
| Misch-Anreger (Blend) | regelmäßiger Anreger + Zufallsanreger, Mischverhältnis über zwei Faktoren | 2× `GainProcessor` ((1−r)/r) → `AdderProcessor` |
| Bandgleiche Anregung | Rauschen erst auf die Korpus-Frequenz bandgefiltert, länger gelassen | `FilterProcessor` mit `cutoff_mod_in` ← die Note + längerer `AdsrProcessor` |
| cutoff_mod_in | Frequenz-Eingang des `FilterProcessor`: anliegende Hz = die Cut off (Ersatz, kein Zusatz) | die Note → Bandpass = mitgestimmtes Rauschen |