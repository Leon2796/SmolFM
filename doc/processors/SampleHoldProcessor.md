# SampleHoldProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Freezt das Eingangssignal auf einem einstellbaren Zeitraster ein (Sample-and-Hold /
Sample-Rate-Reduktion). Der Ausgang trägt für eine Rasterdauer dieselben eingefrorenen
L/R-Werte und nimmt dann eine frische Kopie des Eingangs — zeitlich eine Treppe. Dies ist
die Raster-Achse des Bitcrushers ebenso wie getaktete Mod-Schritte. Die Rasterfrequenz
läuft statisch in Hertz oder tempo-synchronisiert über eine Noten-Länge (gleiche
Sync-Schema-Familie wie der Delay). Stateless außer zwei Werten pro Kanal: das
eingefrorene Stereo-Paar und ein Zähler — kein Puffer, kein Feedback, kein Tail.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 8 | `GraphNodeRegistry::maxSampleHolds` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Bis zu 8 Raster-Einfrierstellen pro Voice — je Schrittgitter/Effekt-Ebene eine. |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `in` | Input | `signal` | liest 0 → eingefrorenes Paar bleibt 0 → stumm | **Eingang.** Das Signal, das auf dem Zeitraster eingefroren wird — wie eine Fotografie, die nur alle N Samples neu abgelichtet wird; dazwischen bleibt das letzte Bild stehen. | `InputPort input` | [src/processors/SampleHoldProcessor.h](../../src/processors/SampleHoldProcessor.h) |
| `rate_mod_in` | Input | `signal` | liest 0 → exakt die statische Rasterfrequenz (neutral) | **Raster-Fremdfahrer.** Multipliziert die eingestellte Rasterfrequenz mit ($\cdot\,(1+m)$, $m$ auf $[-1,+1]$ geclampt): LFO = dichter/weiter schwingendes Raster (Lo-fi-Wobble), ADSR = vom Grob- zum Feinraster öffnen. | `InputPort rateMod` | [src/processors/SampleHoldProcessor.h](../../src/processors/SampleHoldProcessor.h) |
| `out` | Output | `signal` | — | **Eingefrorenes Signal:** die Treppenform — derselbe Wert wiederholt sich $T$ Samples lang, dann springt das Raster zum nächsten frischen L/R-Paar. | `OutputPort output` | [src/processors/SampleHoldProcessor.h](../../src/processors/SampleHoldProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| Rate | `hold<N>Rate` | Float 0.5–20000 Hz, Default 1000 | Wie oft pro Sekunde ein neues Bild abgelichtet wird: hoch (nahe Sample-Rate) = fast transparent, ~8000 Hz = klassischer Lo-fi-Crush-Raster, wenige Hz = grobe Stufen, die den Klang „versteppen". | `std::atomic<float>* rateHz` | [src/processors/SampleHoldProcessor.h](../../src/processors/SampleHoldProcessor.h) |
| Sync | `hold<N>SyncMode` | Bool, Default false | Schnürung der Rasterfrequenz an das Host-Tempo statt an freie Hertz — die Treppenstufen sitzen auf dem Takt statt auf einer Stoppuhr. | `std::atomic<float>* syncMode` | [src/processors/SampleHoldProcessor.h](../../src/processors/SampleHoldProcessor.h) |
| Division | `hold<N>Division` | Choice 0–3 (1/2, 1/4, 1/8, 1/16), Default 1 | Der Taktanteil im Sync-Modus — eine Einfrierung pro Halbe, Viertel, Achtel oder Sechzehntel. | `std::atomic<float>* division` | [src/processors/SampleHoldProcessor.h](../../src/processors/SampleHoldProcessor.h) |

Der Prozessor benötigt zusätzlich den aktuellen Host-BPM, der über
`setHostTempo(double)` vom `AudioPluginAudioProcessor` (via `getPlayHead`)
und `SynthVoice::setHostTempo` eingepeist wird (gleicher Weg wie beim Delay).

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| Sync-Regel | Im Sync-Modus ist der Rate-Drehknopf deaktiviert (abgedunkelt), der Division-Combo nur dann aktiv — der Host-Takt übernimmt das Raster. | `SampleHoldComponent::rateSlider`, `syncButton`, `divisionBox` | [src/gui/components/SampleHoldComponent.h](../../src/gui/components/SampleHoldComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $x_n$ | Eingangssample (L/R-Paar) | Amplitude |
| $r$ | Rasterfrequenz (0.5–20000 Hz) | Hz |
| $m$ | Stereo-Mittel von `rate_mod_in`, geclampt $[-1,+1]$ | — |
| $B$ | Host-Tempo | BPM |
| $f$ | Divisionsfaktor $\in \{2,\,1,\,0.5,\,0.25\}$ für $\{1/2, 1/4, 1/8, 1/16\}$ | — |
| $r_{\text{eff}}$ | wirksame Rasterfrequenz | Hz |
| $T$ | Rasterperiode | Samples |
| $c_n$ | Rasterzähler | Samples |
| $h_n$ | eingefrorenes Stereo-Paar (Hold) | Amplitude |
| $y_n$ | Ausgangssample | Amplitude |

Rasterfrequenz (freier Modus):

$$r_{\text{eff}} = \max(10^{-3},\ r \cdot (1 + m))$$

Sync-Modus:

$$r_{\text{eff}} = \frac{1000}{\text{quarterMs} \cdot f}, \qquad \text{quarterMs} = \frac{60000}{B}$$

Periode und Rasterablauf pro Sample:

$$T = \max\!\left(1,\ \mathrm{round}\!\left(\frac{f_s}{r_{\text{eff}}}\right)\right)$$

$$
c_n \le 1 \;\Rightarrow\; h = x_n,\; c_{n+1} = T
\qquad\text{sonst}\qquad
c_{n+1} = c_n - 1
$$

$$y_n = h \quad (\text{L und R werden unabhängig gefroren})$$

Sonderfälle:

- **Unwired:** $x_n = 0$ → das eingefrorene Paar bleibt $0$ → stumm.
- **$r_{\text{eff}} \ge f_s$:** $T = 1$ → fast transparent (jedes Sample neu abgelichtet).
- **Ganzzahliges Raster:** $T$ ist eine ganzzahlige Sample-Zahl; Raten, die die
  Sample-Rate nicht glatt teilen, quantisieren auf ganze Samples — das ist die
  ehrliche Raster-Semantik (keine fraktionale Interpolation).
- **startNote:** Zähler auf 1, Hold auf 0 → das erste Sample der Note lichtet
  frisch ab; die Treppe beginnt immer auf der Note, nie mitten in einer Periode.
- **Sync an, kein Playhead:** $B = 120$ (Default).
- **$r_{\text{eff}} \to 10^{-3}$ Hz:** lange, aber endliche Einfrierung (Schutz gegen ein
  unendlich stehendes Bild bei modulierter Rate).

### Abschnitt 3b — Modulations-Input `rate_mod_in`

Optionaler Mod-Eingang für die **Rasterfrequenz** (PortType `signal`). Der
Stereo-Mittelwert wird auf $[-1,+1]$ geclampt und multipliziert die
statische Rasterfrequenz in beiden Modi (freie Hertz und Host-Sync):
$r_{\text{eff}} = r \cdot (1+m)$, anschließend per $\max(10^{-3},\cdot)$ gegen das
Stehenbleiben geschützt. Unverbunden ($m = 0$) liefert exakt die statische
Rasterfrequenz. Klangeffekte: langsames Perlin = driftender Lo-fi-Wobble,
Sinus-LFO = pulsierendes Schrittgitter, ADSR = vom Grob- zum Feinraster öffnen
(und zurück — der Klang „schärft" mit der Hüllkurve).
Port-ID-Sync: `GraphNodes`-Spec, XSD-Annotation (`hold`-complexType) und
`SynthVoice::resolveInput` tragen denselben Namen `rate_mod_in`.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x_n$ | `input.getSample()` | [SampleHoldProcessor.cpp](../../src/processors/SampleHoldProcessor.cpp) | Port lesen (Default 0.0f unverbunden) |
| $r$ | `rateHz->load()` | [SampleHoldProcessor.cpp](../../src/processors/SampleHoldProcessor.cpp) | Basis Rate in Hz (freier Modus) |
| $B$ | `currentBpm` | [SampleHoldProcessor.cpp](../../src/processors/SampleHoldProcessor.cpp) | Von `setHostTempo` (aus `getPlayHead()->getPosition()->getBpm`) |
| $f$ | `division->load()` → `factor` | [SampleHoldProcessor.cpp](../../src/processors/SampleHoldProcessor.cpp) | Branch-Tabelle $\{2, 1, 0.5, 0.25\}$ wie beim Delay |
| $r_{\text{eff}}$ | `rate` nach dem `rateMod`-Zweig | [SampleHoldProcessor.cpp](../../src/processors/SampleHoldProcessor.cpp) | `rate_mod_in` verbunden: $r_{\text{eff}} = r\cdot(1+m)$, $m$ = Stereo-Mittel geclampt $[-1,+1]$; danach `jmax(rate, 1.0e-3f)` |
| $T$ | `period` | [SampleHoldProcessor.cpp](../../src/processors/SampleHoldProcessor.cpp) | `jmax(1, round(sampleRate / rate))` |
| $c_n$ | `samplesUntilHold` | [SampleHoldProcessor.cpp](../../src/processors/SampleHoldProcessor.cpp) | Latch bei $\le 1$: Zähler auf $T$; sonst dekrementieren |
| $h_n$ | `held` | [SampleHoldProcessor.cpp](../../src/processors/SampleHoldProcessor.cpp) | `StereoFrame` (L/R unabhängig), gesetzt beim Latch; `startNote`/`prepare` setzen ihn auf 0 |
| $y_n$ | `output.setSample (held)` | [SampleHoldProcessor.cpp](../../src/processors/SampleHoldProcessor.cpp) | Reines Durchreichen des Holds — keine Mischung, kein Tail |