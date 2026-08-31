/*
    FMModulationProcessor implementation.
*/

#include "FMModulationProcessor.h"

namespace smolfm
{

FMModulationProcessor::FMModulationProcessor (std::atomic<float>* amount)
    : Processor (ProcessorRole::fmModulator),
      fmAmount (amount)
{
    // A disconnected frequency input means 0 Hz; the oscillator at the end
    // of the chain maps that to its 440 Hz guard frequency.
    freqInput.setDefaultValue (0.0f);

    // A disconnected modulator must contribute no deviation, not DC.
    modulatorInput.setDefaultValue (0.0f);
}

void FMModulationProcessor::prepare (double newSampleRate)
{
    // Stateless: Hertz in, Hertz out.  No sample-rate dependent state.
    juce::ignoreUnused (newSampleRate);
}

void FMModulationProcessor::startNote()
{
    // No state to reset.
}

float FMModulationProcessor::processSample()
{
    const float carrierHz = freqInput.getSample();
    const float amount    = fmAmount->load();

    // True FM: the deviation is proportional to the carrier frequency, so
    // the modulation index (and therefore the timbre) stays the same on
    // every key.  amount = 0 or no modulator wired: the frequency passes
    // through unchanged and the stage is fully transparent.
    const float modulator = (amount > 0.0f && modulatorInput.isConnected())
                          ? modulatorInput.getSample()
                          : 0.0f;

    const float instantHz = carrierHz * (1.0f + amount * modulator);

    output.setSample (instantHz);
    return instantHz;
}

} // namespace smolfm
