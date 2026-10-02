/*
    FeedbackProcessor implementation.
*/

#include "FeedbackProcessor.h"

#include <algorithm>
#include <cmath>

namespace smolfm
{

FeedbackProcessor::FeedbackProcessor (std::atomic<float>* amountParameter)
    : Processor (ProcessorRole::generic),
      amount (amountParameter)
{
    // Unwired inputs read 0: a feedback node with nothing wired contributes
    // silence, never feedback noise.
    input.setDefaultValue (0.0f);
    fbIn.setDefaultValue (0.0f);
}

void FeedbackProcessor::prepare (double /*newSampleRate*/)
{
    // No sample-rate dependent state; the tail estimate stays valid.
}

void FeedbackProcessor::startNote()
{
    // Keep the loop (see header): recirculated energy carries across notes
    // like the delay tail.
}

void FeedbackProcessor::reset() noexcept
{
    // Called on patch reload together with the other nodes' resets: only
    // the estimator is cleared.  The loop itself drains through its
    // per-pass attenuation the moment the wiring stops feeding it.
    tailPeak = 0.0f;
}

StereoFrame FeedbackProcessor::processSample()
{
    const StereoFrame x  = input.getSample();
    const StereoFrame fb = fbIn.getSample();

    // Per-pass damping (0..0.95): the loop always loses energy, so the
    // recirculation stays stable no matter where the return comes from.
    const float fbAmount = amount != nullptr
        ? juce::jlimit (0.0f, 0.95f, amount->load())
        : 0.3f;

    const StereoFrame y { x.l + fb.l * fbAmount, x.r + fb.r * fbAmount };

    // Track the circulating energy (O(1), same policy as the delay tail).
    const float peak = std::max (std::abs (y.l), std::abs (y.r));
    tailPeak = juce::jmax (tailPeak * 0.99998f, peak);

    output.setSample (y);
    return y;
}

} // namespace smolfm