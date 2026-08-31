# FrequencyScaleProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Verkettbare Frequenz-Skalierung in der Frequenzdomäne. Nimmt eine Frequenz in
Hertz entgegen und gibt sie mit einem konstanten Faktor multipliziert wieder in
Hertz aus. Zustandslos; die Phasenintegration liegt im Oszillator am Ende der
Kette.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 4 | `GraphNodeRegistry::maxFrequencyScales` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `freq_in` (`PortType::frequency`) | `InputPort freqInput` | [src/processors/FrequencyScaleProcessor.h](../../src/processors/FrequencyScaleProcessor.h) |
| Output-Ports | `out` (`PortType::frequency`) | `OutputPort output` | [src/processors/FrequencyScaleProcessor.h](../../src/processors/FrequencyScaleProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| Factor | `fscale<N>Factor` | Float, 0–10 (Default 1) | `std::atomic<float>* factor` | [src/processors/FrequencyScaleProcessor.h](../../src/processors/FrequencyScaleProcessor.h) |

`<N>` = Instanzindex 0–3. Verkettung: `fscale<K>.out` → `fscale<K+1>.freq_in`.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(noch keine)* | Rotary-Slider (Faktor) | `FrequencyScaleComponent::factorSlider` | [src/gui/components/FrequencyScaleComponent.h](../../src/gui/components/FrequencyScaleComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | Index des gerade berechneten Samples | — |
| $f_{in,n}$ | eingehende Frequenz im Sample $n$ | Hz |
| $s$ | Skalierungsfaktor aus dem Slider | — |
| $f_{out,n}$ | skalierte Ausgangsfrequenz im Sample $n$ | Hz |

Diese einzelne Stufe berechnet für jedes Sample:

$$
f_{out,n} = f_{in,n} \cdot s
$$

Kurz erklärt:

- $f_{in,n}$ ist die Frequenz, die vom vorherigen Knoten kommt, zum Beispiel
  440 Hz von einer gespielten Note oder aus einer FM-Stufe.
- $s$ ist der Faktor aus dem Slider, begrenzt auf 0 bis 10.
- Bei $s = 1$ bleibt die Frequenz unverändert — die Stufe ist transparent.
- Bei $s = 2$ klingt die Note eine Oktave höher, bei $s = 0{,}5$ eine Oktave
  tiefer. Bei $s = 0$ wird die Frequenz zu 0 Hz.

Beispiel mit $f_{in,n} = 440\,\mathrm{Hz}$ und $s = 2$: Die Ausgangsfrequenz
beträgt $f_{out,n} = 440 \cdot 2 = 880\,\mathrm{Hz}$.

Zur Veranschaulichung einer Kette: Wenn mehrere Skalierungsstufen
nacheinander verbunden sind, übernimmt jede Stufe den Ausgang der vorherigen
als ihre Eingangsfrequenz. Dadurch multiplizieren sich die Faktoren:

$$
f_{K,n} = f_{0,n} \prod_{k=1}^{K} s_k
$$

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $f_{in}$ | `freqInput.getSample()` → `inputHz` | [FrequencyScaleProcessor.cpp](../../src/processors/FrequencyScaleProcessor.cpp) | Port lesen; Default 0 Hz wenn unverbunden (`setDefaultValue(0.0f)`) |
| $s$ | `factor->load()` → `scale` | [FrequencyScaleProcessor.cpp](../../src/processors/FrequencyScaleProcessor.cpp) | atomarer Read des APVTS-Werts, pro Sample |
| $f_{out}$ | `scaledHz` → `output.setSample(scaledHz)` | [FrequencyScaleProcessor.cpp](../../src/processors/FrequencyScaleProcessor.cpp) | `inputHz * scale`; float-Multiplikation |
