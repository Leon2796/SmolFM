/*
    FrequencyScaleProcessor implementation.
*/

#include "FrequencyScaleProcessor.h"

#include <cmath>

namespace smolfm
{

FrequencyScaleProcessor::FrequencyScaleProcessor (std::atomic<float>* factorParameter,
                                                  std::atomic<float>* intervalParameter)
    : Processor (ProcessorRole::frequencyScale),
      factor (factorParameter),
      interval (intervalParameter)
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

StereoFrame FrequencyScaleProcessor::processSample()
{
    const float inputHz = freqInput.getSample().l;

    // The interval choice contributes its exact 12-TET ratio; reading the
    // raw choice value with round mirrors OscillatorProcessor's enum
    // parameters.  The lookup is a single clamp boundary: index 0 ("none")
    // is transparent and out-of-range values fall back to 1.0 in the helper,
    // so even a hostile automated value cannot produce a wild ratio.  The
    // free-range factor multiplies on top: f_out = f_in * ratio * factor.
    const int intervalIndex = interval != nullptr
        ? static_cast<int> (std::round (interval->load()))
        : 0;

    const float scale    = factor->load() * fscaleIntervalRatioFromIndex (intervalIndex);

    const float scaledHz = inputHz * scale;

    const StereoFrame result = StereoFrame::centered (scaledHz);
    output.setSample (result);
    return result;
}

} // namespace smolfm
