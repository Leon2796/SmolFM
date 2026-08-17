# JUCE FM-Synthesizer – Implementierungsplan für einen Coding-Agent

## Originalanforderung

Baue mit JUCE einen einfachen Synthesizer mit UI, der:

* FM-Synthese anwenden kann, mit einem Carrier und einem Modulator
* Eine ADSR-Hüllkurve besitzt
* Eine UI mit Slidern besitzt, welche die Frequenz von Carrier und Modulator steuern können
* Für jeden Oszillator Radio-Buttons für die Auswahl der Signalquelle besitzt:

  * Sine
  * Saw
  * Square

Die Funktion soll zunächst **minimal gehalten werden**, aber die Architektur soll so aufgebaut sein, dass sie später einfach erweitert werden kann.

Der gesamte Code muss **vollständig und C++-einsteigerfreundlich dokumentiert** werden. Kommentare sollen insbesondere erklären, warum etwas gemacht wird und wie die Audioverarbeitung funktioniert – nicht nur wiederholen, was eine Codezeile offensichtlich tut.

---

# 1. Ziel des Projekts

Erstelle einen kleinen polyphonen FM-Synthesizer als JUCE-Audio-Plugin.

Der Synthesizer besitzt pro Voice:

```text
MIDI Note
    │
    ├──────────────► Carrier Oscillator ──────┐
    │                                         │
    │              FM                       ▼
    └──► Modulator Oscillator ─────► Carrier Phase
                                             │
                                             ▼
                                        ADSR Envelope
                                             │
                                             ▼
                                          Output
```

Die grundlegende FM-Gleichung soll konzeptionell sein:

```text
carrier(t) = waveformCarrier(
    carrierPhase(t)
    + modulationIndex * modulator(t)
)
```

Die Implementierung muss nicht mathematisch unnötig kompliziert werden.

Wichtig ist zunächst:

1. Zwei Oszillatoren.
2. Ein Oszillator moduliert die Phase/Frequenz des anderen.
3. Beide Oszillatoren können Sine, Saw oder Square verwenden.
4. Carrier- und Modulator-Frequenz sind per UI steuerbar.
5. Die Gesamtlautstärke wird durch eine ADSR-Hüllkurve gesteuert.
6. MIDI-Noten triggern die Voices.
7. Mehrere Voices sollen gleichzeitig funktionieren.

---

# 2. JUCE-Version und Dokumentationsgrundlage

Verwende die aktuell installierte/stabile JUCE-Version des Projekts.

Orientiere dich an der aktuellen offiziellen JUCE-Dokumentation, insbesondere an:

* `juce::Synthesiser`
* `juce::SynthesiserVoice`
* `juce::SynthesiserSound`
* `juce::dsp::Oscillator<float>`
* `juce::ADSR`
* `juce::AudioProcessorValueTreeState`
* `juce::AudioProcessorValueTreeState::SliderAttachment`
* `juce::AudioProcessorValueTreeState::ButtonAttachment`
* `juce::AudioProcessorEditor`
* `juce::Slider`
* `juce::ToggleButton` bzw. geeignete JUCE-Button-Klassen

Die aktuelle JUCE-Dokumentation beschreibt `Synthesiser` als Container für Voices und Sounds. Die Audioerzeugung findet in `SynthesiserVoice` statt.

`juce::dsp::Oscillator` soll verwendet werden, weil es eine eigene periodische Funktion akzeptiert und damit die drei gewünschten Wellenformen einfach abbilden kann.

Für die Hüllkurve soll `juce::ADSR` verwendet werden. Sie stellt `setSampleRate()`, `setParameters()`, `noteOn()`, `noteOff()` und `getNextSample()` bereit.

Für die Parameterverwaltung soll `AudioProcessorValueTreeState` eingesetzt werden. JUCE stellt dafür insbesondere `SliderAttachment` zur Verfügung.

---

# 3. Architektur

Verwende zunächst folgende einfache Struktur:

```text
PluginProcessor
│
├── AudioProcessorValueTreeState parameters
│
├── juce::Synthesiser synthesiser
│       │
│       ├── SynthVoice
│       │     ├── Carrier Oscillator
│       │     ├── Modulator Oscillator
│       │     └── ADSR
│       │
│       ├── SynthVoice
│       ├── SynthVoice
│       └── ...
│
└── PluginEditor
        │
        ├── Carrier Frequency Slider
        ├── Modulator Frequency Slider
        ├── FM Amount Slider
        │
        ├── Carrier Waveform Buttons
        │   ├── Sine
        │   ├── Saw
        │   └── Square
        │
        ├── Modulator Waveform Buttons
        │   ├── Sine
        │   ├── Saw
        │   └── Square
        │
        ├── Attack Slider
        ├── Decay Slider
        ├── Sustain Slider
        └── Release Slider
```

