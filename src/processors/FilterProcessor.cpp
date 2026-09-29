/*
    FilterProcessor implementation.
*/

#include "FilterProcessor.h"

namespace smolfm
{

FilterProcessor::FilterProcessor (std::atomic<float>* cutoffParameter,
                                  std::atomic<float>* resonanceParameter,
                                  std::atomic<float>* modeParameter)
    : Processor (ProcessorRole::generic),
      cutoff (cutoffParameter),
      resonance (resonanceParameter),
      mode (modeParameter)
{
    // Unwired cutoff_mod_in falls back to the static cutoff parameter, so the
    // modulation port itself can default to any value (it is only read when
    // connected).
    input.setDefaultValue (0.0f);
    cutoffMod.setDefaultValue (440.0f);
    resonanceMod.setDefaultValue (0.0f);
}

void FilterProcessor::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    ic1eq = StereoFrame {};
    ic2eq = StereoFrame {};
}

void FilterProcessor::startNote()
{
    // Re-zero the integrators so every note starts its filtering from the
    // same point (no leftover rumble from the previous note's tail).
    ic1eq = StereoFrame {};
    ic2eq = StereoFrame {};
}

void FilterProcessor::reset() noexcept
{
    // Same zeroing as startNote(), but independent of a note event: a new
    // patch must not inherit the previous patch's filter ring.
    ic1eq = StereoFrame {};
    ic2eq = StereoFrame {};
}

StereoFrame FilterProcessor::processSample()
{
    const StereoFrame x = input.getSample();

    // Cutoff: the modulation input (frequency domain) replaces the static
    // parameter entirely when connected — the static cutoff is the fallback,
    // not an offset.
    const float fc = cutoffMod.isConnected()
        ? juce::jlimit (20.0f, 20000.0f, cutoffMod.getSample().l)
        : juce::jlimit (20.0f, 20000.0f, cutoff != nullptr ? cutoff->load() : 1000.0f);

    const float resBase = resonance != nullptr ? resonance->load() : 0.0f;
    const int modeIndex = mode != nullptr
        ? juce::jlimit (0, 3, static_cast<int> (std::round (mode->load())))
        : 0;

    // Resonance modulation: the signal input multiplies the static value by
    // (1 + m) with m clamped to [-1, +1] (stereo mean), so an LFO, a noise
    // source or an ADSR-shaped constant can open and close the Q.  Unwired:
    // static resonance, fully transparent.
    float res = juce::jlimit (0.0f, 1.0f, resBase);
    if (resonanceMod.isConnected())
    {
        const StereoFrame pair = resonanceMod.getSample();
        const float m = juce::jlimit (-1.0f, 1.0f, 0.5f * (pair.l + pair.r));
        res = juce::jlimit (0.0f, 1.0f, resBase * (1.0f + m));
    }

    // Trapezoidal state-variable filter (Simper SVF), re-derived per sample.
    // The coefficients depend only on the cutoff and are shared by both
    // channels; only the TPT state is a left/right pair.
    const float g  = std::tan (juce::MathConstants<float>::pi
                               * juce::jlimit (0.0005f, 0.49f, fc / (float) sampleRate));
    const float k  = 2.0f * (1.0f - 0.99f * res);
    const float a1 = 1.0f / (1.0f + g * (g + k));
    const float a2 = g * a1;
    const float a3 = g * a2;

    const auto processChannel = [&] (float channelInput, float& ic1, float& ic2) -> float
    {
        const float v3 = channelInput - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;

        float channelOutput = 0.0f;
        switch (modeIndex)
        {
            case 1:  channelOutput = v1;                         break;   // band pass
            case 2:  channelOutput = channelInput - k * v1 - v2; break;   // high pass
            case 3:  channelOutput = channelInput - k * v1;      break;   // notch (HP + LP)
            default: channelOutput = v2;                         break;   // low pass
        }
        return channelOutput;
    };

    const StereoFrame y {
        processChannel (x.l, ic1eq.l, ic2eq.l),
        processChannel (x.r, ic1eq.r, ic2eq.r)
    };

    output.setSample (y);
    return y;
}

} // namespace smolfm