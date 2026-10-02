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

    // Unwired amount_mod_in contributes no scaling: (1 + 0) = base amount.
    amountMod.setDefaultValue (0.0f);

    // Unwired vel input scales by 1.0: the sound stays exactly as before.
    velIn.setDefaultValue (1.0f);
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

    // Amount modulation: the connected value's stereo mean, clamped to
    // [-1, +1], multiplies the static index by (1 + m) — a signal can open
    // the FM index from zero and swing the timbre per sample (spectral
    // envelopes, index tremolo).  Unwired: static amount, fully transparent.
    float amountEff = amount;
    if (amountMod.isConnected())
    {
        const StereoFrame modPair = amountMod.getSample();
        const float m = juce::jlimit (-1.0f, 1.0f, 0.5f * (modPair.l + modPair.r));
        amountEff = juce::jlimit (0.0f, 10.0f, amount * (1.0f + m));
    }

    // Velocity (accent brightness): the connected vel input's stereo mean
    // (0-1) scales the FM index - hard hits gain metallic sidebands.
    // Unwired: fully sound-invariant.
    if (velIn.isConnected())
    {
        const StereoFrame vp = velIn.getSample();
        amountEff = juce::jlimit (0.0f, 10.0f, amountEff * 0.5f * (vp.l + vp.r));
    }

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
    const StereoFrame modulatorPair = (amountEff > 0.0f && modulatorInput.isConnected())
                                    ? modulatorInput.getSample()
                                    : StereoFrame { 0.0f, 0.0f };
    const float modulator = 0.5f * (modulatorPair.l + modulatorPair.r);

    const float instantHz = carrierHz * (1.0f + amountEff * modulator);

    const StereoFrame result = StereoFrame::centered (instantHz);
    output.setSample (result);
    return result;
}

} // namespace smolfm
