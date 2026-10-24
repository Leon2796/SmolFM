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
#include "processors/VelocityScaleProcessor.h"
#include "processors/NoteProcessor.h"
#include "processors/AdsrProcessor.h"
#include "processors/FAdsrProcessor.h"
#include "processors/GainProcessor.h"
#include "processors/WaveshaperProcessor.h"
#include "processors/FilterProcessor.h"
#include "processors/SplitterProcessor.h"
#include "processors/MergerProcessor.h"
#include "processors/AdderProcessor.h"
#include "processors/MasterOutputProcessor.h"
#include "processors/RingModulatorProcessor.h"
#include "processors/AmProcessor.h"
#include "processors/DelayProcessor.h"
#include "processors/SampleHoldProcessor.h"
#include "processors/ResonatorProcessor.h"
#include "processors/FeedbackProcessor.h"
#include "graph/GraphNodes.h"
#include "EffectiveRangeTelemetry.h"     // telemetry range protocol (see header)

namespace smolfm
{

// Effective-value telemetry range protocol: see src/EffectiveRangeTelemetry.h.
// A degenerate (v, v) range is real data (one needle value); "no data" is
// exclusively a NaN length — never isEmpty(), which is also true for
// (v, v) and would discard every played voice in the aggregator.

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
    std::array<std::atomic<float>*, GraphNodeRegistry::maxOscillators> oscMode;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxOscillators> oscStaticFreq;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxOscillators> oscLfoRate;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFmAmounts>   fmAmount;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFrequencyScales> freqScaleFactor;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFrequencyScales> freqScaleInterval;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxVelocityScales> velScaleCurve;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxVelocityScales> velScaleScale;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxAdsr> adsrAttack;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxAdsr> adsrDecay;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxAdsr> adsrSustain;
        std::array<std::atomic<float>*, GraphNodeRegistry::maxAdsr> adsrRelease;
        std::array<std::atomic<float>*, GraphNodeRegistry::maxAmModulators> amAmount;

    // F-ADSR pitch-envelope parameters per instance (6 parameters per node).
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFAdsr> fAdsrAttack;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFAdsr> fAdsrDecay;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFAdsr> fAdsrSustain;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFAdsr> fAdsrRelease;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFAdsr> fAdsrUp;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFAdsr> fAdsrDown;

    // Gain parameters per instance (1 parameter per node).
    std::array<std::atomic<float>*, GraphNodeRegistry::maxGains> gainFactor;

    // Waveshaper parameters per instance (2 parameters per node).
    std::array<std::atomic<float>*, GraphNodeRegistry::maxWaveshapers> shaperDrive;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxWaveshapers> shaperShape;

    // Filter parameters per instance (3 parameters per node).
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFilters> filterCutoff;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFilters> filterResonance;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFilters> filterMode;

    // Delay parameters per instance (5 parameters per delay node).
    std::array<std::atomic<float>*, GraphNodeRegistry::maxDelays> delayTimeMs;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxDelays> delayFeedback;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxDelays> delayMix;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxDelays> delaySync;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxDelays> delayDivision;

    // Sample-hold parameters per instance (3 parameters per node).
    std::array<std::atomic<float>*, GraphNodeRegistry::maxSampleHolds> holdRate;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxSampleHolds> holdSync;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxSampleHolds> holdDivision;

    // Resonator parameters per instance (6 parameters per node): tune (Hz,
    // static fallback), q (0..1), decay (t60 seconds), the vel spread
    // (0.25..4; 1 = neutral — accent scales the decay time per the plan's
    // pull-to-neutral pattern) and the internal strike pair (strikeLevel
    // 0..2, strikeTime seconds; both default 0 = strike off, the
    // byte-identical old behaviour — plan Phase 2).
    std::array<std::atomic<float>*, GraphNodeRegistry::maxResonators> resonatorTune;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxResonators> resonatorQ;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxResonators> resonatorDecay;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxResonators> resonatorVelSpread;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxResonators> resonatorStrikeLevel;
    std::array<std::atomic<float>*, GraphNodeRegistry::maxResonators> resonatorStrikeTime;
    // Feedback (bounded cycle breaker) per instance (1 parameter per node).
    std::array<std::atomic<float>*, GraphNodeRegistry::maxFeedbacks> feedbackAmount;

    // Master output is a singleton.
    std::atomic<float>* masterLevel;
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

        /** Peak of this voice's master output, for the UI meter. */
        float getMasterPeakLevel() const noexcept;

