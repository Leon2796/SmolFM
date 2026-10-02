/*
    AdderProcessor sums two signal inputs samplewise.

    Ports:
        - in1 (signal): first summand.
        - in2 (signal): second summand.
        - out (signal): the sum — (in1.l + in2.l, in1.r + in2.r).

    Per sample:

        out = in1 + in2

    This is the bus-builder of the graph: parallel sources (oscillator
    clusters, layered drum voices) are folded into ONE mono/stereo signal
    before it continues through the shared signal domains (filter, shape,
    envelope).  It is NOT a stereo router like the merger — both inputs
    contribute to BOTH output channels.

    Each input reads its default 0 when unconnected, so a partially wired
    adder passes the connected side through unchanged: add is neutral, an
    unwired branch contributes silence.

    The processor is stateless and parameterless.
*/

#pragma once

#include "ProcessorPort.h"

namespace smolfm
{

/**
    Pure topology node: two signal inputs, one signal output.

    Channel-wise summation without clipping or gain: the output's channels
    are the floating-point sums of the corresponding input channels.  No
    parameters, no state — just additive mixing.
*/
class AdderProcessor final : public Processor
{
public:
    AdderProcessor();

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    InputPort& getInput1() noexcept { return input1; }
    InputPort& getInput2() noexcept { return input2; }
    OutputPort& getOutput() noexcept { return output; }

private:
    InputPort input1 { PortType::signal };
    InputPort input2 { PortType::signal };

    OutputPort output { PortType::signal, *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdderProcessor)
};

} // namespace smolfm