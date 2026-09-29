/*
    SplitterProcessor divides one stereo frame into its two side branches.

    Ports:
        - in   (signal): incoming audio.
        - outL (signal): the LEFT branch — (in.l, 0).
        - outR (signal): the RIGHT branch — (0, in.r).

    Per sample:

        outL = (in.l, 0)
        outR = (0, in.r)

    Both outputs stay PortType::signal (Welt A: ONE audio type), so each
    branch feeds any standard processor without converters.  The ports carry
    a display-only ChannelSide hint (left/right) that the UI renders as an
    L/R badge on the wire.  Each branch contains only its own side, so a
    split branch routed straight into the master output contributes to
    exactly one channel — no bleed.

    The processor is stateless and parameterless.
*/

#pragma once

#include "ProcessorPort.h"

namespace smolfm
{

/**
    Pure topology node: one stereo input, two per-side outputs.

    No DSP, no parameters: each output copies the matching channel of the
    input and silences the other.  processSample() returns the input frame;
    consumers read the explicit side outputs (outL / outR).
*/
class SplitterProcessor final : public Processor
{
public:
    SplitterProcessor();

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    InputPort& getInput() noexcept       { return input; }
    OutputPort& getLeftOutput() noexcept { return leftOutput; }
    OutputPort& getRightOutput() noexcept { return rightOutput; }

private:
    InputPort input { PortType::signal };

    OutputPort leftOutput  { PortType::signal, *this };
    OutputPort rightOutput { PortType::signal, *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SplitterProcessor)
};

} // namespace smolfm