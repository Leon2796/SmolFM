# JUCE FM Synthesizer – Implementation Plan for a Coding Agent

## Original Requirement

Build a simple synthesizer with JUCE that:

* Uses FM synthesis with one carrier and one modulator.
* Has an ADSR envelope.
* Has a UI with sliders that control the carrier and modulator frequencies.
* Provides waveform radio buttons for each oscillator:

  * Sine
  * Saw
  * Square

The functionality should initially remain **minimal**, but the architecture should make it easy to extend later.

The entire codebase must be documented in a **C++ beginner-friendly way**. Comments should primarily explain why something is done and how the audio processing works, rather than merely repeating what an obvious line of code does.

---

# 1. Project Goal

Create a small polyphonic FM synthesizer as a JUCE audio plugin.

Each voice should follow this signal flow:

```text
MIDI Note
    │
    ├──────────────► Carrier Oscillator ──────┐
    │                                         │
    │              FM                         ▼
    └──► Modulator Oscillator ─────► Carrier Phase
                                             │
                                             ▼
                                        ADSR Envelope
                                             │
                                             ▼
                                           Output
```

The conceptual FM equation is:

```text
carrier(t) = waveformCarrier(
    carrierPhase(t)
    + modulationIndex * modulator(t)
)
```

The implementation does not need to become mathematically unnecessarily complex.

The initial implementation must provide:

1. Two oscillators.
2. One oscillator modulating the phase of the other.
3. Sine, Saw, and Square waveforms for both oscillators.
4. UI control of carrier and modulator frequency.
5. Overall volume controlled by an ADSR envelope.
6. MIDI notes triggering voices.
7. Multiple voices working simultaneously.

---

# 2. JUCE Version and Documentation

Use the currently installed/stable JUCE version of the project.

Use the current official JUCE documentation, especially for:

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
* `juce::ToggleButton`

Use **Context7** to check current JUCE/library documentation before implementing APIs whose exact behavior or signature matters.

`juce::Synthesiser` provides the voice/sound infrastructure for polyphonic synthesis. Audio generation happens inside `SynthesiserVoice`.

`juce::dsp::Oscillator` can be initialized with a periodic waveform function, prepared with a `ProcessSpec`, and controlled with `setFrequency()`. Its `processSample()` method accepts an input value that is used during oscillator processing, which can be used for the planned phase modulation.

`juce::ADSR` provides `setSampleRate()`, `setParameters()`, `noteOn()`, `noteOff()`, and `getNextSample()`.

`AudioProcessorValueTreeState` should be used as the central parameter/state system. Its attachments keep UI controls synchronized with plugin parameters.

---

# 3. Architecture

Use the following simple structure:

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

Keep the architecture deliberately simple.

Avoid:

* Complex DSP graphs
* Unnecessary abstract factory classes
* Dependency injection frameworks
* Complex GUI frameworks
* Custom parameter-management systems
* Custom state-storage systems
* Unnecessary threading abstractions

The architecture should still make later additions such as additional operators or filters reasonably straightforward.

---

# 4. Files

Initially create:

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

An additional file may later be introduced:

```text
OscillatorWaveform.h
```

However, only introduce it if it genuinely improves readability.

---

# 5. SynthSound

Create a simple class:

```cpp
class SynthSound : public juce::SynthesiserSound
```

The class only describes the sound that the voices can play.

It must:

* Accept all MIDI notes.
* Accept all MIDI channels.
* Contain no audio-generation code.

Implement:

```cpp
bool appliesToNote (int midiNoteNumber) override;
bool appliesToChannel (int midiChannel) override;
```

Both functions can simply return `true` for this first synthesizer.

Document why `SynthSound` does not contain audio-processing code:

> `SynthSound` describes which sounds can be played. The actual audio rendering is performed by `SynthesiserVoice`.

This follows the intended JUCE synthesizer model.

---

# 6. SynthVoice

Create:

```cpp
class SynthVoice : public juce::SynthesiserVoice
```

The voice contains:

```cpp
juce::dsp::Oscillator<float> carrierOscillator;
juce::dsp::Oscillator<float> modulatorOscillator;

juce::ADSR adsr;
```

Additionally, keep the required per-voice state, for example:

```cpp
double sampleRate = 44100.0;

float currentVelocity = 0.0f;
float currentCarrierFrequency = 440.0f;
float currentModulatorFrequency = 440.0f;
```

The FM amount and other values should preferably not be permanently copied into each voice. The voice should access the current plugin parameters through a shared, clearly defined parameter interface.

