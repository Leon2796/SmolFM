/*
    SplitterProcessor implementation.
*/

#include "SplitterProcessor.h"

namespace smolfm
{

SplitterProcessor::SplitterProcessor()
    : Processor (ProcessorRole::generic)
{
    // An unwired input reads silence, so both branches are silent too.
    input.setDefaultValue (0.0f);

    // Display-only side hints: the UI can label outL with "L" and outR
    // with "R".  connect() and the audio path ignore them completely.
    leftOutput.setChannelSide (ChannelSide::left);
    rightOutput.setChannelSide (ChannelSide::right);
}

void SplitterProcessor::prepare (double /*newSampleRate*/)
{
    // Stateless: pure channel routing, no sample-rate dependent state.
}

void SplitterProcessor::startNote()
{
    // No state to reset.
}

StereoFrame SplitterProcessor::processSample()
{
    const StereoFrame in = input.getSample();

    const StereoFrame left  { in.l, 0.0f };
    const StereoFrame right { 0.0f, in.r };

    leftOutput.setSample (left);
    rightOutput.setSample (right);

    // The return value is only the graph's pass-through convention; real
    // consumers read the explicit side outputs via their ports.
    return in;
}

} // namespace smolfm