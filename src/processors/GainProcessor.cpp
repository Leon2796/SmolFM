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

    // Unwired factor_mod_in contributes no scaling: (1 + 0) = static gain.
    modInput.setDefaultValue (0.0f);
}

void GainProcessor::prepare (double /*newSampleRate*/)
{
    // Stateless: nothing to prepare.
}

void GainProcessor::startNote()
{
    // Stateless: no per-note state.
}

StereoFrame GainProcessor::processSample()
{
    const StereoFrame sourceSample = input.getSample();
    const float gBase = gain != nullptr ? gain->load() : 1.0f;

    // Gain modulation: the connected value's stereo mean, clamped to
    // [-1, +1], multiplies the static factor by (1 + m) — a clean VCA
    // scaling without the AM ring-modulator's depth semantics.  Unwired:
    // static gain, fully transparent.
    float g = gBase;
    if (modInput.isConnected())
    {
        const StereoFrame pair = modInput.getSample();
        const float m = juce::jlimit (-1.0f, 1.0f, 0.5f * (pair.l + pair.r));
        g = juce::jlimit (0.0f, 10.0f, gBase * (1.0f + m));
    }

    const StereoFrame sample { sourceSample.l * g, sourceSample.r * g };
    output.setSample (sample);

    return sample;
}

} // namespace smolfm