Die Architektur soll bewusst einfach bleiben.

Vermeide zunächst:

* komplizierte DSP-Graphen
* unnötige abstrakte Factory-Klassen
* Dependency Injection
* komplexe GUI-Frameworks
* eigene Parameterverwaltung
* eigene State-Speichermechanismen
* unnötige Thread-Abstraktionen

Die Architektur soll aber so geschrieben werden, dass später beispielsweise weitere Operatoren oder Filter hinzugefügt werden können.

---

# 4. Dateien

Erstelle zunächst folgende Dateien:

```text
PluginProcessor.h
PluginProcessor.cpp

PluginEditor.h
PluginEditor.cpp

SynthVoice.h
SynthVoice.cpp

SynthSound.h
SynthSound.cpp
```

Optional kann später eine Datei hinzukommen:

```text
OscillatorWaveform.h
```

Diese soll aber erst angelegt werden, wenn dadurch die Verständlichkeit tatsächlich verbessert wird.

---

# 5. SynthSound

Erstelle eine einfache Klasse:

```cpp
class SynthSound : public juce::SynthesiserSound
```

Die Klasse beschreibt lediglich den Sound, den die Voices abspielen können.

Sie soll:

* alle MIDI-Noten akzeptieren
* alle MIDI-Kanäle akzeptieren
* keine eigene Audioberechnung enthalten

Implementiere insbesondere:

```cpp
bool appliesToNote (int midiNoteNumber) override;
bool appliesToChannel (int midiChannel) override;
```

Beide Funktionen können für diesen ersten Synthesizer einfach `true` zurückgeben.

Erkläre im Kommentar, warum `SynthSound` selbst keinen Audio-Code enthält:

> `SynthSound` beschreibt, welche Sounds gespielt werden können. Die eigentliche Audioerzeugung übernimmt `SynthesiserVoice`.

Das entspricht dem von JUCE vorgesehenen Modell.

---

# 6. SynthVoice

Erstelle:

```cpp
class SynthVoice : public juce::SynthesiserVoice
```

Die Voice besitzt:

```cpp
juce::dsp::Oscillator<float> carrierOscillator;
juce::dsp::Oscillator<float> modulatorOscillator;

juce::ADSR adsr;
```

Zusätzlich benötigst du:

```cpp
double sampleRate = 44100.0;

float currentVelocity = 0.0f;
float currentCarrierFrequency = 440.0f;
float currentModulatorFrequency = 440.0f;
float currentFMAmount = 0.0f;
```

Die Parameter sollen später nicht als dauerhaft kopierte Werte behandelt werden, wenn dies vermeidbar ist. Die Voice soll die aktuellen Plugin-Parameter aus einer geeigneten gemeinsamen Parameterstruktur erhalten.

Für die erste Implementierung darf die Voice einen Verweis auf den Parameter-State bzw. eine klar definierte Parameter-Schnittstelle bekommen.

---

# 7. Waveforms

Beide Oszillatoren müssen diese drei Signalformen unterstützen:

```text
Sine
Saw
Square
```

Definiere eine kleine Enumeration:

```cpp
enum class Waveform
{
    sine,
    saw,
    square
};
```

Für jede Waveform soll eine mathematische Funktion definiert werden.

## Sine

```cpp
std::sin (phase)
```

## Saw

Verwende eine einfache mathematische Sawtooth-Funktion.

Wichtig:

Die Funktion muss den Eingang im Bereich `-pi ... +pi` berücksichtigen, weil `juce::dsp::Oscillator` die periodische Funktion mit diesem Phasenbereich verwendet.

## Square

Eine einfache Variante:

```cpp
phase < 0.0f ? -1.0f : 1.0f
```

Die Wellenformen müssen nicht bandlimitiert sein.

Das ist für Version 1 akzeptabel.

Dokumentiere ausdrücklich, dass diese einfache Variante bei hohen Frequenzen Aliasing erzeugen kann und dass bandbegrenzte Oszillatoren später ergänzt werden könnten.

---

# 8. Oscillator-Initialisierung

Initialisiere beide `juce::dsp::Oscillator<float>` mit einer geeigneten periodischen Funktion.

Verwende:

```cpp
oscillator.initialise (...);
```

und:

```cpp
oscillator.prepare (spec);
```

Die JUCE-Dokumentation sieht `prepare()` vor der Audioverarbeitung vor und bietet `setFrequency()` zur Frequenzsteuerung.

Die Initialisierung darf nicht unnötig pro Audioblock neu aufgebaut werden.

---

# 9. MIDI Note → Frequenz

In `startNote()`:

```cpp
double frequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
```

Die MIDI-Notenfrequenz ist zunächst die Basis für den Carrier.

