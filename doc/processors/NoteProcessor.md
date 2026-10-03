# NoteProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Wandelt die zuletzt gespielte MIDI-Note in eine Frequenz in Hertz um und stellt
sie als `frequency`-Quelle im Graph bereit. Es gibt genau eine Instanz; ihr
Output kann mehrere Konsumenten speisen (ein Output port teilt sich an viele
Inputs).

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 1 | `GraphNodeRegistry::maxNotes` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Exakt eine Noten-Quelle pro Voice — der gemeinsame Update-Anker aller tonhöhen-/anschlugs-empfindlichen Verbraucher. |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| *(keine)* | Input | — | — | Der Knoten braucht keinen Draht — die Voice drückt ihm die gespielte MIDI-Note und ihre Anschlagstärke direkt auf. | — | — |
| `out` | Output | `frequency` | — | **Spielpitch-Quelle.** Die Frequenz der zuletzt gespielten Note in Hz (12-TET-Stimmung, 440 Hz bei MIDI-Note 69) — der „Update-Anker" für Oszillator und FM-Ketten; 0 Hz, solange der Node nicht verdrahtet ist. Ein Output-Port darf sich an viele Inputs teilen. | `OutputPort output` | [src/processors/NoteProcessor.h](../../src/processors/NoteProcessor.h) |
| `vel` | Output | `velocity` | — | **Anschlag-Fernleitung.** Die Anschlagstärke (0–1) der aktuellen Note als velocity-Quelle — speist die `vel`-Ports von Gain, Waveshaper, FM, Filter, F-ADSR (Accent-Klangregulierung); unverdrahtet skalieren die Konsumenten neutral mit 1.0. | `OutputPort velOutput` | [src/processors/NoteProcessor.h](../../src/processors/NoteProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| *(keine APVTS-Parameter)* | — | — | Es gibt nichts einzustellen — die Werte kommen aus dem Spiel (gespielte MIDI-Note + Anschlag), nicht von Reglern. | `int currentNote`, `float currentFrequency` | [src/processors/NoteProcessor.h](../../src/processors/NoteProcessor.h) |

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(noch keine)* | statische Info-Anzeige, kein editierbares Control | `NoteNodeComponent` | [src/gui/components/NoteNodeComponent.h](../../src/gui/components/NoteNodeComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $n$ | MIDI-Notennummer | — |
| $f$ | Ausgangsfrequenz | Hz |
| $e$ | Enable-Flag (Note verdrahtet?) | bool |
| $v$ | Velocity (Anschlagstärke 0–1) der aktuellen Note | — |

Funktionsablauf:

Beim Notenanschlag wird die Frequenz nach der gleichtemperierten Stimmung
berechnet (JUCE: `juce::MidiMessage::getMidiNoteInHertz`):

$$f = 440 \cdot 2^{\frac{n - 69}{12}}$$

Pro Sample wird der Wert unverändert ausgegeben, solange der Node aktiviert ist:

$$out = \begin{cases} f & e = \text{true} \\ 0 & e = \text{false} \end{cases}$$

$e$ ist `false`, wenn im Patch keine Verbindung von `note<N>.out` dieser
Instanz existiert — die Instanz ist dann abgeschaltet und liefert 0 Hz.

Bei einem Notenanschlag setzt die Voice **alle** Instanzen auf dieselbe
MIDI-Note **und** dieselbe Velocity (`SynthVoice::startNote`). Der `vel`-
Ausgang liefert die Anschlagstärke $v \in [0,1]$ der aktuellen Note als
zentriertes Frame (Port-Typ `velocity`); unverbundene `vel`-Konsumenten
skalieren mit 1.0 (sound-invariant).

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $n$ | `currentNote` (`int`) | [NoteProcessor.h](../../src/processors/NoteProcessor.h) | gesetzt in `setMidiNoteNumber(int)` |
| $f$ | `currentFrequency` (`float`) | [NoteProcessor.cpp](../../src/processors/NoteProcessor.cpp) | `getMidiNoteInHertz(currentNote)`, Cast `double→float` |
| $e$ | `enabled` (`std::atomic<bool>`) | [NoteProcessor.h](../../src/processors/NoteProcessor.h) | `setEnabled(bool)` aus `SynthVoice::applyConnectionPatch`; atomar, da UI-Thread schreibt |
| $out$ | `processSample()` | [NoteProcessor.cpp](../../src/processors/NoteProcessor.cpp) | `isEnabled() ? currentFrequency : 0.0f`, dann `output.setSample(f)` |
| $v$ | `setVelocity(float)` → `currentVelocity` (atomic) | [NoteProcessor.h](../../src/processors/NoteProcessor.h) | gesetzt aus `SynthVoice::startNote`; `velOutput.setSample(centered(v))` pro Sample |
