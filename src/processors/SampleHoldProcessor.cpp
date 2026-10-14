/*
    SampleHoldProcessor implementation.
*/

#include "SampleHoldProcessor.h"

#include <cmath>

namespace smolfm
{

SampleHoldProcessor::SampleHoldProcessor()
    : Processor (ProcessorRole::generic)
{
    // No input wired -> the held value stays at its latched zero: silence,
    // the node is inert, not absent.
    input.setDefaultValue (0.0f);

    // Unwired rate_mod_in contributes no scaling: (1 + 0) = base rate.
    rateMod.setDefaultValue (0.0f);
}

SampleHoldProcessor::SampleHoldProcessor (std::atomic<float>* rateHzP,
                                          std::atomic<float>* syncP,
                                          std::atomic<float>* divP)
    : SampleHoldProcessor()
{
    rateHz   = rateHzP;
    syncMode = syncP;
    division = divP;
}

void SampleHoldProcessor::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    // Grid restarts on the next sample and the held value returns to
    // silence so a re-prepared voice never spits out a stale latch.
    held = StereoFrame { 0.0f, 0.0f };
    samplesUntilHold = 1;
}

void SampleHoldProcessor::startNote()
{
    // Re-anchor the grid to the note: the first rendered sample latches the
    // fresh input, so the staircase starts on the note, not mid-period.
    held = StereoFrame { 0.0f, 0.0f };
    samplesUntilHold = 1;
}

float SampleHoldProcessor::getHoldRate() const noexcept
{
    const float sync     = syncMode != nullptr ? syncMode->load()   : 0.0f;
    const float rate     = rateHz   != nullptr ? rateHz->load()     : 1000.0f;
    const float divIndex = division != nullptr ? division->load()   : 1.0f;  // 1/4 default

    if (sync < 0.5f)
    {
        // Free-running in Hertz.
        return rate;
    }

    // Sync mode: one hold per note division — the rate is the reciprocal
    // of the division's duration (same division table as the delay).
    const float quarterMs = 60000.0f / static_cast<float> (currentBpm);
    const float factor = (divIndex < 0.5f) ? 2.0f    // 1/2 note
                       : (divIndex < 1.5f) ? 1.0f    // 1/4
                       : (divIndex < 2.5f) ? 0.5f    // 1/8
                                           : 0.25f;  // 1/16
    return 1000.0f / (quarterMs * factor);
}

StereoFrame SampleHoldProcessor::processSample()
{
    // Rate modulation on the audio thread mirrors the delay's time_mod_in:
    // the connected value's stereo mean, clamped to [-1, +1], multiplies
    // the base hold rate (free or synced).  With an LFO this sweeps the
    // grid density around the base rate; m = 0 (unwired) keeps the base.
    float rate = getHoldRate();

    if (rateMod.isConnected())
    {
        const StereoFrame pair = rateMod.getSample();
        const float m = juce::jlimit (-1.0f, 1.0f, 0.5f * (pair.l + pair.r));
        rate *= (1.0f + m);
    }

    // Guard the extremes: a modulated rate near zero freezes for a long but
    // finite span; a rate above the sample rate holds for one sample.
    rate = juce::jmax (rate, 1.0e-3f);
    const int period = juce::jmax (1, static_cast<int> (std::round (sampleRate / rate)));

    if (samplesUntilHold <= 1)
    {
        held = input.getSample();
        samplesUntilHold = period;
    }
    else
    {
        --samplesUntilHold;
    }

    output.setSample (held);
    return held;
}

} // namespace smolfm