        /**
            Effective (modulated) band of one control while this voice
            renders, for the UI's mod indicator overlays.

            The slider attachment shows the *static* parameter; what the
            processor actually uses per sample while its mod port is wired
            is computed in processSample() and accumulated into the
            NeedleWindowTelemetry min/max window (a per-sample overwrite
            would alias against the overlay's 30 Hz poll).  This returns
            the completed window's [min, max] band: a degenerate (v, v)
            point for a constant / unwired control, the real excursion
            while a mod signal moves it.  Velocity deliberately never feeds
            the needle: it stays sound-only (see the processors' telemetry
            comments).

            @param type          node type (fmAmount, gain, waveshaper,
                                 filter, delay or resonator)
            @param instanceIndex index into that type's pool
            @param controlIndex  which control of the instance (filter:
                                 0 = cutoff, 1 = resonance; everywhere else 0)
            @return              the telemetry window band — degenerate
                                 (v, v) for a constant value;
                                 noEffectiveData() (NaN length) when the
                                 instance does not exist or has not
                                 rendered yet — never an empty/zero-length
                                 range, which would be indistinguishable
                                 from the value 0.0 (see the protocol
                                 comment above).

            Idle voices are excluded at the aggregator (isVoiceActive): they
            stopped rendering, so the window band their processors
            published last is stale.
        */
        juce::Range<float> getEffectiveControlRange (NodeType type, int instanceIndex,
                                                     int controlIndex) const noexcept;

    /** Called from the processor thread when the host playhead BPM changes.
        Forwards the tempo to every tempo-sync-aware processor (delays). */
    void setHostTempo (double bpm) noexcept;

private:
    /**
        Voice lifetime policy.

        A voice keeps rendering only while it has a reason to live:
          - the key is still held, OR
          - a connected ADSR is still sounding (attack..release), OR
          - a connected delay still carries tail energy.

        Without the last two rules a voice with no wired envelope would drone
        forever (note sources keep emitting the last note frequency), and
        delay tails would be chopped the moment the last envelope finished.
    */
    bool hasReasonToLive() const noexcept;

    /// True from startNote() until the voice's lifetime policy ends it.
    bool keyHeld = false;

    SynthVoiceParameters parameters;

    double sampleRate = 44100.0;

    SignalGraph graph;

    // Single nodes.
    MasterOutputProcessor* masterOutput = nullptr;

    // Processor pools, indexed by instance index parsed from the node id.
    // Every note source mirrors the currently played MIDI note; each wired
    // "note<N>" box simply taps the same pitch on its own output.
    std::array<NoteProcessor*, GraphNodeRegistry::maxNotes> noteSources {};
    std::array<OscillatorProcessor*, GraphNodeRegistry::maxOscillators> oscillators {};
    std::array<FMModulationProcessor*, GraphNodeRegistry::maxFmAmounts> fmProcessors {};
    std::array<FrequencyScaleProcessor*, GraphNodeRegistry::maxFrequencyScales> frequencyScalers {};
    std::array<VelocityScaleProcessor*, GraphNodeRegistry::maxVelocityScales> velocityScalers {};
    std::array<AdsrProcessor*, GraphNodeRegistry::maxAdsr> adsrProcessors {};
    std::array<FAdsrProcessor*, GraphNodeRegistry::maxFAdsr> fAdsrProcessors {};
    std::array<RingModulatorProcessor*, GraphNodeRegistry::maxRingModulators> ringModulators {};
    std::array<AmProcessor*, GraphNodeRegistry::maxAmModulators> amModulators {};
    std::array<DelayProcessor*, GraphNodeRegistry::maxDelays> delays {};
    std::array<SampleHoldProcessor*, GraphNodeRegistry::maxSampleHolds> holdProcessors {};
    std::array<GainProcessor*, GraphNodeRegistry::maxGains> gainProcessors {};
    std::array<WaveshaperProcessor*, GraphNodeRegistry::maxWaveshapers> waveshapers {};
    std::array<FilterProcessor*, GraphNodeRegistry::maxFilters> filters {};
    std::array<SplitterProcessor*, GraphNodeRegistry::maxSplitters> splitters {};
    std::array<MergerProcessor*, GraphNodeRegistry::maxMergers> mergers {};
    std::array<AdderProcessor*, GraphNodeRegistry::maxAdds> adders {};
    // Feedback nodes sit before the resonators in render order so the loop
    // entry (their out) reaches the body fresh and the return edge (into
    // fb_in) is what contributes the 1-sample delay.
    std::array<FeedbackProcessor*, GraphNodeRegistry::maxFeedbacks> feedbackProcessors {};
    std::array<ResonatorProcessor*, GraphNodeRegistry::maxResonators> resonators {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthVoice)
};

} // namespace smolfm
