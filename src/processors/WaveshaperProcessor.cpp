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

    // Unwired drive_mod_in contributes no scaling: (1 + 0) = static drive.
    driveMod.setDefaultValue (0.0f);

    // Unwired vel input scales by 1.0: the sound stays exactly as before.
    velIn.setDefaultValue (1.0f);
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

    const float dBase = drive != nullptr ? juce::jlimit (0.0f, 1.0f, drive->load()) : 0.0f;
    const int shapeIndex = shape != nullptr
        ? juce::jlimit (0, 3, static_cast<int> (std::round (shape->load())))
        : 0;

    // Drive modulation: the connected value's stereo mean, clamped to
    // [-1, +1], multiplies the static drive by (1 + m) — an envelope or LFO
    // can run the curve from clean to driven per sample.  Unwired: static
    // drive, fully transparent.
    float d = dBase;
    if (driveMod.isConnected())
    {
        const StereoFrame pair = driveMod.getSample();
        const float m = juce::jlimit (-1.0f, 1.0f, 0.5f * (pair.l + pair.r));
        d = juce::jlimit (0.0f, 1.0f, dBase * (1.0f + m));
    }

    // Velocity (accent drive): the connected vel input's stereo mean (0-1)
    // scales the effective drive - harder hits enter the curve harder.
    // Unwired: fully sound-invariant.
    if (velIn.isConnected())
    {
        const StereoFrame vp = velIn.getSample();
        d = juce::jlimit (0.0f, 1.0f, d * 0.5f * (vp.l + vp.r));
    }

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

            case 3:
            {
                // Quantize: amplitude staircase (the crush on the value axis;
                // the hold node is the crush on the time axis).  Drive sets
                // the grid coarseness: levels per unit drop from 64 (drive 0,
                // quantization error <= 1/128 — near-transparent) down to 2
                // (drive 1 = sign steps).  A connected drive_mod_in re-sets
                // the grid per sample, so an envelope sweeps the crush.
                const int levels = juce::jlimit (2, 64,
                    static_cast<int> (std::round (64.0f * (1.0f - d))));
                return std::round (xChannel * static_cast<float> (levels))
                     / static_cast<float> (levels);
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