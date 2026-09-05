# DelayProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Zeitbasierter Effekt: Verschiebt das Eingangssignal um eine wählbare Dauer
und füttert es skaliert zurück in den eigenen Verzögerungspuffer. Unterstützt
freie Millisekunden oder Sync an die Host-Tempo.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 2 | `GraphNodeRegistry::maxDelays` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `in` (`PortType::signal`) | `InputPort input` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |
| Output-Ports | `out` (`PortType::signal`) | `OutputPort output` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Time | `delay<N>TimeMs` | Float 1 – 2000 ms, Default 250 | `std::atomic<float>* timeMs` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |
| Feedback | `delay<N>Feedback` | Float 0 – 0.95, Default 0.3 | `std::atomic<float>* feedback` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |
| Mix | `delay<N>Mix` | Float 0 – 1, Default 0.3 | `std::atomic<float>* mix` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |
| Sync | `delay<N>SyncMode` | Bool, Default false | `std::atomic<float>* syncMode` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |
| Division | `delay<N>Division` | Choice 0-3 (1/2, 1/4, 1/8, 1/16), Default 1 | `std::atomic<float>* division` | [src/processors/DelayProcessor.h](../../src/processors/DelayProcessor.h) |

Der Prozessor benötigt zusätzlich den aktuellen Host-BPM, der über
`setHostTempo(double)` vom `AudioPluginAudioProcessor` (via `getPlayHead`)
und `SynthVoice::setHostTempo` eingeschleust wird.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| Host-Sync Toggle | Wechselt zwischen freiem ms-Wert und BPM-abhängiger Notendivision; deaktiviert das Time-Textfeld im Sync-Modus | `DelayComponent::syncButton` | [src/gui/components/DelayComponent.cpp](../../src/gui/components/DelayComponent.cpp) |

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
- **Tail-Energie-Bewertung:** `tailPeak` beobachtet parallel die Schreib-
  amplitude (Peak-Follower, Decay $\approx -7.7$ dB/s). Er dient der
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
| — (Observation) | `tailPeak` | [DelayProcessor.cpp](../../src/processors/DelayProcessor.cpp) | `jmax(tailPeak * 0.99998, abs(written))`; nur für Voice-Lifetime, kein Audio-Pfad |