Der Modulator erhält seine Frequenz ebenfalls aus der MIDI-Note, multipliziert mit dem vom Benutzer eingestellten Verhältnis bzw. einer geeigneten Frequenzeinstellung.

Für die erste Version kann die UI direkt Frequenzwerte anbieten.

Beispiel:

```text
Carrier Frequency:
0.25x – 4.0x

Modulator Frequency:
0.25x – 8.0x
```

Empfohlen ist allerdings, intern mit Frequenzverhältnissen zu arbeiten:

```text
carrierFrequency = midiFrequency * carrierRatio
modulatorFrequency = midiFrequency * modulatorRatio
```

Das macht den Synthesizer musikalischer und transponierbar.

Die UI darf die Werte als Ratio darstellen:

```text
Carrier:    1.00x
Modulator:  2.00x
```

Wenn die Anforderung ausdrücklich absolute Hz-Slider verlangt, kann alternativ direkt in Hz gearbeitet werden.

---

# 10. FM-Implementierung

Der wichtigste Teil ist die FM-Synthese.

Für jedes Sample:

1. Modulator-Sample erzeugen.
2. Dieses Sample für die Phasenmodulation des Carriers verwenden.
3. Carrier-Sample erzeugen.
4. ADSR auf das Ergebnis anwenden.
5. In den Output schreiben.

Konzeptionell:

```cpp
float modulatorSample = modulatorOscillator.processSample (0.0f);

float modulation = modulatorSample * fmAmount;

float carrierSample =
    carrierOscillator.processSample (modulation);
```

Falls die gewählte JUCE-Oszillator-Schnittstelle nicht exakt diese Form der Phasenmodulation erlaubt, implementiere die minimale eigene Phase-Accumulation für die Voice.

Wichtig:

Der Agent darf nicht blind davon ausgehen, dass `processSample(input)` bei `juce::dsp::Oscillator` bereits genau die gewünschte FM-Architektur darstellt.

Prüfe die aktuelle JUCE-Dokumentation und implementiere die mathematisch korrekte Phasenmodulation.

---

# 11. Empfehlung für die eigentliche FM-Engine

Für maximale Verständlichkeit ist es sogar sinnvoll, die eigentliche FM-Berechnung zunächst als kleine eigene Klasse zu kapseln:

```cpp
class SimpleOscillator
{
public:
    void prepare (double sampleRate);
    void setFrequency (float frequency);
    void setWaveform (Waveform waveform);

    float getNextSample (float phaseModulation = 0.0f);

private:
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    Waveform waveform = Waveform::sine;
};
```

Damit wird die FM-Logik für einen C++-Anfänger deutlich transparenter.

Die Klasse soll:

```text
Phase
  ↓
Waveform Function
  ↓
Sample
```

implementieren.

Bei jedem Sample:

```cpp
phase += phaseIncrement;
```

und für den Carrier:

```cpp
float output = waveformFunction (phase + phaseModulation);
```

Dadurch wird die eigentliche FM-Mathematik explizit sichtbar.

`juce::dsp::Oscillator` kann trotzdem als Referenz oder für spätere Erweiterungen betrachtet werden. Für die minimalste Lernarchitektur ist eine kleine eigene Oszillator-Klasse jedoch zulässig und möglicherweise verständlicher.

---

# 12. ADSR

Jede `SynthVoice` besitzt eine eigene:

```cpp
juce::ADSR adsr;
```

Beim Start einer Note:

```cpp
adsr.noteOn();
```

Beim Ende:

```cpp
adsr.noteOff();
```

Bei jedem Sample:

```cpp
float envelope = adsr.getNextSample();
```

und:

```cpp
outputSample *= envelope;
```

JUCE verlangt, dass vor Verwendung die Sample Rate gesetzt wird. Die ADSR stellt außerdem `setParameters()` mit Attack, Decay, Sustain und Release bereit.

Die Parameter:

```text
Attack
Decay
Sustain
Release
```

sollen aus dem zentralen Parameter-State kommen.

Wichtig: Die aktuelle JUCE-Dokumentation weist darauf hin, dass ADSR-Parameter während der Wiedergabe nicht einfach verändert werden sollen. Der Agent soll deshalb eine sichere Strategie verwenden, z. B. die ADSR-Parameter beim Start einer Note bzw. an geeigneten sicheren Stellen aktualisieren und bei Änderungen während einer laufenden Release-Phase korrekt `reset()` berücksichtigen.

---

# 13. stopNote()

Implementiere:

```cpp
void stopNote (float velocity, bool allowTailOff) override
```

Wenn:

```cpp
allowTailOff == true
```

dann:

```cpp
adsr.noteOff();
```

Die Voice darf anschließend weiterlaufen, bis die Release-Phase beendet ist.

In `renderNextBlock()`:

