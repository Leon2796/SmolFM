/*
    RingModulatorProcessor multiplies two signals sample by sample.

    Ports:
        - in1 (signal): first factor, typically a carrier oscillator.
        - in2 (signal): second factor, typically a modulator oscillator.
        - out (signal): the product in1 * in2.

    Per sample:

        out = in1 * in2

    Ring modulation is plain amplitude multiplication.  With two bipole
    audio signals the result carries the sum and difference of the input
    partials and no carrier residue — the inharmonic sidebands give bells,
    tremolo and metallic timbres.

    A disconnected input reads as 0, so an unwired port silences the output
    instead of leaking DC (matching a diode ring modulator's behaviour).
*/

#pragma once

#include "ProcessorPort.h"

namespace smolfm
{

/**
    Two-quadrant signal multiplier.

    Both inputs are audio-rate signals; the output is their sample-wise
    product.  Stateless and cheap: no parameters, no phase, no memory.
*/
class RingModulatorProcessor final : public Processor
{
public:
    RingModulatorProcessor();

    void prepare (double newSampleRate) override;
    void startNote() override;
    float processSample() override;

    InputPort& getInput1() noexcept { return input1; }
    InputPort& getInput2() noexcept { return input2; }
    OutputPort& getOutput() noexcept { return output; }

private:
    InputPort input1 { PortType::signal };
    InputPort input2 { PortType::signal };
    OutputPort output { PortType::signal, *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingModulatorProcessor)
};

} // namespace smolfm
