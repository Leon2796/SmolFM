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
}

void FilterProcessor::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    ic1eq = 0.0f;
    ic2eq = 0.0f;
}

void FilterProcessor::startNote()
{
    // Re-zero the integrators so every note starts its filtering from the
    // same point (no leftover rumble from the previous note's tail).
    ic1eq = 0.0f;
    ic2eq = 0.0f;
}

float FilterProcessor::processSample()
{
    const float x = input.getSample();

    // Cutoff: the modulation input (frequency domain) replaces the static
    // parameter entirely when connected — the static cutoff is the fallback,
    // not an offset.
    const float fc = cutoffMod.isConnected()
        ? juce::jlimit (20.0f, 20000.0f, cutoffMod.getSample())
        : juce::jlimit (20.0f, 20000.0f, cutoff != nullptr ? cutoff->load() : 1000.0f);

    const float res = resonance != nullptr ? juce::jlimit (0.0f, 1.0f, resonance->load()) : 0.0f;
    const int modeIndex = mode != nullptr
        ? juce::jlimit (0, 3, static_cast<int> (std::round (mode->load())))
        : 0;

    // Trapezoidal state-variable filter (Simper SVF), re-derived per sample.
    const float g  = std::tan (juce::MathConstants<float>::pi
                               * juce::jlimit (0.0005f, 0.49f, fc / (float) sampleRate));
    const float k  = 2.0f * (1.0f - 0.99f * res);
    const float a1 = 1.0f / (1.0f + g * (g + k));
    const float a2 = g * a1;
    const float a3 = g * a2;

    const float v3 = x - ic2eq;
    const float v1 = a1 * ic1eq + a2 * v3;
    const float v2 = ic2eq + a2 * ic1eq + a3 * v3;
    ic1eq = 2.0f * v1 - ic1eq;
    ic2eq = 2.0f * v2 - ic2eq;

    float y = 0.0f;
    switch (modeIndex)
    {
        case 1:  y = v1;                        break;   // band pass
        case 2:  y = x - k * v1 - v2;           break;   // high pass
        case 3:  y = x - k * v1;                break;   // notch (HP + LP)
        default: y = v2;                        break;   // low pass
    }

    output.setSample (y);
    return y;
}

} // namespace smolfm