```cpp
if (! adsr.isActive())
{
    clearCurrentNote();
    return;
}
```

JUCE verlangt bei einer beendeten Voice `clearCurrentNote()`, damit die Voice wieder für andere Noten verfügbar wird.

---

# 14. Polyphonie

Im `PluginProcessor`:

```cpp
juce::Synthesiser synth;
```

füge beispielsweise 8 Voices hinzu:

```cpp
for (int i = 0; i < 8; ++i)
    synth.addVoice (new SynthVoice (...));
```

und einen Sound:

```cpp
synth.addSound (new SynthSound());
```

JUCE unterstützt Polyphonie über mehrere `SynthesiserVoice`-Objekte.

Die Voice-Anzahl soll als einfache Konstante definiert werden:

```cpp
constexpr int numberOfVoices = 8;
```

---

# 15. Parameter-System

Verwende:

```cpp
juce::AudioProcessorValueTreeState parameters;
```

mit einem `ParameterLayout`.

Die Parameter sollen ungefähr folgende IDs erhalten:

```text
carrierRatio
modulatorRatio
fmAmount

carrierWaveform
modulatorWaveform

attack
decay
sustain
release
```

Optional:

```text
outputLevel
```

Die Parameter-IDs müssen stabil bleiben.

Verwende keine GUI-Komponenten als Quelle der Wahrheit.

Die Architektur muss sein:

```text
Parameter
   ↓
AudioProcessorValueTreeState
   ├── Audio Engine
   └── GUI
```

nicht:

```text
Slider
   ↓
Audio Engine
```

JUCE beschreibt `AudioProcessorValueTreeState` ausdrücklich als zentrale Verwaltung des Processor-Zustands und seiner Parameter.

---

# 16. Parameter-Typen

Für numerische Parameter:

```cpp
juce::AudioParameterFloat
```

verwenden.

Für Waveform-Auswahl ist ein Choice-Parameter sinnvoll:

```cpp
juce::AudioParameterChoice
```

mit:

```text
Sine
Saw
Square
```

Dadurch muss nicht versucht werden, drei voneinander unabhängige boolesche Parameter synchron zu halten.

Intern:

```text
0 = Sine
1 = Saw
2 = Square
```

Die UI stellt diese Auswahl trotzdem als drei Radio-Buttons dar.

---

# 17. UI

Die UI soll bewusst einfach aussehen.

Empfohlene Struktur:

```text
-----------------------------------------
|             SIMPLE FM SYNTH            |
-----------------------------------------

 CARRIER                MODULATOR
 Frequency              Frequency
 [ slider ]             [ slider ]

 [Sine] [Saw] [Square]  [Sine] [Saw] [Square]

 FM AMOUNT
 [---------------- slider ----------------]

 ADSR

 Attack   Decay   Sustain   Release
 [---]    [---]   [---]     [---]


-----------------------------------------
```

Jeder Oszillator MUSS seine eigenen drei Radio-Buttons besitzen:

### Carrier

```text
( ) Sine
( ) Saw
( ) Square
```

### Modulator

```text
( ) Sine
( ) Saw
( ) Square
```

Es darf nicht nur eine gemeinsame Waveform-Auswahl geben.

---

# 18. Radio-Button-Verhalten

Verwende drei `ToggleButton`s pro Oszillator.

Die Buttons müssen sich jeweils gegenseitig ausschließen.

Verwende dafür JUCEs Button-Group-/Radio-Group-Mechanismus.

Alternativ kann bei einer sehr einfachen Implementierung ein `AudioParameterChoice` verwendet und die drei Buttons als UI-Repräsentation dieses Parameters behandelt werden.

Wichtig:

```text
Carrier Radio Group
    Sine
    Saw
    Square

Modulator Radio Group
    Sine
    Saw
    Square
```

Diese beiden Gruppen müssen unabhängig voneinander funktionieren.

---

# 19. Slider

Verwende `juce::Slider`.

Die Slider sollen mit:

```cpp
juce::AudioProcessorValueTreeState::SliderAttachment
```

verbunden werden.

Beispielsweise:

```cpp
std::unique_ptr<
    juce::AudioProcessorValueTreeState::SliderAttachment>
    carrierRatioAttachment;
```

JUCEs `SliderAttachment` hält Slider und Parameter automatisch synchron.

Für die UI müssen deshalb keine manuellen `sliderValueChanged()`-Callbacks verwendet werden, um Plugin-Parameter zu setzen.

---

# 20. Empfohlene Parameterbereiche

Verwende sinnvolle Bereiche:

### Carrier Ratio

```text
0.25 – 4.0
Default: 1.0
```

### Modulator Ratio

```text
0.25 – 8.0
Default: 1.0
```

### FM Amount

```text
0.0 – 10.0
Default: 0.0
```

