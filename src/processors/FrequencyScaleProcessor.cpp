/*
    FrequencyScaleProcessor implementation.
*/

#include "FrequencyScaleProcessor.h"

namespace smolfm
{

FrequencyScaleProcessor::FrequencyScaleProcessor (std::atomic<float>* factorParameter)
    : Processor (ProcessorRole::frequencyScale),
      factor (factorParameter)
{
    // A disconnected frequency input means 0 Hz; the oscillator at the end
    // of the chain maps that to its 440 Hz guard frequency.
    freqInput.setDefaultValue (0.0f);
}

void FrequencyScaleProcessor::prepare (double newSampleRate)
{
    // Stateless: Hertz in, Hertz out.  No sample-rate dependent state.
    juce::ignoreUnused (newSampleRate);
}

void FrequencyScaleProcessor::startNote()
{
    // No state to reset.
}

float FrequencyScaleProcessor::processSample()
{
    const float inputHz  = freqInput.getSample();
    const float scale    = factor->load();

    const float scaledHz = inputHz * scale;

    output.setSample (scaledHz);
    return scaledHz;
}

} // namespace smolfm
