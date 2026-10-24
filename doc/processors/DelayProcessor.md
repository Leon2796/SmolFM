# DelayProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Zeitbasierter Effekt: Verschiebt das Eingangssignal um eine wählbare Dauer
und füttert es skaliert zurück in den eigenen Verzögerungspuffer. Unterstützt
freie Millisekunden oder Sync an die Host-Tempo.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 16 | `GraphNodeRegistry::maxDelays` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Bis zu 16 Echo-Kammern pro Voice — je Layer eine eigene Raumschattierung. |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `in` | Input | `signal` | liest 0 → Ausgang bleibt 0 | **Eingang.** Das Signal, das in die Echo-Kammer geschoben wird — es geht unverändert hinein und als zeitversetzte Kopie wieder hinaus. | `InputPort input` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |
| `time_mod_in` | Input | `signal` | liest 0 → exakt die statische Zeit (neutral, byteidentisch) | **Fremdfahrer der Echo-Zeit.** Multipliziert die eingestellte Verzögerung mit (1 + m): schneller Sinus bei kurzer Grundzeit stampft die Linie ins Flanger-Schimmern, langsame Kurven watscheln die Echos dub-mäßig — die Verschiebung selbst ist zugleich die Tonhöhe der Echos: die Zeit IST die Echo-Pitch. | `InputPort timeMod` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |
| `out` | Output | `signal` | — | **Mix aus Original und Echo(n):** trockenes Signal plus zeitversetzte, ggf. mehrfach zurückkehrende Kopie. | `OutputPort output` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| Time | `delay<N>TimeMs` | Float 1–2000 ms, Default 250 | Der Abstand zwischen Original und Echo in Millisekunden — wie weit die Stimme in den Raum geschoben wird; wenige ms schimmern, hunderte ms rufen deutlich zurück. | `std::atomic<float>* timeMs` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |
| Feedback | `delay<N>Feedback` | Float 0–0.95, Default 0 | Wie oft das Echo sich selbst wiederhören darf: 0 = nur ein einzelnes Echo (default, absichtlich ohne Rückkopplung), 0.95 = langer, fast ewiger Ausläufer (Sicherheitsdeckel gegen Aufschaukeln). | `std::atomic<float>* feedback` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |
| Mix | `delay<N>Mix` | Float 0–1, Default 0.3 | Mischhebel zwischen trockenem Original (0) und reinem Echo (1) — wie viel „Ferne" im Klang sitzt. | `std::atomic<float>* mix` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |
| Sync | `delay<N>SyncMode` | Bool, Default false | Schnürung der Echozeit an das Host-Tempo statt an freie Millisekunden — die Echos sitzen auf dem Takt statt einer Stoppuhr. | `std::atomic<float>* syncMode` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |
| Division | `delay<N>Division` | Choice 0–3 (1/2, 1/4, 1/8, 1/16), Default 1 | Der Taktanteil im Sync-Modus — von breiten Halben bis zum rastlosen Sechzehntel-Echo. | `std::atomic<float>* division` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |

Der Prozessor benötigt zusätzlich den aktuellen Host-BPM, der über
`setHostTempo(double)` vom `AudioPluginAudioProcessor` (via `getPlayHead`)
und `SynthVoice::setHostTempo` eingeschleust wird.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| Host-Sync Toggle | Wechselt zwischen freiem ms-Wert und BPM-abhängiger Notendivision; deaktiviert das Time-Textfeld im Sync-Modus | `DelayComponent::syncButton` | [src/gui/components/DelayComponent.cpp](../../src/gui/components/DelayComponent.cpp) |
| Effektivwert-Anzeige (Ghost-Needle/Band) am Time-Regler | zeigt bei Tastendruck die wirksame Verzögerungszeit in ms — Statischer Reglerwert skaliert über `time_mod_in` plus Puffer-Deckel; im Sync-Modus die BPM-berechnete Zeit (die Nadel deckt beide Modi auf derselben ms-Skala ab; Telemetrie: Min/Max-Fenster ≈ 5 ms, die Nadel folgt dem Fensterband mit Pendel-Ballistik) | `DelayComponent::timeOverlay` (Typ `ModIndicatorOverlay`, Quelle `DelayProcessor::getEffectiveTimeMsRange`) | [src/gui/components/ModIndicatorOverlay.h](../../src/gui/components/ModIndicatorOverlay.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | Sample-Index | — |
| $x_n$ | Eingangssignal am Port `in` | linear |
| $f_s$ | Abtastrate | Hz |
| $B$ | Host BPM (aus dem Playhead) | $\text{min}^{-1}$ |
| $m$ | Mix-Tiefe (0 … 1) | linear |
| $fb$ | Feedback-Koeffizient (0 … 0.95) | linear |
| $d_{\text{ms}}$ | Eingestellte Dauer in ms im Free-Modus | ms |
| $k$ | Division-Index 0…3 | — |
| $L$ | Verzögerungslänge in Samples | Samples |
| $w_n$ | Schreibposition im Ring-Puffer im Sample $n$ | Samples |
| $y_n$ | Ausgangssignal im Sample $n$ | linear |

**Verzögerungslänge**:

$$
L =
\begin{cases}
d_{\text{ms}} \cdot 10^{-3} f_s, & \text{Sync aus} \\[4pt]
\dfrac{60}{B} \cdot \text{factor}_k \cdot f_s, & \text{Sync an}
\end{cases}
$$

wobei $\text{factor}_k \in \{2, 1, 0.5, 0.25\}$ für $\{1/2, 1/4, 1/8, 1/16\}$.

**Puffer-Gleichung:** Der Ring-Puffer der Länge $M$ speichert an Position
$w_n$ die Summe aus trockenem Sample und rückgekoppeltem verzögertem Sample;
gelesen wird $L$ Samples hinter dem Schreibzeiger:

$$
b[w_n] = x_n + fb \cdot b[\,(w_n - L) \bmod M\,]
$$

$$
y_n = (1 - m) \cdot x_n + m \cdot b[\,(w_n - L) \bmod M\,]
$$

$$
w_{n+1} = (w_n + 1) \bmod M
$$

Sonderfälle:

- **Unwired:** $x_n = 0$ → Ausgang bleibt $0$.
- **$fb = 0$:** Einzelnes Echo, keine Wiederholung.
- **$fb \to 0.95$:** Sehr langer Nachhall (stabiles Limit in der UI gesetzt).
- **Sync an, kein Playhead:** $B = 120$ (Default).
- **Patch-Wechsel:** Beim Laden eines neuen Patches wird der Puffer geleert
  (`reset()`), damit kein Signal des alten Instruments stecken bleibt. Der
  Tail überlebt weiterhin Noten innerhalb eines Patches (siehe `startNote`).
- **Tail-Energie-Bewertung:** `tailPeak` folgt dem rezirkulierenden Feedback-
  Anteil $fb\cdot|wet|$ (Peak-Follower, Faktor 0.999 ≈ $-8.7$ dB/s), **nicht**
  dem geschriebenen Wert: Der Eingang wird ausgenommen, damit ein nach
  Note-Off weitertönender Eingang die Voice nicht ewig festhält. Er dient der
  Voice-Lifetime-Policy (Voice klingt aus, bis der Tail unter $\approx$
  $-60$ dB fällt) und verändert die Audiosignale nicht.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x$ | `input.getSample()` | [DelayProcessor.cpp](../../src/processors/DelayProcessor.cpp) | Port lesen; Default 0 |
| $B$ | `currentBpm` | [DelayProcessor.cpp](../../src/processors/DelayProcessor.cpp) | Von `setHostTempo` (aus `getPlayHead()->getPosition()->getBpm`) |
| $L$ | `getDelaySamples()` | [DelayProcessor.cpp](../../src/processors/DelayProcessor.cpp) | Branch: frei (`timeMs`) oder sync (`quarterMs * factor`) |
| $fb$ | `feedback->load()` | [DelayProcessor.cpp](../../src/processors/DelayProcessor.cpp) | Auf 0.95 gedeckelt via `jlimit` |
| $m$ | `mix->load()` | [DelayProcessor.cpp](../../src/processors/DelayProcessor.cpp) | 0..1 |
| $w_n$ | `writePos` | [DelayProcessor.cpp](../../src/processors/DelayProcessor.cpp) | Ring-Puffer modulo `buffer.size()` |
| $b[\cdot]$ | `buffer[...]` | [DelayProcessor.cpp](../../src/processors/DelayProcessor.cpp) | `std::vector<float>` allokiert in `prepare()`; keine Realzeit-Allokation |
| $y$ | `result` → `output.setSample(result)` | [DelayProcessor.cpp](../../src/processors/DelayProcessor.cpp) | Dry/Wet-Mischung |
| $L_{\text{eff}}$ | `ds` nach dem `timeMod`-Zweig | [DelayProcessor.cpp](../../src/processors/DelayProcessor.cpp) | `time_mod_in` verbunden: $L_{\text{eff}} = L\cdot(1+m_{\text{tm}})$, $m_{\text{tm}}$ = Stereo-Mittelwert geclampt $[-1,+1]$; `jlimit(1, M-2, …)` deckelt die Linienlänge |

### Abschnitt 3b — Modulations-Input `time_mod_in`

Optionaler Mod-Eingang für die **Verzögerungszeit** (PortType `signal`). Der
Stereo-Mittelwert wird auf $[-1,+1]$ geclampt und multipliziert die
effektive Verzögerungslänge in beiden Modi (freie Millisekunden und
Host-Sync): $L_{\text{eff}} = L\cdot(1+m)$, anschließend per
`jlimit(1, M-2, …)` in den Puffer gezwungen. Unverbunden ($m = 0$) liefert
exakt die statische Zeit. Klangeffekte: schneller Sinus bei kurzer
Grundzeit = **Flanger** (die Linie taucht in die Minimallänge); langsames
Perlin im Sync-Modus = menschliches Dub-Wobble; schneller Sinus auf langen
Zeiten = Vibrato auf den Echos (die Zeit IST die Echo-Pitch-Skala).
Port-ID-Sync: `GraphNodes`-Spec, XSD-Annotation (`delay`-complexType) und
`SynthVoice::resolveInput` tragen denselben Namen `time_mod_in`.
| — (Observation) | `tailPeak` | [DelayProcessor.cpp](../../src/processors/DelayProcessor.cpp) | `jmax(tailPeak * 0.999, fb * |wet|)`; trackt nur die rezirkulierende Linie (frischer Eingang ausgenommen), sonst hält ein nach Note-Off weitertönender Eingang die Voice für immer fest; nur für Voice-Lifetime, kein Audio-Pfad |
