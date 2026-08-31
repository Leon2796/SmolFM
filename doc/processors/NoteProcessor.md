# NoteProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Wandelt die zuletzt gespielte MIDI-Note in eine Frequenz in Hertz um und stellt
sie als `frequency`-Quelle im Graph bereit. Alle Instanzen liefern dieselbe
gespielte Note; mehrere Boxen können so verschiedene Ketten speisen.

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | 4 | `GraphNodeRegistry::maxNotes` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) |
| Input-Ports | *(keine)* | — | — |
| Output-Ports | `out` (`PortType::frequency`) | `OutputPort output` | [src/processors/NoteProcessor.h](../../src/processors/NoteProcessor.h) |

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| *(keine APVTS-Parameter)* | — | MIDI-Note → Hz | `int currentNote`, `float currentFrequency` | [src/processors/NoteProcessor.h](../../src/processors/NoteProcessor.h) |

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

Funktionsablauf:

Beim Notenanschlag wird die Frequenz nach der gleichtemperierten Stimmung
berechnet (JUCE: `juce::MidiMessage::getMidiNoteInHertz`):

$$f = 440 \cdot 2^{\frac{n - 69}{12}}$$

Pro Sample wird der Wert unverändert ausgegeben, solange der Node aktiviert ist:

$$out = \begin{cases} f & e = \text{true} \\ 0 & e = \text{false} \end{cases}$$

$e$ ist `false`, wenn im Patch keine Verbindung von `note<N>.out` dieser
Instanz existiert — die Instanz ist dann abgeschaltet und liefert 0 Hz.

Bei einem Notenanschlag setzt die Voice **alle** Instanzen auf dieselbe
MIDI-Note (`SynthVoice::startNote`).

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $n$ | `currentNote` (`int`) | [NoteProcessor.h](../../src/processors/NoteProcessor.h) | gesetzt in `setMidiNoteNumber(int)` |
| $f$ | `currentFrequency` (`float`) | [NoteProcessor.cpp](../../src/processors/NoteProcessor.cpp) | `getMidiNoteInHertz(currentNote)`, Cast `double→float` |
| $e$ | `enabled` (`std::atomic<bool>`) | [NoteProcessor.h](../../src/processors/NoteProcessor.h) | `setEnabled(bool)` aus `SynthVoice::applyConnectionPatch`; atomar, da UI-Thread schreibt |
| $out$ | `processSample()` | [NoteProcessor.cpp](../../src/processors/NoteProcessor.cpp) | `isEnabled() ? currentFrequency : 0.0f`, dann `output.setSample(f)` |
