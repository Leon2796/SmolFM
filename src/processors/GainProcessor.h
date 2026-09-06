/*
    GainProcessor boosts or attenuates a signal by a constant factor.

    Ports:
        - in  (signal): incoming audio.
        - out (signal): the scaled audio.

    Per sample:

        y_n = gain * x_n

    gain = 1 passes the signal through unchanged, so an idle (or unwired)
    gain node is fully transparent.  Negative values invert the phase.
*/

#pragma once

#include "ProcessorPort.h"

namespace smolfm
{

/**
    Chainable gain stage operating on audio signals.

    The single input carries a signal; the output carries that signal
    multiplied by the gain parameter.
*/
class GainProcessor final : public Processor
{
public:
    /**
        Create the gain node.

        @param gainParameter atomic pointer to the gain factor (0–10 in the
                             UI; 1.0 means transparent).
    */
    explicit GainProcessor (std::atomic<float>* gainParameter);

    void prepare (double newSampleRate) override;
    void startNote() override;
    float processSample() override;

    InputPort& getInput() noexcept   { return input; }
    OutputPort& getOutput() noexcept { return output; }

private:
    std::atomic<float>* gain;

    InputPort input  { PortType::signal };
    OutputPort output { PortType::signal, *this };
};

} // namespace smolfm