### Attack

```text
0.001 – 5.0 Sekunden
Default: 0.01
```

### Decay

```text
0.001 – 5.0 Sekunden
Default: 0.2
```

### Sustain

```text
0.0 – 1.0
Default: 0.8
```

### Release

```text
0.001 – 10.0 Sekunden
Default: 0.5
```

Für Attack/Decay/Release ist eine logarithmische bzw. sinnvoll skalierten Slider-Darstellung zu bevorzugen, damit kleine Zeiten gut einstellbar sind.

---

# 21. AudioProcessor

`PluginProcessor` übernimmt:

```text
MIDI
 ↓
Synthesiser
 ↓
Voices
 ↓
Output
```

In `prepareToPlay()`:

1. Sample Rate speichern.
2. `Synthesiser::setCurrentPlaybackSampleRate()` aufrufen.
3. Voices entsprechend vorbereiten.

JUCE weist ausdrücklich darauf hin, dass der Synthesizer vor dem Rendering seine aktuelle Playback-Sample-Rate erhalten muss.

In `processBlock()`:

1. Audio-Buffer löschen bzw. korrekt vorbereiten.
2. MIDI-Daten an den Synthesizer weitergeben.
3. `renderNextBlock()` aufrufen.
4. Ausgangspegel kontrollieren.

---

# 22. Audio-Thread-Regeln

Der Agent muss besonders auf Echtzeit-Sicherheit achten.

Innerhalb von:

```cpp
processBlock()
renderNextBlock()
startNote()
stopNote()
```

dürfen keine unnötigen Operationen ausgeführt werden, die den Audio-Thread blockieren könnten.

Insbesondere vermeiden:

```text
std::cout
Datei-I/O
Locks
Speicherallokationen pro Sample
GUI-Zugriffe
```

Parameterwerte sollen effizient gelesen werden.

Die Voice-Rendering-Funktionen müssen möglichst deterministisch und klein bleiben.

---

# 23. Parameterzugriff in Voices

Bevorzuge eine einfache gemeinsame Parameterstruktur, z. B.:

```cpp
struct SynthParameters
{
    std::atomic<float>* carrierRatio;
    std::atomic<float>* modulatorRatio;
    std::atomic<float>* fmAmount;

    std::atomic<float>* carrierWaveform;
    std::atomic<float>* modulatorWaveform;

    std::atomic<float>* attack;
    std::atomic<float>* decay;
    std::atomic<float>* sustain;
    std::atomic<float>* release;
};
```

Falls dies die Lesbarkeit unnötig verschlechtert, darf stattdessen eine kleine Parameter-Zugriffsklasse erstellt werden.

Wichtig ist:

**Die GUI darf nicht direkt auf die Voice zugreifen.**

Die Datenflussrichtung lautet:

```text
UI
 ↓
APVTS
 ↓
Voice
```

---

# 24. State-Speicherung

Der Synthesizer soll seinen Zustand speichern und wiederherstellen können.

Verwende dafür:

```cpp
AudioProcessorValueTreeState
```

sowie die üblichen:

```cpp
getStateInformation()
setStateInformation()
```

im `AudioProcessor`.

Die JUCE-Dokumentation empfiehlt `AudioProcessorValueTreeState` ausdrücklich für Parameter-State und dessen Verbindung zur Plugin-Oberfläche.

---

# 25. C++-Einsteigerfreundlichkeit

Der gesamte Code muss didaktisch geschrieben werden.

Jede zentrale Klasse bekommt am Anfang eine kurze Erklärung:

```cpp
/**
    SynthVoice erzeugt eine einzelne Note unseres Synthesizers.

    Eine Voice besitzt ihren eigenen Carrier, Modulator und ihre eigene
    ADSR-Hüllkurve.

    Dadurch können mehrere Voices gleichzeitig unterschiedliche MIDI-Noten
    spielen.
*/
```

Ebenso wichtige Funktionen kommentieren:

```cpp
/**
    Wird von JUCE aufgerufen, wenn eine MIDI-Note startet.

    Hier setzen wir die Frequenzen der beiden Oszillatoren und starten
    die ADSR-Hüllkurve.
*/
```

Nicht jede triviale Getter-/Setter-Zeile kommentieren.

Kommentare sollen vor allem erklären:

* Audiofluss
* FM-Prinzip
* Voice-Lebenszyklus
* MIDI → Frequenz
* ADSR
* Parameterfluss
* Threading-/Realtime-Aspekte

---

# 26. Erweiterbare Architektur

Obwohl das Projekt minimal bleiben soll, soll die Architektur später folgende Erweiterungen ermöglichen:

