/*
    GainProcessor implementation.
*/

#include "GainProcessor.h"

namespace smolfm
{

GainProcessor::GainProcessor (std::atomic<float>* gainParameter)
    : Processor (ProcessorRole::generic),
      gain (gainParameter)
{
    // Unwired input reads silence; a transparent idle node adds nothing.
    input.setDefaultValue (0.0f);
}

void GainProcessor::prepare (double /*newSampleRate*/)
{
    // Stateless: nothing to prepare.
}

void GainProcessor::startNote()
{
    // Stateless: no per-note state.
}

float GainProcessor::processSample()
{
    const float sourceSample = input.getSample();
    const float g = gain != nullptr ? gain->load() : 1.0f;

    const float sample = sourceSample * g;
    output.setSample (sample);

    return sample;
}

} // namespace smolfm