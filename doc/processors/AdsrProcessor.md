# AdsrProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

ADSR-Hüllkurve als reiner Filter im Signalfluss. Multipliziert das eingehende
Audiosignal mit dem Hüllkurvenwert und leitet das Ergebnis weiter — die
Weiterleitung ans Master-Ausgangssignal übernimmt der `MasterOutputProcessor`.
Wrappt `juce::ADSR`; Parameter werden bei Note-On gelesen (JUCE-Empfehlung) und
wirken daher auf die nächste Note.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 16 | `GraphNodeRegistry::maxAdsr` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Bis zu 16 Lautstärke-Blendregler pro Voice — je Layer einer, damit jeder Klanganteil sein eigenes Auf- und Abblenden bekommt. |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `in` | Input | `signal` | liest 0 → der Knoten gibt Stille weiter (die Hüllkurve läuft unsichtbar weiter) | **Rohsignal unter der Blende.** Das Signal wird nicht verformt, sondern nur in Lautstärke gewogen — wie ein Vorhang, der sich nach Kurvengang öffnet und schließt. | `InputPort input` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |
| `out` | Output | `signal` | — | **Geblendetes Signal:** Eingang × Hüllkurvenwert — trägt die Anschlags-Gestalt (Aufbau, Halt, Ausklingen) in den nachgeschalteten Bus. | `OutputPort output` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| Attack | `adsr<N>Attack` | Float, 0.001–5 s | Anstiegszeit vom Stillstand zum Peak: 1 ms knallt der Ton schlagartig rein (Schlagstock), Sekunden lassen ihn langsam aufschwellen (Bogenstrich). Wird bei Note-On gelesen und wirkt auf die nächste Note. | `std::atomic<float>* attack` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |
| Decay | `adsr<N>Decay` | Float, 0.001–5 s | Zeit, in der der Pegel vom Peak auf den Sustain-Halt zurückfällt — der „Nachfall" direkt hinter dem Einschlag. | `std::atomic<float>* decay` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |
| Sustain | `adsr<N>Sustain` | Float, 0–1 | Halt-Pegel, solange die Note klingt: 1 = volle Lautheit bis zum Loslassen, 0 = der Ton verklingt schon während des Decay. | `std::atomic<float>* sustain` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |
| Release | `adsr<N>Release` | Float, 0.001–10 s | Ausblendzeit nach Note-Off — wie lange der Ton hinter der losgelassenen Note noch nachhallt. | `std::atomic<float>* release` | [src/processors/AdsrProcessor.h](../../src/processors/AdsrProcessor.h) |

`<N>` = Instanzindex 0–3.

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| Envelope-Vorschau | Zeichnet die ADSR-Kurve aus den vier Slider-Werten; die vier Phasen teilen sich die Breite anteilig nach Dauer (linearer Attack, exponentieller Decay/Release via `quadraticTo`); Repaint bei jedem Slider-Drag | `AdsrPanel::CurveDisplay::paint` | [src/gui/components/AdsrPanel.cpp](../../src/gui/components/AdsrPanel.cpp) |
| Kurven-Refresh | `Slider::Listener` des Panels löst `curveDisplay.repaint()` bei Wertänderung aus | `AdsrPanel::sliderValueChanged` | [src/gui/components/AdsrPanel.cpp](../../src/gui/components/AdsrPanel.cpp) |
| *(Rest wie gehabt)* | vier Rotary-Slider (A/D/S/R) | `AdsrPanel::attackSlider` u. a. | [src/gui/components/AdsrPanel.h](../../src/gui/components/AdsrPanel.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $x_n$ | Eingangssample | Amplitude |
| $g_n$ | Hüllkurvenwert (Gain) | 0–1 |
| $A, D, R$ | Attack-, Decay-, Release-Zeit | s |
| $S$ | Sustain-Pegel | 0–1 |
| $y_n$ | Ausgangssample | Amplitude |

Funktionsablauf:

Bei Note-On wird die Hüllkurve mit $(A, D, S, R)$ parametrisiert und gestartet
(`noteOn`), bei Note-Off läuft die Release-Phase (`noteOff`). `juce::ADSR`
liefert pro Sample den Gain $g_n$ entlang der linearen Attack- und
exponentiellen Decay/Release-Segmente.

Pro Sample:

$$y_n = x_n \cdot g_n$$

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $A, D, S, R$ | `attack->load()` … `release->load()` → `juce::ADSR::Parameters` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | in `startNote()` gelesen, `envelope.setParameters(params)` |
| $g_n$ | `envelope.getNextSample()` → `env` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | JUCE-ADSR-State-Machine; Sample-Rate via `envelope.setSampleRate()` in `prepare()` |
| $x_n$ | `input.getSample()` → `sourceSample` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | Port lesen (Default 0.0f unverbunden) |
| $y_n$ | `sourceSample * env` → `output.setSample()` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | Multiplikation, Ergebnis in den Port |
| Note-Off | `noteOff()` → `envelope.noteOff()` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | aus `SynthVoice::stopNote` bei `allowTailOff` |
| Still-Reset | `reset()` → `envelope.reset()` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | aus `SynthVoice::applyConnectionPatch` — ein neues Patch erbt keinen hängenden Sustain (verlorenes Note-Off / gehaltenes Sustain-Pedal) |
| Aktiv-Flag | `isActive()` → `envelope.isActive()` | [AdsrProcessor.cpp](../../src/processors/AdsrProcessor.cpp) | Voice wird freigegeben, wenn Hüllkurve endet |
