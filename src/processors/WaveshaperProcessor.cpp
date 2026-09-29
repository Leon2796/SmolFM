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

StereoFrame WaveshaperProcessor::processSample()
{
    const StereoFrame x = input.getSample();

    const float d = drive != nullptr ? juce::jlimit (0.0f, 1.0f, drive->load()) : 0.0f;
    const int shapeIndex = shape != nullptr
        ? juce::jlimit (0, 2, static_cast<int> (std::round (shape->load())))
        : 0;

    // The transfer function runs once per channel (standard stereo
    // distortion).  Both channels are shaped independently; M/S processing
    // would be a separate feature, not part of the stereo conversion.
    const auto transfer = [d, shapeIndex] (float xChannel) -> float
    {
        switch (shapeIndex)
        {
            case 0:
            {
                // Soft clip: x / (1 + |x|) after driving the input.  Smooth,
                // tube-like saturation; keeps a hint of the original dynamics.
                const float driven = xChannel * (1.0f + 9.0f * d);
                return driven / (1.0f + std::abs (driven));
            }

            case 1:
            {
                // Hard clip: brick-wall clamp at +/-1 after driving.  Aggressive,
                // digital-sounding distortion; harmonics decay slowly.
                const float driven = xChannel * (1.0f + 9.0f * d);
                return juce::jlimit (-1.0f, 1.0f, driven);
            }

            case 2:
            {
                // Foldback: reflect the signal around +/-1 instead of clamping.
                // Metallic, inharmonic character; extreme drive creates sidebands.
                float yChannel = xChannel * (1.0f + 19.0f * d);
                while (std::abs (yChannel) > 1.0f)
                    yChannel = 2.0f * (yChannel > 0.0f ? 1.0f : -1.0f) - yChannel;
                return yChannel;
            }

            default:
                return xChannel;
        }
    };

    const StereoFrame y { transfer (x.l), transfer (x.r) };
    output.setSample (y);
    return y;
}

} // namespace smolfm