A simple reference to the parameter state or a small parameter-access structure is acceptable.

Because `juce::SynthesiserVoice` has pure virtual functions such as `startNote()`, `stopNote()`, `renderNextBlock()`, and `pitchWheelMoved()`, all required functions must be implemented even if pitch wheel support is outside the feature scope. `pitchWheelMoved()` can simply do nothing for version 1.

---

# 7. Waveforms

Both oscillators must support:

```text
Sine
Saw
Square
```

Define a small enumeration:

```cpp
enum class Waveform
{
    sine,
    saw,
    square
};
```

Each waveform should have a mathematical function.

## Sine

```cpp
std::sin (phase)
```

## Saw

Use a simple mathematical sawtooth function.

The waveform function should use the phase range expected by the oscillator implementation. When using `juce::dsp::Oscillator`, verify the current JUCE phase convention in the documentation rather than assuming it from an older example.

## Square

A simple implementation is sufficient:

```cpp
phase < 0.0f ? -1.0f : 1.0f
```

The waveforms do not need to be band-limited.

This is acceptable for version 1.

Document explicitly that these simple waveforms can produce aliasing at higher frequencies. Band-limited oscillators can be added later.

---

# 8. Oscillator Initialization

Initialize both `juce::dsp::Oscillator<float>` instances with suitable periodic waveform functions.

Use:

```cpp
oscillator.initialise (...);
```

and:

```cpp
oscillator.prepare (spec);
```

`prepare()` must be called before audio processing. The oscillator must not be unnecessarily rebuilt for every audio block.

The oscillator frequency can then be updated with `setFrequency()` when required.

---

# 9. MIDI Note → Frequency

In `startNote()`:

```cpp
double frequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
```

The MIDI note frequency is the basis for the carrier frequency.

Use musical frequency ratios internally:

```text
carrierFrequency  = midiFrequency * carrierRatio
modulatorFrequency = midiFrequency * modulatorRatio
```

This makes the synthesizer musical and automatically transposable.

The UI should display these values as ratios:

```text
Carrier:    1.00x
Modulator:  2.00x
```

Recommended ranges:

```text
Carrier Ratio:
0.25x – 4.0x

Modulator Ratio:
0.25x – 8.0x
```

If the existing project specification explicitly requires absolute Hz sliders, absolute frequencies may be used instead. Otherwise, ratios are preferred.

---

# 10. FM Implementation

The important part is the FM synthesis.

For each sample:

1. Generate a modulator sample.
2. Convert that sample into a phase-modulation amount.
3. Generate the carrier using the modulated phase.
4. Apply the ADSR envelope.
5. Add the result to the output buffer.

Conceptually:

```cpp
float modulatorSample =
    modulatorOscillator.processSample (0.0f);

float phaseModulation =
    modulatorSample * fmAmount;

float carrierSample =
    carrierOscillator.processSample (phaseModulation);
```

The important detail is that `phaseModulation` represents a **phase offset in radians**.

Do not treat the `processSample()` input as a frequency value. The current JUCE implementation accepts an input value as part of oscillator phase processing. Verify the exact behavior against the JUCE version used by the project.

A suitable `fmAmount` range for version 1 can therefore be:

```text
0.0 – 10.0 radians
```

This is a modulation-depth control rather than a frequency value.

The resulting sample is then multiplied by the ADSR envelope and added to the output buffer.

---

# 11. Optional SimpleOscillator Abstraction

For maximum educational clarity, it is possible to encapsulate the basic oscillator mathematics in a small class:

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

The class would implement:

```text
Phase
  ↓
Waveform Function
  ↓
Sample
```

For every sample:

```cpp
phase += phaseIncrement;
```

For the carrier:

```cpp
float output = waveformFunction (phase + phaseModulation);
```

This makes the FM mathematics very explicit.

However, for this version, **prefer `juce::dsp::Oscillator` unless the custom class clearly improves understanding**. The original requirement explicitly calls for JUCE's oscillator, and it already provides the necessary processing interface.

A custom oscillator may be introduced later if it provides a clear educational or architectural benefit.

---

# 12. ADSR

Each `SynthVoice` has its own:

```cpp
juce::ADSR adsr;
```

When a note starts:

```cpp
adsr.noteOn();
```

When the note ends:

```cpp
adsr.noteOff();
```

For each sample:

```cpp
float envelope = adsr.getNextSample();
```

Then:

```cpp
outputSample *= envelope;
```

Before use, configure the ADSR with the current sample rate:

