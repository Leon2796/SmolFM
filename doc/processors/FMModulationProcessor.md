# FMModulationProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Verkettbare FM-Stufe in der Frequenzdomäne. Nimmt die Carrier-Frequenz in Hertz
entgegen und einen Modulator als Audiosignal, und gibt die momentan modulierte
Frequenz wieder in Hertz aus. Zustandslos; die Phasenintegration liegt im
Oszillator am Ende der Kette.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 4 | `GraphNodeRegistry::maxFmAmounts` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | `freq_in` (`PortType::frequency`), `modulator_in` (`PortType::signal`) | `InputPort freqInput`, `InputPort modulatorInput` | [src/processors/FMModulationProcessor.h](../../src/processors/FMModulationProcessor.h) |
| Output-Ports | `out` (`PortType::frequency`) | `OutputPort output` | [src/processors/FMModulationProcessor.h](../../src/processors/FMModulationProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| FM Amount | `fmAmount<N>` | Float, 0–10 | `std::atomic<float>* fmAmount` | [src/processors/FMModulationProcessor.h](../../src/processors/FMModulationProcessor.h) |

`<N>` = Instanzindex 0–3. Verkettung: `fm<K>.out` → `fm<K+1>.freq_in`.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(noch keine)* | Rotary-Slider (Amount) | `FMModulationComponent::fmSlider` | [src/gui/components/FMModulationComponent.h](../../src/gui/components/FMModulationComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | Index des gerade berechneten Samples | — |
| $f_{in,n}$ | eingehende Carrier-Frequenz im Sample $n$ | Hz |
| $a$ | FM Amount aus dem Slider | — |
| $m_n$ | Modulatorsample am Eingang `modulator_in` im Sample $n$ | normalerweise -1 bis 1 |
| $c$ | Modulator ist verbunden | bool |
| $\widetilde{m}_n$ | wirksames Modulatorsample im Sample $n$ | normalerweise -1 bis 1 |
| $f_{out,n}$ | momentane Ausgangsfrequenz im Sample $n$ | Hz |

Diese einzelne FM-Stufe berechnet für jedes Sample:

$$
f_{out,n} = f_{in,n} \left(1 + a \widetilde{m}_n\right)
$$

Dabei ist $\widetilde{m}_n$ das wirksame Modulatorsample:

$$
\widetilde{m}_n =
\begin{cases}
m_n, & a > 0 \land c \\
0, & \neg(a > 0 \land c)
\end{cases}
$$

$$
f_{out,n} = f_{in,n} \left(1 + a \tilde{m}_n\right)
$$

Kurz erklärt:

- $f_{in,n}$ ist die Frequenz, die vom vorherigen Knoten kommt, zum Beispiel
  440 Hz von einer gespielten Note.
- $m_n$ ist der aktuelle Ausschlag des angeschlossenen Audiosignals. Bei einer
  Sinuswelle bewegt er sich normalerweise zwischen $-1$ und $1$.
- $a \widetilde{m}_n$ bestimmt die relative Abweichung. Der Term
  $1 + a \widetilde{m}_n$ ist der Faktor, mit dem $f_{in,n}$ multipliziert wird.
- Bei $a = 0$ oder wenn $c$ falsch ist, setzt der Prozessor
  $\widetilde{m}_n$ auf $0$. Der Faktor ist dann $1$, also bleibt die Frequenz
  unverändert.

Beispiel mit $f_{in,n} = 440\,\mathrm{Hz}$, $a = 0{,}5$ und
$\widetilde{m}_n = -1$: Der Faktor ist $1 + 0{,}5 \cdot (-1) = 0{,}5$; die
Ausgangsfrequenz beträgt daher $f_{out,n} = 440 \cdot 0{,}5 = 220\,\mathrm{Hz}$.
Bei $\widetilde{m}_n = 1$ wären es $660\,\mathrm{Hz}$.

Ein Amount $a = 1$ lässt ein wirksames Modulatorsample $\widetilde{m}_n$ von
$-1$ bis $1$ die Frequenz von 0 % bis 200 % der Eingangsfrequenz bewegen. Bei
größeren Amounts kann $f_{out,n}$ negativ werden. Der Oszillator kann das verarbeiten, indem seine
Phase rückwärts läuft.

Warum das FM ist: Diese Stufe verändert die Frequenz für jedes einzelne
Sample. Der nachgeschaltete Oszillator verwendet die jeweils neue Frequenz für
sein Phasen-Increment und integriert sie über die Zeit. Das ist
Frequenzmodulation in diesem Signalfluss, nicht ein direkter Eingriff dieser
Stufe in die Phase.

**Stabilität bei nicht-sinusförmigen Wellenformen**: Der Oszillator begrenzt
die Momentanfrequenz auf den Bereich [0, Nyquist] (siehe
[OscillatorProcessor.md](OscillatorProcessor.md)).  Bei Sinus-Carriern sind
auch extreme FM-Deviations unkritisch, da die Sinusfunktion stetig ist.  Bei
Saw/Square/Tri-Carriern führen große negative Frequenzausschläge oder sehr
hohe Momentanfrequenzen zu Diskontinuitäten im Wellenverlauf, die als harsch
klingendes Rauschen hörbar werden.  Für saubere Ergebnisse mit nicht-sinusförmigen
Carriern sollten FM-Amounts moderat gewählt werden (typisch < 1.5 bei Saw).

Zur Veranschaulichung einer Kette: Wenn mehrere FM-Stufen nacheinander mit
eigenen Modulatoren verbunden sind, übernimmt jede Stufe den Ausgang der
vorherigen als ihre Eingangsfrequenz. Dadurch multiplizieren sich ihre Faktoren:

$$
f_{K,n} = f_{0,n} \prod_{k=1}^{K} \left(1 + a_k \tilde{m}_{k,n}\right)
$$

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $f_{in}$ | `freqInput.getSample()` → `carrierHz` | [FMModulationProcessor.cpp](../../src/processors/FMModulationProcessor.cpp) | Port lesen; Default 0 Hz wenn unverbunden (`setDefaultValue(0.0f)`) |
| $a$ | `fmAmount->load()` → `amount` | [FMModulationProcessor.cpp](../../src/processors/FMModulationProcessor.cpp) | atomarer Read des APVTS-Werts, pro Sample |
| $c$ | `modulatorInput.isConnected()` | [FMModulationProcessor.cpp](../../src/processors/FMModulationProcessor.cpp) | atomarer Pointer-Check (`std::atomic<OutputPort*>`) |
| $\tilde{m}$ | `modulator` (lokal, ternär) | [FMModulationProcessor.cpp](../../src/processors/FMModulationProcessor.cpp) | `modulatorInput.getSample()` nur wenn `amount > 0 && isConnected()`, sonst `0.0f` |
| $f_{out}$ | `instantHz` → `output.setSample(instantHz)` | [FMModulationProcessor.cpp](../../src/processors/FMModulationProcessor.cpp) | `carrierHz * (1.0f + amount * modulator)`; float-Multiplikation |
