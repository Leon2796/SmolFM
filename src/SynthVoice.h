/*
    SynthVoice generates one note of our FM synthesizer.

    A voice owns a *pool* of processors, not a fixed few: 8 oscillators, 4 FM
    stages, 1 note source and 1 ADSR envelope exist per voice from the start.
    The UI patch decides which of them are wired — an unused processor just
    stays disconnected and contributes nothing.

    Signal flow inside one voice is fully defined by the ConnectionPatch:

        note.out (frequency)      →  fmM.freq_in
        oscN.out  (signal)        →  fmM.modulator_in
        fmM.out   (frequency)     →  fmK.freq_in (chaining) / oscN.note_in
        oscN.out  (signal)        →  adsr.in

    The modulator is not special: it is simply an oscillator that happens to
    be wired into the FM input labelled "modulator".  The FM stage only
    bends Hertz; the carrier oscillator renders the waveform (true FM).
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "SimpleOscillator.h"
#include "processors/SignalGraph.h"
#include "processors/OscillatorProcessor.h"
#include "processors/FMModulationProcessor.h"
#include "processors/FrequencyScaleProcessor.h"
#include "processors/NoteProcessor.h"
#include "processors/AdsrProcessor.h"
#include "graph/GraphNodes.h"

namespace smolfm
{

/**
    Read-only view of the parameters each voice needs.

    Because the graph is dynamic, the voice holds *pools* of raw parameter
    pointers — one entry per possible instance.  The audio thread never locks;
    everything here is a plain atomic pointer.
*/
struct SynthVoiceParameters
{
    // Index into these arrays is the instance index of the node.
    std::array<std::atomic<float>*, GraphNodeRegistry::maxOscillators> oscWaveform;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFmAmounts>   fmAmount;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFrequencyScales> freqScaleFactor;

    // ADSR is a singleton.
    std::atomic<float>* attack;
    std::atomic<float>* decay;
    std::atomic<float>* sustain;
    std::atomic<float>* release;
};

class SynthVoice final : public juce::SynthesiserVoice
{
public:
    explicit SynthVoice (SynthVoiceParameters params);

    void prepare (double newSampleRate);
    void buildGraph();

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    /**
        Rewire this voice's processor graph to match a connection patch.

        Called from the message thread; the atomic port pointers make the
        change visible to the audio thread on the next sample.
    */
    void applyConnectionPatch (const ConnectionPatch& patch);

private:
    SynthVoiceParameters parameters;

    double sampleRate = 44100.0;
    float  currentVelocity = 0.0f;

    SignalGraph graph;

    // Single nodes.
    AdsrProcessor* adsrProcessor = nullptr;

    // Processor pools, indexed by instance index parsed from the node id.
    // Every note source mirrors the currently played MIDI note; each wired
    // "note<N>" box simply taps the same pitch on its own output.
    std::array<NoteProcessor*, GraphNodeRegistry::maxNotes> noteSources {};
    std::array<OscillatorProcessor*, GraphNodeRegistry::maxOscillators> oscillators {};
    std::array<FMModulationProcessor*, GraphNodeRegistry::maxFmAmounts> fmProcessors {};
    std::array<FrequencyScaleProcessor*, GraphNodeRegistry::maxFrequencyScales> frequencyScalers {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthVoice)
};

} // namespace smolfm