```text
Version 1
Carrier
Modulator
ADSR
        ↓
Version 2
+ Output Gain
+ Filter
        ↓
Version 3
+ zweiter Modulator
        ↓
Version 4
+ Modulator Envelope
        ↓
Version 5
+ LFO
        ↓
Version 6
+ mehrere FM-Operatoren
```

Deshalb sollte die Voice nicht so geschrieben werden, dass Carrier und Modulator überall als hart codierte Sonderfälle auftauchen.

Eine spätere Struktur wie:

```cpp
struct Operator
{
    SimpleOscillator oscillator;
    float level;
    float ratio;
};
```

soll möglich bleiben.

Für Version 1 ist jedoch ausdrücklich **nur ein Carrier + ein Modulator** zu implementieren.

---

# 27. Keine unnötigen Features

Nicht implementieren:

* Preset Browser
* Reverb
* Delay
* Chorus
* Filter
* LFO
* Arpeggiator
* Unison
* Portamento
* MPE
* Pitch Bend
* Mod Wheel
* Automation UI
* Custom Skin
* Spectrum Analyzer

Diese Dinge sind ausdrücklich außerhalb des Scopes.

Der Agent soll nicht versuchen, aus dem kleinen Synthesizer ein vollständiges kommerzielles Instrument zu machen.

---

# 28. Entwicklungsreihenfolge

Implementiere das Projekt in folgenden Phasen.

## Phase 1 – Minimaler Audio-Output

Zunächst:

```text
MIDI Note
→ Carrier
→ Output
```

Nur Sine.

Test:

* MIDI Note C3 erzeugt einen hörbaren Ton.
* MIDI Note C4 erzeugt die doppelte Frequenz.

---

## Phase 2 – Voice-System

Implementieren:

```text
Synthesiser
SynthesiserSound
SynthesiserVoice
```

8 Voices hinzufügen.

Test:

* mehrere Noten gleichzeitig spielen.

---

## Phase 3 – Modulator

Zweiten Oszillator hinzufügen.

Test:

```text
FM Amount = 0
```

muss ungefähr wie ein normaler Carrier klingen.

Dann:

```text
FM Amount > 0
```

muss das Klangspektrum hörbar verändern.

---

## Phase 4 – Waveforms

Carrier:

```text
Sine
Saw
Square
```

Modulator:

```text
Sine
Saw
Square
```

implementieren.

Jede Auswahl unabhängig testen.

---

## Phase 5 – ADSR

ADSR hinzufügen.

Testfälle:

```text
Attack = kurz
Decay = kurz
Sustain = 0.8
Release = lang
```

Note-On und Note-Off müssen klar hörbar sein.

---

## Phase 6 – Parameter-State

`AudioProcessorValueTreeState` vollständig integrieren.

Test:

1. Parameter verändern.
2. Plugin schließen.
3. Plugin-State speichern.
4. Plugin neu laden.
5. Parameter müssen wiederhergestellt sein.

---

## Phase 7 – UI

UI hinzufügen:

```text
Carrier Frequency
Modulator Frequency
FM Amount

Carrier Waveform
Modulator Waveform

Attack
Decay
Sustain
Release
```

Alle numerischen Parameter über `SliderAttachment` verbinden.

---

# 29. Akzeptanzkriterien

Das Projekt gilt erst als fertig, wenn alle folgenden Punkte erfüllt sind:

* [ ] JUCE-Projekt kompiliert ohne Fehler.
* [ ] Plugin kann von einer DAW geladen werden.
* [ ] MIDI-Noten erzeugen Audio.
* [ ] Mindestens 8 Voices funktionieren polyphon.
* [ ] Carrier-Oszillator funktioniert.
* [ ] Modulator-Oszillator funktioniert.
* [ ] FM Amount = 0 erzeugt keine FM-Modulation.
* [ ] FM Amount > 0 verändert das Spektrum.
* [ ] Carrier kann Sine auswählen.
* [ ] Carrier kann Saw auswählen.
* [ ] Carrier kann Square auswählen.
* [ ] Modulator kann Sine auswählen.
* [ ] Modulator kann Saw auswählen.
* [ ] Modulator kann Square auswählen.
* [ ] Carrier- und Modulator-Waveform sind unabhängig.
* [ ] Carrier-Frequenz kann verändert werden.
* [ ] Modulator-Frequenz kann verändert werden.
* [ ] Attack funktioniert.
* [ ] Decay funktioniert.
* [ ] Sustain funktioniert.
* [ ] Release funktioniert.
* [ ] Note-Off startet die Release-Phase.
* [ ] Eine Voice wird nach abgeschlossener Release-Phase freigegeben.
* [ ] Parameter sind über `AudioProcessorValueTreeState` verwaltet.
* [ ] UI bleibt mit den Parametern synchron.
* [ ] Plugin-State kann gespeichert und wiederhergestellt werden.
* [ ] Keine unnötigen Echtzeit-allokierenden Operationen im Audio-Rendering.
* [ ] Code ist vollständig C++-einsteigerfreundlich kommentiert.
* [ ] Keine unnötigen Features außerhalb des Scopes wurden hinzugefügt.

