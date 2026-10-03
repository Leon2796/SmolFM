# FrequencyScaleProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Verkettbare Frequenz-Skalierung in der Frequenzdomäne. Nimmt eine Frequenz in
Hertz entgegen und gibt sie mit einem konstanten Faktor multipliziert wieder in
Hertz aus. Zustandslos; die Phasenintegration liegt im Oszillator am Ende der
Kette.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 16 | `GraphNodeRegistry::maxFrequencyScales` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Bis zu 16 Frequenz-Verhältnis-Umschalter pro Voice — je Intervall/Quint-Lage eine Stufe. |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `freq_in` | Input | `frequency` | liest 0 Hz → 0 rein, 0 raus (die Stufe ist tonlos inert) | **Aufzustreckende Frequenz.** Die Hz-Zahl von `note.out` oder einer FM-Stufe, die mit einem Verhältnis multipliziert wird — wie ein Fotokopierer, der die Vorlage größer/kleiner druckt. | `InputPort freqInput` | [src/processors/FrequencyScaleProcessor.h](../../src/processors/FrequencyScaleProcessor.h) |
| `out` | Output | `frequency` | — | **Skalierte Frequenz:** Eingang × Intervallverhältnis × Faktor — die verschobene Spielpitch für den Downstream-Carrier. | `OutputPort output` | [src/processors/FrequencyScaleProcessor.h](../../src/processors/FrequencyScaleProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| Factor | `fscale<N>Factor` | Float, 0–10 (Default 1) | Der freiveränderliche Skalierer: 1 = neutral, 2 = Oktave hoch, 0,5 = Oktave runter, 0 = die Frequenz ruht (kein Ton fließt). | `std::atomic<float>* factor` | [src/processors/FrequencyScaleProcessor.h](../../src/processors/FrequencyScaleProcessor.h) |
| Interval | `fscale<N>Interval` | Choice: none, octave_up, octave_down, fifth_up, fifth_down, fourth_up, fourth_down, minor_third_up, major_third_up, semitone_up, semitone_down (Default none) | Der musikalische Kurzbefehl statt Zahlenraten: jeder Name ist ein exaktes 12-TET-Verhältnis (Quinte rauf ≈ 1,5); multipliziert sich mit dem Factor, „none" ist neutral. | `std::atomic<float>* interval` | [src/processors/FrequencyScaleProcessor.h](../../src/processors/FrequencyScaleProcessor.h) |

`<N>` = Instanzindex 0–7. Verkettung: `fscale<K>.out` → `fscale<K+1>.freq_in`.
Das Intervall ist der musikalische Kurzbefehl: der Name steht für ein exaktes
12-TET-Verhältnis (Tabelle `fscaleIntervalRatios` im Header); Intervall und
Faktor multiplizieren sich ($f_{out} = f_{in} \cdot r_i \cdot s$).

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| Rotary-Slider | Freier Faktor (0–10, Default 1) | `FrequencyScaleComponent::factorSlider` | [src/gui/components/FrequencyScaleComponent.h](../../src/gui/components/FrequencyScaleComponent.h) |
| Intervall-Dropdown | Musikalischer Kurzbefehl: Name → exaktes 12-TET-Verhältnis, multipliziert mit dem Faktor | `FrequencyScaleComponent::intervalBox` | [src/gui/components/FrequencyScaleComponent.h](../../src/gui/components/FrequencyScaleComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | Index des gerade berechneten Samples | — |
| $f_{in,n}$ | eingehende Frequenz im Sample $n$ | Hz |
| $r_i$ | Verhältnis der gewählten Intervall-Option ($r_i = 1$ bei „none", exakt $2^{k/12}$ sonst) | — |
| $s$ | Skalierungsfaktor aus dem Slider | — |
| $f_{out,n}$ | skalierte Ausgangsfrequenz im Sample $n$ | Hz |

Diese einzelne Stufe berechnet für jedes Sample:

$$
f_{out,n} = f_{in,n} \cdot r_i \cdot s
$$

Kurz erklärt:

- $f_{in,n}$ ist die Frequenz, die vom vorherigen Knoten kommt, zum Beispiel
  440 Hz von einer gespielten Note oder aus einer FM-Stufe.
- $r_i$ kommt aus dem Intervall-Dropdown; „none" liefert $r_i = 1$, „octave_up"
  $r_i = 2$, „fifth_up" $2^{7/12} \approx 1{,}4983$.
- $s$ ist der Faktor aus dem Slider, begrenzt auf 0 bis 10.
- Bei $r_i = 1$ und $s = 1$ bleibt die Frequenz unverändert — die Stufe ist
  transparent.
- Bei $s = 2$ klingt die Note eine Oktave höher, bei $s = 0{,}5$ eine Oktave
  tiefer. Bei $s = 0$ wird die Frequenz zu 0 Hz.

Beispiel mit $f_{in,n} = 440\,\mathrm{Hz}$ und $s = 2$: Die Ausgangsfrequenz
beträgt $f_{out,n} = 440 \cdot 2 = 880\,\mathrm{Hz}$.

Zur Veranschaulichung einer Kette: Wenn mehrere Skalierungsstufen
nacheinander verbunden sind, übernimmt jede Stufe den Ausgang der vorherigen
als ihre Eingangsfrequenz. Dadurch multiplizieren sich die Faktoren:

$$
f_{K,n} = f_{0,n} \prod_{k=1}^{K} (r_k \, s_k)
$$

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $f_{in}$ | `freqInput.getSample()` → `inputHz` | [FrequencyScaleProcessor.cpp](../../src/processors/FrequencyScaleProcessor.cpp) | Port lesen; Default 0 Hz wenn unverbunden (`setDefaultValue(0.0f)`) |
| $r_i$ | `fscaleIntervalRatioFromIndex (intervalIndex)` | [FrequencyScaleProcessor.h](../../src/processors/FrequencyScaleProcessor.h) | raw Choice-Index (round) → exaktes 12-TET-Verhältnis; „none"/out-of-range → 1 |
| $s$ | `factor->load() * fscaleIntervalRatioFromIndex (intervalIndex)` → `scale` | [FrequencyScaleProcessor.cpp](../../src/processors/FrequencyScaleProcessor.cpp) | Free-range-Faktor multipliziert mit dem Intervallverhältnis, pro Sample |
| $f_{out}$ | `scaledHz` → `output.setSample(scaledHz)` | [FrequencyScaleProcessor.cpp](../../src/processors/FrequencyScaleProcessor.cpp) | `inputHz * scale`; float-Multiplikation |