```cpp
adsr.setSampleRate (sampleRate);
```

and its parameters:

```text
Attack
Decay
Sustain
Release
```

The ADSR parameters come from the central parameter state.

Important: JUCE explicitly states that ADSR parameters should not be changed during playback. If parameters are changed before the release stage has completed, `reset()` must be called before the next `noteOn()`. The implementation must therefore avoid blindly calling `setParameters()` on an active envelope.

A simple safe strategy for version 1 is:

* Read the current ADSR parameter values when `startNote()` is called.
* Create/update the `juce::ADSR::Parameters` structure there.
* Call `setParameters()` before `noteOn()`.
* Do not modify ADSR parameters while that voice is actively playing.
* Apply newly changed ADSR settings to the next note.

This keeps the implementation simple and follows JUCE's documented behavior.

---

# 13. stopNote()

Implement:

```cpp
void stopNote (float velocity, bool allowTailOff) override
```

If:

```cpp
allowTailOff == true
```

call:

```cpp
adsr.noteOff();
```

The voice should continue rendering until the release phase has finished.

Inside `renderNextBlock()`:

```cpp
if (! adsr.isActive())
{
    clearCurrentNote();
    return;
}
```

When a voice has finished its sound, `clearCurrentNote()` must be called so the synthesizer can reuse the voice.

If `allowTailOff` is `false`, immediately stop the voice and clear its current note after resetting the necessary per-note state.

---

# 14. Polyphony

In `PluginProcessor`:

```cpp
juce::Synthesiser synth;
```

Add eight voices:

```cpp
constexpr int numberOfVoices = 8;

for (int i = 0; i < numberOfVoices; ++i)
    synth.addVoice (new SynthVoice (...));
```

Add one sound:

```cpp
synth.addSound (new SynthSound());
```

JUCE provides polyphony by assigning notes to multiple `SynthesiserVoice` instances.

---

# 15. Parameter System

Use:

```cpp
juce::AudioProcessorValueTreeState parameters;
```

with a `ParameterLayout`.

Use stable parameter IDs:

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

Parameter IDs must remain stable because they are part of the plugin state and automation/preset compatibility.

Do not use GUI components as the source of truth.

The architecture must be:

```text
Parameter
   ↓
AudioProcessorValueTreeState
   ├── Audio Engine
   └── GUI
```

not:

```text
Slider
   ↓
Audio Engine
```

`AudioProcessorValueTreeState` is intended to manage the processor's parameter/state information and provides the attachment classes used to connect the GUI.

---

# 16. Parameter Types

Use:

```cpp
juce::AudioParameterFloat
```

for numeric parameters.

For waveform selection, use:

```cpp
juce::AudioParameterChoice
```

with:

```text
Sine
Saw
Square
```

Internally:

```text
0 = Sine
1 = Saw
2 = Square
```

Do not use three independent boolean parameters for a waveform selection. A `Choice` parameter guarantees that the state represents exactly one selected waveform.

The UI can still represent this choice with three radio-style buttons.

---

# 17. UI

The UI should remain deliberately simple.

Recommended structure:

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

Each oscillator MUST have its own three waveform buttons.

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

The two waveform selections must be independent.

---

# 18. Radio-Button Behavior

Use three `ToggleButton`s per oscillator.

The three buttons in each group must behave as a radio group.

Use JUCE's radio-group mechanism, for example by assigning the same radio group ID to the three buttons in a group.

There must be two independent groups:

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

Do not use one radio group for all six buttons.

The selected button must represent the corresponding `AudioParameterChoice`.

The parameter remains the source of truth; the buttons are only its UI representation.

If `ButtonAttachment` is used, verify the attachment behavior for the installed JUCE version. JUCE provides `AudioProcessorValueTreeState::ButtonAttachment` specifically for synchronizing buttons and parameters.

---

# 19. Sliders

Use:

```cpp
juce::Slider
```

and connect the sliders to parameters using:

```cpp
juce::AudioProcessorValueTreeState::SliderAttachment
```

For example:

```cpp
std::unique_ptr<
    juce::AudioProcessorValueTreeState::SliderAttachment>
    carrierRatioAttachment;
```

`SliderAttachment` automatically keeps the slider and parameter synchronized during its lifetime.

Do not implement manual `sliderValueChanged()` callbacks just to transfer slider values into the plugin parameters.

---

# 20. Recommended Parameter Ranges

