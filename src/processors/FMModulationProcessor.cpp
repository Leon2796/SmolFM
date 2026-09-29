/*
    FMModulationProcessor implementation.
*/

#include "FMModulationProcessor.h"

namespace smolfm
{

FMModulationProcessor::FMModulationProcessor (std::atomic<float>* amount)
    : Processor (ProcessorRole::fmModulator),
      fmAmount (amount)
{
    // A disconnected frequency input means 0 Hz; the oscillator at the end
    // of the chain stays silent then (0 Hz, no fallback frequency).
    freqInput.setDefaultValue (0.0f);

    // A disconnected modulator must contribute no deviation, not DC.
    modulatorInput.setDefaultValue (0.0f);
}

void FMModulationProcessor::prepare (double newSampleRate)
{
    // Stateless: Hertz in, Hertz out.  No sample-rate dependent state.
    juce::ignoreUnused (newSampleRate);
}

void FMModulationProcessor::startNote()
{
    // No state to reset.
}

StereoFrame FMModulationProcessor::processSample()
{
    const float carrierHz = freqInput.getSample().l;
    const float amount    = fmAmount->load();

    // True FM: the deviation is proportional to the carrier frequency, so
    // the modulation index (and therefore the timbre) stays the same on
    // every key.  amount = 0 or no modulator wired: the frequency passes
    // through unchanged and the stage is fully transparent.
    //
    // Stereo compatibility: modulator_in carries a StereoFrame pair, but the
    // stage emits exactly one instantaneous frequency (the oscillator owns a
    // single phase).  The pair collapses through its mean (L + R) * 0.5:
    // centered (mono) modulators — everything that existed before stereo —
    // keep exactly the modulation depth they had before, and per-side or
    // panned modulators contribute their averaged deviation.  (A plain sum
    // would double the depth for centered content.)
    const StereoFrame modulatorPair = (amount > 0.0f && modulatorInput.isConnected())
                                    ? modulatorInput.getSample()
                                    : StereoFrame { 0.0f, 0.0f };
    const float modulator = 0.5f * (modulatorPair.l + modulatorPair.r);

    const float instantHz = carrierHz * (1.0f + amount * modulator);

    const StereoFrame result = StereoFrame::centered (instantHz);
    output.setSample (result);
    return result;
}

} // namespace smolfm
