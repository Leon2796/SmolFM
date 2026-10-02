/*
    AdderProcessor implementation.
*/

#include "AdderProcessor.h"

namespace smolfm
{

AdderProcessor::AdderProcessor()
    : Processor (ProcessorRole::generic)
{
    // An unconnected summand contributes 0, so the adder passes the
    // connected input through unchanged (documented neutrality rule).
    input1.setDefaultValue (0.0f);
    input2.setDefaultValue (0.0f);
}

void AdderProcessor::prepare (double /*newSampleRate*/)
{
    // Stateless: pure per-sample addition, no sample-rate dependent state.
}

void AdderProcessor::startNote()
{
    // No state to reset.
}

StereoFrame AdderProcessor::processSample()
{
    const StereoFrame a = input1.getSample();
    const StereoFrame b = input2.getSample();

    const StereoFrame out { a.l + b.l, a.r + b.r };
    output.setSample (out);

    return out;
}

} // namespace smolfm