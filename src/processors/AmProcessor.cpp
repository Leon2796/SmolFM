/*
    AmProcessor implementation.
*/

#include "AmProcessor.h"

namespace smolfm
{

AmProcessor::AmProcessor()
    : Processor (ProcessorRole::generic)
{
    carrierIn  .setDefaultValue (0.0f);   // no carrier → silence
    modulatorIn.setDefaultValue (0.0f);   // no modulator → dry passthrough
}

AmProcessor::AmProcessor (std::atomic<float>* amountParam)
    : AmProcessor()
{
    amount = amountParam;
}

void AmProcessor::prepare (double newSampleRate)
{
    // Stateless: no filters, no phase to track.
    juce::ignoreUnused (newSampleRate);
}

void AmProcessor::startNote()
{
    // Nothing to reset.
}

StereoFrame AmProcessor::processSample()
{
    const float depth = amount != nullptr ? amount->load() : 0.0f;
    const StereoFrame mod = modulatorIn.getSample();   // expected in [-1, 1]
    const StereoFrame car = carrierIn.getSample();

    // Bias modulator into a positive-only gain factor:
    //   depth = 0   → gain = 1 (dry)
    //   depth = 1   → gain in [0, 2]
    // Per channel: a stereo modulator times a mono carrier yields stereo AM
    // for free; a centered modulator reproduces the previous mono behaviour.
    const float gainL = 1.0f - depth + depth * (mod.l * 0.5f + 0.5f) * 2.0f;
    const float gainR = 1.0f - depth + depth * (mod.r * 0.5f + 0.5f) * 2.0f;

    const StereoFrame result { car.l * gainL, car.r * gainR };
    output.setSample (result);
    return result;
}

} // namespace smolfm