Use:

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
0.0 – 10.0 radians
Default: 0.0
```

### Attack

```text
0.001 – 5.0 seconds
Default: 0.01
```

### Decay

```text
0.001 – 5.0 seconds
Default: 0.2
```

### Sustain

```text
0.0 – 1.0
Default: 0.8
```

### Release

```text
0.001 – 10.0 seconds
Default: 0.5
```

For Attack, Decay, and Release, use a logarithmic or otherwise musically useful slider mapping so that short envelope times can be adjusted precisely.

---

# 21. AudioProcessor

`PluginProcessor` is responsible for:

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

1. Store the sample rate.
2. Call:

```cpp
synth.setCurrentPlaybackSampleRate (sampleRate);
```

3. Ensure all voice DSP objects are prepared for the new sample rate.

JUCE requires the synthesizer to receive the current playback sample rate before rendering; this value is then propagated to the voices.

In `processBlock()`:

1. Prepare/clear the audio buffer as appropriate.
2. Pass the MIDI data to the synthesizer.
3. Call:

```cpp
synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
```

4. Apply any final output-level handling required by the specification.

The synthesizer adds its generated audio to the supplied buffer, so the buffer must be cleared first if no existing audio should be mixed with the synthesizer output.

---

# 22. Audio-Thread Rules

Pay particular attention to real-time safety.

Inside:

```cpp
processBlock()
renderNextBlock()
startNote()
stopNote()
```

avoid operations that can block or introduce unpredictable latency.

In particular, avoid:

```text
std::cout
File I/O
Locks
Memory allocation per sample
GUI access
```

Do not allocate memory inside the per-sample rendering loop.

Parameter values should be read efficiently.

Voice rendering should remain small, deterministic, and predictable.

---

# 23. Parameter Access in Voices

Prefer a simple shared parameter-access structure, for example:

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

Alternatively, create a small parameter-access class if this makes the code easier to understand.

The important rule is:

**The GUI must not directly access a voice.**

The data flow is:

```text
UI
 ↓
APVTS
 ↓
Voice
```

The voice reads the current parameter values from the processor-side parameter system.

For parameters that can change while notes are playing, such as frequency ratios and FM amount, use the current parameter values during rendering.

For ADSR parameters, follow the safe update strategy described in section 12 because JUCE does not allow arbitrary parameter changes during an active envelope.

---

# 24. State Saving

The synthesizer must be able to save and restore its state.

Use:

```cpp
AudioProcessorValueTreeState
```

together with the standard:

```cpp
getStateInformation()
setStateInformation()
```

methods of the `AudioProcessor`.

The APVTS should be the single source of truth for all plugin parameters and state. JUCE documents `AudioProcessorValueTreeState` as the state/parameter-management mechanism for an `AudioProcessor`.

---

# 25. C++ Beginner Friendliness

The entire codebase must be written in a didactic way.

Each important class should have a short introductory comment:

```cpp
/**
    SynthVoice generates one note of our synthesizer.

    A voice owns its own carrier, modulator, and ADSR envelope.
    This allows multiple voices to play different MIDI notes
    simultaneously.
*/
```

Important functions should also explain their purpose:

```cpp
/**
    Called by JUCE when a MIDI note starts.

    We calculate the oscillator frequencies and start
    the ADSR envelope for this voice.
*/
```

Do not comment every trivial getter or setter.

Comments should primarily explain:

* Audio flow
* FM synthesis
* Voice lifecycle
* MIDI → frequency conversion
* ADSR behavior
* Parameter flow
* Real-time/threading considerations

---

# 26. Extensible Architecture

Although the project should remain minimal, the architecture should allow later additions such as:

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
+ Second Modulator
        ↓
Version 4
+ Modulator Envelope
        ↓
Version 5
+ LFO
        ↓
Version 6
+ Multiple FM Operators
```

Do not structure version 1 in a way that makes carrier and modulator impossible to generalize later.

A future structure such as:

```cpp
struct Operator
{
    SimpleOscillator oscillator;
    float level;
    float ratio;
};
```

should remain possible.

However, version 1 must implement **only one carrier and one modulator**.

---

# 27. No Unnecessary Features

Do not implement:

* Preset browser
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
* Custom skin
* Spectrum analyzer

These features are explicitly outside the scope.

The agent must not turn the small synthesizer into a complete commercial instrument.

---

# 28. Development Order

Implement the project in the following phases.

## Phase 1 – Minimal Audio Output

Initially implement:

```text
MIDI Note
→ Carrier
→ Output
```

Use only Sine.

Test:

* MIDI note C3 produces an audible tone.
* MIDI note C4 produces a frequency approximately twice that of C3.

---

