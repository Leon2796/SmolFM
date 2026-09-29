/*
    GainProcessor boosts or attenuates a signal by a constant factor.

    Ports:
        - in           (signal): incoming audio.
        - factor_mod_in (signal): optional multiplier on the gain.  The
          connected value's stereo mean is clamped to [-1, +1] and the
          effective gain becomes gain * (1 + m), clamped to 0..10.
          Unconnected: the static gain runs as-is.  An LFO here turns the
          node into a clean VCA (linear tremolo/swells); a fast square is a
          rhythmic gate.
        - out (signal): the scaled audio.

    Per sample (per channel):

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
    StereoFrame processSample() override;

    InputPort& getInput() noexcept   { return input; }
    InputPort& getModInput() noexcept { return modInput; }
    OutputPort& getOutput() noexcept { return output; }

private:
    std::atomic<float>* gain;

    InputPort input  { PortType::signal };
    InputPort modInput { PortType::signal };
    OutputPort output { PortType::signal, *this };
};

} // namespace smolfm