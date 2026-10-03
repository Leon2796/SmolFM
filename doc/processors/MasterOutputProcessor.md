# MasterOutputProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Endstufe des per-Voice-Graphen. Mischt bis zu acht Signaleingänge zu einem
stereofähigen Ausgangsframe (kanalweise Summierung: L zu L, R zu R) und
skaliert das Ergebnis mit dem Mastervolume. Der höchste Pegel beider Kanäle
wird atomar publiziert, damit die UI eine Pegelanzeige zeigen kann.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 1 | `GraphNodeRegistry::maxMasterOutputs` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Exakt eine Endstufe pro Voice — der Sammelbus, durch den jede Voice zur Master-Leitung laufen muss. |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `in1` | Input | `signal` | liest 0 → trägt nichts zur Summe bei | **Layer-Eingang 1.** Eins der acht Signal-Zuläufe: fließt L zu L, R zu R in die Kanalsumme — wie ein belegter Mischpultkanal; ohne Kabel bleibt der Kanal frei und summiert still. | `inputs[0]` | [src/processors/MasterOutputProcessor.h](../../src/processors/MasterOutputProcessor.h) |
| `in2` | Input | `signal` | liest 0 → trägt nichts zur Summe bei | **Layer-Eingang 2.** Zweiter Kanalplatz des Sammelbus — unabhängig, kanalrein und frei für einen zweiten Signalstrang. | `inputs[1]` | [src/processors/MasterOutputProcessor.h](../../src/processors/MasterOutputProcessor.h) |
| `in3` | Input | `signal` | liest 0 → trägt nichts zur Summe bei | **Layer-Eingang 3.** Dritter Zulauf — parallel zur Summe, ohne Reihenfolgevorteil. | `inputs[2]` | [src/processors/MasterOutputProcessor.h](../../src/processors/MasterOutputProcessor.h) |
| `in4` | Input | `signal` | liest 0 → trägt nichts zur Summe bei | **Layer-Eingang 4.** Vierter Platz der Endstufe — auch er nur L zu L, R zu R. | `inputs[3]` | [src/processors/MasterOutputProcessor.h](../../src/processors/MasterOutputProcessor.h) |
| `in5` | Input | `signal` | liest 0 → trägt nichts zur Summe bei | **Layer-Eingang 5.** Fünfter Zulauf — weiterer belegbarer Kanalplatz. | `inputs[4]` | [src/processors/MasterOutputProcessor.h](../../src/processors/MasterOutputProcessor.h) |
| `in6` | Input | `signal` | liest 0 → trägt nichts zur Summe bei | **Layer-Eingang 6.** Sechster Kanalplatz der Summierung. | `inputs[5]` | [src/processors/MasterOutputProcessor.h](../../src/processors/MasterOutputProcessor.h) |
| `in7` | Input | `signal` | liest 0 → trägt nichts zur Summe bei | **Layer-Eingang 7.** Siebter Zulauf an der Endstufe. | `inputs[6]` | [src/processors/MasterOutputProcessor.h](../../src/processors/MasterOutputProcessor.h) |
| `in8` | Input | `signal` | liest 0 → trägt nichts zur Summe bei | **Layer-Eingang 8.** Achter und letzter Platz — danach muss gepoolt (Adder) werden. | `inputs[7]` | [src/processors/MasterOutputProcessor.h](../../src/processors/MasterOutputProcessor.h) |
| *(keiner — Endstufe)* | Output | — | — | Der MasterOutput speist direkt die Voice-Ausgabe; was hier hineinfließt, ist im Stereomaster des Instruments hörbar — ein letztes Sammeln, Blenden und Publizieren (Peak für die UI-Anzeige). | — | — |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| Master Level | `masterLevel` | Float, 0–1 (Default 0.8) | Gesamtlautheits-Regler der Voice (0–1): die letzte Blendoberfläche, bevor die summierten Layer zur Master-Leitung laufen; skaliert die komplette Kanalsumme gleichmäßig. | `std::atomic<float>* level` | [src/processors/MasterOutputProcessor.h](../../src/processors/MasterOutputProcessor.h) |

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(noch keine)* | Rotary-Slider (Level) + Pegelanzeige | `MasterOutputComponent::levelSlider`, `MasterOutputComponent::displayedPeak` | [src/gui/components/MasterOutputComponent.h](../../src/gui/components/MasterOutputComponent.h) |

Die Pegelanzeige pollt pro 30 Hz-Timer den höchsten Peak aller Voices über
`AudioPluginAudioProcessor::getMasterPeakLevel()`.

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | Index des gerade berechneten Samples | — |
| $x_{k,n}$ | Stereopaar am Eingang $k$ im Sample $n$ | Amplitude |
| $L$ | Mastervolume | — |
| $y_n$ | Ausgangs-Stereopaar $(y_{n,L},\, y_{n,R})$ | Amplitude |

Pro Sample (kanalweise):

$$
y_{n,L} = L \sum_{k=1}^{8} x_{k,n,L} \qquad y_{n,R} = L \sum_{k=1}^{8} x_{k,n,R}
$$

Kurz erklärt: Der Prozessor summiert alle acht Eingänge kanalweise (L zu L, R
zu R) und multipliziert mit dem Master-Slider. Unverbundene Eingänge liefern
0 und verändern die Summe nicht.

Die Pegelanzeige folgt einem Peak-Follower mit schnellem Attack und langsamem
Release (`smoothedPeak`), damit die Anzeige nicht flackert; der lautere der
beiden Kanäle bestimmt den angezeigten Peak.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $x_{k,n}$ | `inputs[k].getSample()` | [MasterOutputProcessor.cpp](../../src/processors/MasterOutputProcessor.cpp) | Port lesen (Stereopaar); Default 0 wenn unverbunden |
| $L$ | `level->load()` | [MasterOutputProcessor.cpp](../../src/processors/MasterOutputProcessor.cpp) | atomarer Read des APVTS-Werts |
| $y_n$ | `out` (Rückgabewert) | [MasterOutputProcessor.cpp](../../src/processors/MasterOutputProcessor.cpp) | kanalweise Summe × Level; Peak = max(|y_L|, |y_R|) in `peakLevel` publiziert |