## Phase 2 – Voice System

Implement:

```text
Synthesiser
SynthesiserSound
SynthesiserVoice
```

Add eight voices.

Test:

* Multiple notes can be played simultaneously.

---

## Phase 3 – Modulator

Add the second oscillator.

Test:

```text
FM Amount = 0
```

should sound approximately like the carrier alone.

Then:

```text
FM Amount > 0
```

must audibly change the spectrum.

---

## Phase 4 – Waveforms

Implement for the carrier:

```text
Sine
Saw
Square
```

and for the modulator:

```text
Sine
Saw
Square
```

Test each selection independently.

---

## Phase 5 – ADSR

Add the ADSR envelope.

Test:

```text
Attack = short
Decay = short
Sustain = 0.8
Release = long
```

Note-On and Note-Off must produce clearly audible envelope behavior.

Verify that changing ADSR controls affects subsequent notes without violating JUCE's ADSR parameter-update rules.

---

## Phase 6 – Parameter State

Fully integrate `AudioProcessorValueTreeState`.

Test:

1. Change parameters.
2. Save the plugin state.
3. Close/reload the plugin.
4. Restore the saved state.
5. Verify that all parameters are restored.

---

## Phase 7 – UI

Add:

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

Connect all numeric controls through `SliderAttachment`.

Connect waveform buttons through the parameter system.

---

# 29. Acceptance Criteria

The project is complete only when all of the following are satisfied:

* [ ] The JUCE project compiles without errors.
* [ ] The plugin can be loaded by a DAW.
* [ ] MIDI notes produce audio.
* [ ] At least 8 voices work polyphonically.
* [ ] The carrier oscillator works.
* [ ] The modulator oscillator works.
* [ ] FM Amount = 0 produces no phase modulation.
* [ ] FM Amount > 0 changes the resulting spectrum.
* [ ] The carrier can select Sine.
* [ ] The carrier can select Saw.
* [ ] The carrier can select Square.
* [ ] The modulator can select Sine.
* [ ] The modulator can select Saw.
* [ ] The modulator can select Square.
* [ ] Carrier and modulator waveforms are independent.
* [ ] Carrier frequency can be changed.
* [ ] Modulator frequency can be changed.
* [ ] Attack works.
* [ ] Decay works.
* [ ] Sustain works.
* [ ] Release works.
* [ ] Note-Off starts the release phase.
* [ ] A voice is released after its release phase has completed.
* [ ] All parameters are managed through `AudioProcessorValueTreeState`.
* [ ] The UI stays synchronized with the parameters.
* [ ] Plugin state can be saved and restored.
* [ ] No unnecessary real-time allocations occur during audio rendering.
* [ ] `pitchWheelMoved()` is implemented as required by `SynthesiserVoice`, even though pitch bend is outside the scope of version 1.
* [ ] The code is fully documented for C++ beginners.
* [ ] No unnecessary features outside the scope have been added.

---

# 30. Important Implementation Decision

Before writing the code, consider the oscillator implementation carefully.

### Preferred: `juce::dsp::Oscillator`

Advantages:

```text
JUCE-native
Less custom DSP code
Well integrated with JUCE's DSP infrastructure
```

The oscillator supports a periodic waveform function, `prepare()`, frequency control, and sample-by-sample processing. Its `processSample()` input can be used as the phase-modulation input required for the FM implementation.

### Optional: Small Custom `SimpleOscillator`

Advantages:

```text
FM mathematics is completely explicit
Phase modulation is easy to understand
Waveform logic is fully controlled
```

For this project, prefer **`juce::dsp::Oscillator`** for the first implementation.

Only introduce a custom `SimpleOscillator` if it provides a clear educational benefit and does not unnecessarily increase the complexity of the project.

Always verify the exact JUCE API against the version installed in the project before implementation.

---

# 31. Definition of Done

The final result must not be a code skeleton.

The agent must deliver a **compilable minimal plugin**.

The code should be structured so that a C++ beginner can follow:

```text
How does MIDI enter the synthesizer?
        ↓
How does MIDI become a voice?
        ↓
How is the frequency calculated?
        ↓
How does the carrier generate samples?
        ↓
How does the modulator affect the carrier?
        ↓
How is the ADSR envelope applied?
        ↓
How does the result reach the audio output?
        ↓
How does the GUI control the parameters?
```

The implementation should be **small, clear, and correct** rather than unnecessarily extensive.

When a design decision has to choose between maximum extensibility and readability, **readability has priority for version 1**, as long as future extension is not unnecessarily prevented.
