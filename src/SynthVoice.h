/*
    SynthVoice generates one note of our FM synthesizer.

    A voice owns its own carrier, modulator, and ADSR envelope.  This allows
    multiple voices to play different MIDI notes simultaneously (polyphony).

    The signal flow inside one voice is:

        MIDI note frequency
              │
              ├──► carrier oscillator ──┐
              │                         │
              │      FM phase offset    ▼
              └──► modulator oscillator ─► carrier phase
                                            │
                                            ▼
                                      ADSR envelope
                                            │
                                            ▼
                                          output

    The modulator does not change the carrier's amplitude directly.  Instead,
    its output is used as a phase offset, which is what makes this frequency
    modulation synthesis.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "SimpleOscillator.h"
#include "processors/SignalGraph.h"
#include "processors/NoteProcessor.h"
#include "processors/OscillatorProcessor.h"
#include "processors/FMModulationProcessor.h"
#include "processors/AdsrProcessor.h"

namespace smolfm
{

/**
    A small read-only view of the parameters each voice needs.

    AudioProcessorValueTreeState stores the actual parameters.  It gives us
    raw std::atomic<float>* pointers that the audio thread can read safely
    without locks.  We wrap those pointers in a tiny struct so the voice does
    not have to know about the full APVTS object.
*/
struct SynthVoiceParameters
{
    std::atomic<float>* carrierFrequency;
    std::atomic<float>* modulatorFrequency;
    std::atomic<float>* fmAmount;

    std::atomic<float>* carrierWaveform;
    std::atomic<float>* modulatorWaveform;

    std::atomic<float>* attack;
    std::atomic<float>* decay;
    std::atomic<float>* sustain;
    std::atomic<float>* release;
};

class SynthVoice final : public juce::SynthesiserVoice
{
public:
    /**
        Create a voice that reads its parameters from the shared parameter view.
    */
    explicit SynthVoice (SynthVoiceParameters params);

    /**
        Prepare the voice for a new sample rate.

        This is called from PluginProcessor::prepareToPlay() so every oscillator
        and envelope is configured before any audio is generated.
    */
    void prepare (double newSampleRate);

    /**
        Build the static processor graph for one voice.

        The graph is created once per voice in the constructor.  Order is:
        modulator oscillator → FM modulator (with carrier oscillator inside)
        → ADSR envelope.
    */
    void buildGraph();

    /**
        JUCE calls this when a MIDI note starts.

        We calculate the oscillator frequencies and start the ADSR envelope for
        this voice.
    */
    void startNote (int midiNoteNumber,
                    float velocity,
                    juce::SynthesiserSound* sound,
                    int currentPitchWheelPosition) override;

    /**
        JUCE calls this when a MIDI note ends.

        If allowTailOff is true, the ADSR enters its release phase and the
        voice keeps producing sound until the envelope has fully decayed.  If
        it is false, the voice stops immediately.
    */
    void stopNote (float velocity, bool allowTailOff) override;

    /**
        JUCE asks whether this voice can play a given sound.

        Our synth has only one sound class, so every voice can play it.
    */
    bool canPlaySound (juce::SynthesiserSound* sound) override;

    /**
        Pitch wheel handling is required by JUCE, but pitch bend is outside the
        scope of version 1.  This function therefore does nothing.
    */
    void pitchWheelMoved (int newPitchWheelValue) override;

    /**
        Aftertouch / controller handling is also required by JUCE, but not used
        in this first version.
    */
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    /**
        Render one block of audio for this voice.

        JUCE calls this for every active voice and mixes the outputs together.
        We must add our generated samples to the supplied buffer.
    */
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample,
                          int numSamples) override;

private:
    SynthVoiceParameters parameters;

    double sampleRate = 44100.0;
    float  currentVelocity = 0.0f;

    SignalGraph graph;
    NoteProcessor* noteProcessor = nullptr;
    OscillatorProcessor* modulatorProcessor = nullptr;
    FMModulationProcessor* fmProcessor = nullptr;
    AdsrProcessor* adsrProcessor = nullptr;
};

} // namespace smolfm
