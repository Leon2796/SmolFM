/*
    WaveshaperProcessor implementation.
*/

#include "WaveshaperProcessor.h"

namespace smolfm
{

WaveshaperProcessor::WaveshaperProcessor (std::atomic<float>* driveParameter,
                                          std::atomic<float>* shapeParameter)
    : Processor (ProcessorRole::generic),
      drive (driveParameter),
      shape (shapeParameter)
{
    input.setDefaultValue (0.0f);
}

void WaveshaperProcessor::prepare (double /*newSampleRate*/)
{
    // Stateless: nothing to prepare.
}

void WaveshaperProcessor::startNote()
{
    // Stateless: no per-note state.
}

float WaveshaperProcessor::processSample()
{
    const float x = input.getSample();

    const float d = drive != nullptr ? juce::jlimit (0.0f, 1.0f, drive->load()) : 0.0f;
    const int shapeIndex = shape != nullptr
        ? juce::jlimit (0, 2, static_cast<int> (std::round (shape->load())))
        : 0;

    float y = 0.0f;

    switch (shapeIndex)
    {
        case 0:
        {
            // Soft clip: x / (1 + |x|) after driving the input.  Smooth,
            // tube-like saturation; keeps a hint of the original dynamics.
            const float driven = x * (1.0f + 9.0f * d);
            y = driven / (1.0f + std::abs (driven));
            break;
        }

        case 1:
        {
            // Hard clip: brick-wall clamp at +/-1 after driving.  Aggressive,
            // digital-sounding distortion; harmonics decay slowly.
            const float driven = x * (1.0f + 9.0f * d);
            y = juce::jlimit (-1.0f, 1.0f, driven);
            break;
        }

        case 2:
        {
            // Foldback: reflect the signal around +/-1 instead of clamping.
            // Metallic, inharmonic character; extreme drive creates sidebands.
            const float driven = x * (1.0f + 19.0f * d);
            y = driven;
            while (std::abs (y) > 1.0f)
                y = 2.0f * (y > 0.0f ? 1.0f : -1.0f) - y;
            break;
        }

        default:
            y = x;
            break;
    }

    output.setSample (y);
    return y;
}

} // namespace smolfm