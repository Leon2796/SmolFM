/*
    FMModulationProcessor implementation.
*/

#include "FMModulationProcessor.h"
#include <cmath>

namespace smolfm
{

FMModulationProcessor::FMModulationProcessor (std::atomic<float>* amount)
    : Processor (ProcessorRole::fmModulator),
      fmAmount (amount)
{
    // A disconnected input must be silent, not DC.
    carrierInput.setDefaultValue (0.0f);
    modulatorInput.setDefaultValue (0.0f);
}

void FMModulationProcessor::prepare (double newSampleRate)
{
    if (newSampleRate > 0.0)
        sampleRate = newSampleRate;
}

void FMModulationProcessor::startNote()
{
    phase = 0.0f;
    lastCarrierSample = 0.0f;
    samplesSinceCrossing = 0;
    lastMeasuredPeriodInSamples = 0.0f;
}

float FMModulationProcessor::processSample()
{
    const float carrier = carrierInput.getSample();
    const float modulator = modulatorInput.getSample();

    // Rising zero-crossing detection on the carrier input: the time between
    // two crossings is one period in samples.  Whatever oscillator the user
    // wired in (sine, saw, ...) drives our pitch — the node never generates
    // its own tone.
    if (lastCarrierSample <= 0.0f && carrier > 0.0f)
    {
        if (samplesSinceCrossing > 0)
            lastMeasuredPeriodInSamples = static_cast<float> (samplesSinceCrossing);

        samplesSinceCrossing = 0;
    }
    else
    {
        ++samplesSinceCrossing;
    }

    lastCarrierSample = carrier;

    // Advance own phase by one tick of the measured period.  Before the
    // first crossing we don't know the pitch yet, so we stay silent rather
    // than emit a DC step.
    if (lastMeasuredPeriodInSamples > 0.0f)
    {
        phase += juce::MathConstants<float>::twoPi / lastMeasuredPeriodInSamples;
        while (phase >= juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;
    }

    const float phaseOffset = modulator * fmAmount->load();
    const float sample = std::sin (phase + phaseOffset);

    output.setSample (sample);
    return sample;
}

} // namespace smolfm
