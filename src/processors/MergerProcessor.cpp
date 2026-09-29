/*
    MergerProcessor implementation.
*/

#include "MergerProcessor.h"

namespace smolfm
{

MergerProcessor::MergerProcessor()
    : Processor (ProcessorRole::generic)
{
    // An unconnected side contributes 0 on its channel (documented rule).
    leftInput.setDefaultValue (0.0f);
    rightInput.setDefaultValue (0.0f);

    // Display-only side hints: the UI can label in1 with "L" and in2 with
    // "R" on wires feeding this node.
    leftInput.setChannelSide (ChannelSide::left);
    rightInput.setChannelSide (ChannelSide::right);
}

void MergerProcessor::prepare (double /*newSampleRate*/)
{
    // Stateless: pure channel routing, no sample-rate dependent state.
}

void MergerProcessor::startNote()
{
    // No state to reset.
}

StereoFrame MergerProcessor::processSample()
{
    const StereoFrame leftSide  = leftInput.getSample();
    const StereoFrame rightSide = rightInput.getSample();

    const StereoFrame out { leftSide.l, rightSide.r };
    output.setSample (out);

    return out;
}

} // namespace smolfm