---

# 30. Wichtige Implementierungsentscheidung

Bevor du Code schreibst, entscheide dich bewusst zwischen:

### Option A – `juce::dsp::Oscillator`

Vorteil:

```text
JUCE-native
weniger eigener DSP-Code
```

### Option B – kleiner eigener `SimpleOscillator`

Vorteil:

```text
FM-Prinzip vollständig transparent
Phasenmodulation leicht verständlich
Waveform-Logik vollständig kontrollierbar
```

Für dieses Lernprojekt ist **Option B zu bevorzugen**, sofern die Implementierung sauber und klein bleibt.

Der Agent soll aber die aktuelle JUCE-Dokumentation von `juce::dsp::Oscillator` berücksichtigen und nicht aufgrund veralteter JUCE-Beispiele eine falsche API verwenden.

---

# 31. Definition of Done

Am Ende soll kein bloßes Code-Skelett entstehen.

Der Agent soll ein **kompilierbares Minimalprojekt** liefern.

Dabei soll der Code so strukturiert sein, dass ein C++-Anfänger anhand der Dateien nachvollziehen kann:

```text
Wie kommt MIDI in den Synth?
        ↓
Wie wird daraus eine Voice?
        ↓
Wie wird die Frequenz berechnet?
        ↓
Wie erzeugt der Carrier Samples?
        ↓
Wie moduliert der Modulator den Carrier?
        ↓
Wie wird die ADSR angewendet?
        ↓
Wie landet das Ergebnis im Audio-Output?
        ↓
Wie steuert die GUI die Parameter?
```

Die Implementierung soll lieber **klein, klar und korrekt** sein als umfangreich.

Wenn eine Designentscheidung zwischen maximaler Erweiterbarkeit und Verständlichkeit besteht, hat bei Version 1 die **Verständlichkeit Vorrang**, solange eine spätere Erweiterung nicht unnötig erschwert wird.

----

# 32. Build- und Export-Dokumentation für die VST-Datei

Erstelle zusätzlich eine Markdown-Datei:

```text
BUILD.md
```

Diese Datei muss einem C++-/JUCE-Einsteiger vollständig erklären, **wie aus dem Quellcode eine fertige VST3-Datei gebaut und anschließend in einer DAW getestet wird**.

Die Dokumentation muss sich auf den tatsächlich verwendeten Build-Aufbau des Projekts beziehen und darf keine hypothetischen oder nicht verwendeten Build-Schritte beschreiben.

## 32.1 Build-System dokumentieren

Beschreibe:

* welche JUCE-Version verwendet wird
* welche C++-Version benötigt wird
* welches Build-System verwendet wird, z. B. CMake
* welche Compiler unterstützt werden
* welche Betriebssysteme unterstützt werden
* welche zusätzlichen Abhängigkeiten benötigt werden
* welche Dateien für den Build relevant sind

Wenn das Projekt CMake verwendet, erkläre insbesondere:

```text
CMakeLists.txt
JUCE-Verzeichnis bzw. JUCE-Abhängigkeit
Source/
Build/
```

und die Beziehung dieser Komponenten.

## 32.2 Voraussetzungen

Dokumentiere die notwendigen Installationen.

Beispielsweise:

### Windows

* Visual Studio
* C++ Desktop Development Workload
* CMake
* JUCE

### macOS

* Xcode
* Command Line Tools
* CMake
* JUCE

### Linux

* geeigneter C++-Compiler
* CMake
* notwendige Entwicklungsbibliotheken
* JUCE

Nur Plattformen dokumentieren, die das Projekt tatsächlich unterstützt.

## 32.3 Projekt konfigurieren

Beschreibe den vollständigen Ablauf:

```text
Quellcode
    ↓
CMake konfigurieren
    ↓
Build-Verzeichnis erzeugen
    ↓
Projektdateien/Buildsystem generieren
    ↓
Plugin kompilieren
```

Gib konkrete Terminalbefehle für die jeweilige Plattform an.

Beispielstruktur:

```bash
cmake -B build
cmake --build build --config Release
```

Falls das Projekt andere Befehle benötigt, müssen diese an die tatsächliche `CMakeLists.txt` angepasst werden.

## 32.4 VST3 als Build-Ziel

Erkläre, dass das Projekt als **VST3-Plugin** gebaut wird.

Dokumentiere:

* den Namen des VST3-Targets
* wo die fertige `.vst3`-Datei nach dem Build liegt
* ob die Datei/der Bundle-Ordner auf dem jeweiligen Betriebssystem anders strukturiert ist
* welche Build-Konfiguration verwendet werden soll

