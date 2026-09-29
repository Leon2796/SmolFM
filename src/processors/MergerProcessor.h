/*
    MergerProcessor combines two side branches back into one stereo frame.

    Ports:
        - in1 (signal): LEFT branch  — contributes out.l.
        - in2 (signal): RIGHT branch — contributes out.r.
        - out (signal): the combined frame — (in1.l, in2.r).

    Per sample:

        out = (in1.l, in2.r)

    Each input contributes exactly one channel; an unconnected side reads
    its default 0 and therefore contributes silence on that side.  The ports
    carry a display-only ChannelSide hint (in1 = left, in2 = right) that the
    UI renders as an L/R badge on the wire.  The type stays PortType::signal
    (Welt A: ONE audio type), so mono-centered sources may feed both inputs.

    The processor is stateless and parameterless.
*/

#pragma once

#include "ProcessorPort.h"

namespace smolfm
{

/**
    Pure topology node: two signal inputs, one stereo output.

    Channel-wise recombination: the output's left channel comes from in1,
    the right channel from in2.  No gain, no mixing — a pure sample router.
*/
class MergerProcessor final : public Processor
{
public:
    MergerProcessor();

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    InputPort& getLeftInput() noexcept  { return leftInput; }
    InputPort& getRightInput() noexcept { return rightInput; }
    OutputPort& getOutput() noexcept    { return output; }

private:
    InputPort leftInput  { PortType::signal };
    InputPort rightInput { PortType::signal };

    OutputPort output { PortType::signal, *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MergerProcessor)
};

} // namespace smolfm