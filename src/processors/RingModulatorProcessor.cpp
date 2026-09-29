/*
    RingModulatorProcessor implementation.
*/

#include "RingModulatorProcessor.h"

namespace smolfm
{

RingModulatorProcessor::RingModulatorProcessor()
    : Processor (ProcessorRole::ringModulator)
{
    // A disconnected factor contributes 0, so the product stays 0 and the
    // node is silent until both sides are wired.
    input1.setDefaultValue (0.0f);
    input2.setDefaultValue (0.0f);
}

void RingModulatorProcessor::prepare (double newSampleRate)
{
    // Stateless: signal in, signal out.  No sample-rate dependent state.
    juce::ignoreUnused (newSampleRate);
}

void RingModulatorProcessor::startNote()
{
    // No state to reset.
}

StereoFrame RingModulatorProcessor::processSample()
{
    const StereoFrame factor1 = input1.getSample();
    const StereoFrame factor2 = input2.getSample();

    const StereoFrame result { factor1.l * factor2.l, factor1.r * factor2.r };
    output.setSample (result);
    return result;
}

} // namespace smolfm
