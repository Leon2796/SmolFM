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

float AmProcessor::processSample()
{
    const float depth = amount != nullptr ? amount->load() : 0.0f;
    const float mod   = modulatorIn.getSample();   // expected in [-1, 1]
    const float car   = carrierIn  .getSample();

    // Bias modulator into a positive-only gain factor:
    //   depth = 0   → gain = 1 (dry)
    //   depth = 1   → gain in [0, 2]
    const float gain = 1.0f - depth + depth * (mod * 0.5f + 0.5f) * 2.0f;

    const float result = car * gain;
    output.setSample (result);
    return result;
}

} // namespace smolfm
