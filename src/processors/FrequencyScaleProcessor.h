/*
    FrequencyScaleProcessor scales a frequency by a constant factor in the
    frequency domain.

    Ports:
        - freq_in (frequency): incoming frequency in Hertz, wired from
                               note.out, an FM stage or another scaler.
        - out     (frequency): the scaled frequency in Hertz.

    Per sample:

        f_out = f_in * factor

    Stateless: Hertz in, Hertz out.  The oscillator at the end of the chain
    keeps owning the phase integration, so this node can sit anywhere in a
    frequency cascade (transpose by octaves with factor 2.0, mute with 0.0).

    factor = 1 passes the frequency through unchanged, so an idle scaler in
    a chain is fully transparent.
*/

#pragma once

#include "ProcessorPort.h"

namespace smolfm
{

/**
    Chainable frequency scaler operating on Hertz values.

    The single input carries a frequency; the output carries that frequency
    multiplied by the factor parameter.
*/
class FrequencyScaleProcessor final : public Processor
{
public:
    /**
        Create the frequency scale node.

        @param factorParameter atomic pointer to the scale factor (0–10 in
                               the UI; 1.0 means transparent).
    */
    explicit FrequencyScaleProcessor (std::atomic<float>* factorParameter);

    void prepare (double newSampleRate) override;
    void startNote() override;
    float processSample() override;

    InputPort& getFreqInput() noexcept      { return freqInput; }
    OutputPort& getOutput() noexcept        { return output; }

private:
    std::atomic<float>* factor;

    InputPort freqInput { PortType::frequency };
    OutputPort output { PortType::frequency, *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyScaleProcessor)
};

} // namespace smolfm