Bevorzugt soll für die Verteilung bzw. den normalen DAW-Test:

```text
Release
```

verwendet werden.

## 32.5 Plugin-Installationspfad

Dokumentiere die üblichen VST3-Installationspfade.

Beispielsweise:

### Windows

```text
C:\Program Files\Common Files\VST3\
```

### macOS

```text
/Library/Audio/Plug-Ins/VST3/
```

### Linux

den für das Projekt vorgesehenen VST3-Pfad.

Erkläre außerdem, dass ein VST3-Plugin nicht einfach nur eine einzelne `.dll`/`.dylib` sein muss, sondern abhängig vom Betriebssystem als Plugin-Bundle bzw. entsprechendes VST3-Paket vorliegen kann.

## 32.6 Installation

Beschreibe Schritt für Schritt:

1. Release-Build erstellen.
2. erzeugtes VST3-Plugin finden.
3. Plugin in den korrekten VST3-Ordner kopieren/installieren.
4. DAW starten.
5. Plugin-Scan durchführen bzw. Plugin-Cache aktualisieren.
6. Synthesizer als Instrument laden.

## 32.7 Test in einer DAW

Beschreibe einen minimalen Funktionstest.

Der Test muss überprüfen:

```text
MIDI Note
    ↓
Synthesizer
    ↓
Audio
```

und anschließend:

* Carrier-Frequenz ändern
* Modulator-Frequenz ändern
* FM Amount ändern
* Carrier Sine/Saw/Square testen
* Modulator Sine/Saw/Square testen
* Attack testen
* Decay testen
* Sustain testen
* Release testen
* Polyphonie testen

Dokumentiere außerdem, wie festgestellt werden kann, ob das Plugin von der DAW korrekt erkannt wurde.

## 32.8 Debug- und Release-Build

Erkläre den Unterschied zwischen:

```text
Debug
Release
```

und wann welcher Build verwendet wird.

Der Agent soll insbesondere erklären, dass der Debug-Build für Entwicklung und Fehlersuche geeignet ist, während der Release-Build für normale Plugin-Tests verwendet werden soll.

## 32.9 Häufige Build-Probleme

Füge einen Abschnitt hinzu:

```markdown
## Troubleshooting
```

Behandle mindestens:

* CMake findet JUCE nicht
* Compiler nicht gefunden
* fehlende C++-Abhängigkeiten
* Build schlägt wegen einer falschen JUCE-Version fehl
* VST3 erscheint nicht in der DAW
* DAW findet Plugin erst nach erneutem Scan
* Plugin lädt, erzeugt aber keinen Ton
* Plugin lädt und stürzt beim Spielen ab
* Plugin wird wegen eines ungültigen Plugin-Bundles nicht akzeptiert

Die Erklärungen müssen für C++-Einsteiger verständlich sein.

## 32.10 Reproduzierbarer Build

Dokumentiere, wie ein anderer Entwickler das Projekt von einem frisch ausgecheckten Repository aus bauen kann.

Das Ziel ist:

```text
git clone
    ↓
Dependencies installieren
    ↓
CMake konfigurieren
    ↓
Release bauen
    ↓
VST3 installieren
    ↓
DAW öffnen
    ↓
Synthesizer verwenden
```

Alle notwendigen Schritte müssen in `BUILD.md` enthalten sein.

## 32.11 Keine erfundenen Pfade

Verwende keine erfundenen Dateinamen, Targets oder Build-Verzeichnisse.

Prüfe vor dem Schreiben von `BUILD.md`:

* tatsächliche `CMakeLists.txt`
* tatsächliche Target-Namen
* tatsächliche JUCE-Einbindung
* tatsächliche Source-Dateien
* tatsächliche Output-Pfade

Die Dokumentation muss anschließend zum tatsächlich erzeugten Projekt passen.

## 32.12 Abschluss

Am Ende von `BUILD.md` soll eine kurze Checkliste stehen:

```markdown
## Build-Checkliste

- [ ] Voraussetzungen installiert
- [ ] JUCE verfügbar
- [ ] CMake-Konfiguration erfolgreich
- [ ] Release-Build erfolgreich
- [ ] VST3 wurde erzeugt
- [ ] VST3 im korrekten Plugin-Verzeichnis installiert
- [ ] DAW erkennt das Plugin
- [ ] MIDI erzeugt Audio
- [ ] Carrier funktioniert
- [ ] Modulator funktioniert
- [ ] FM funktioniert
- [ ] Sine/Saw/Square funktionieren
- [ ] ADSR funktioniert
- [ ] Polyphonie funktioniert
```

`BUILD.md` ist Teil des fertigen Projekts und muss gemeinsam mit dem Quellcode ausgeliefert werden.
