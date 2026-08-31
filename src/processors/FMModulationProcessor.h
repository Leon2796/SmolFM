/*
    FMModulationProcessor applies true frequency modulation in the
    frequency domain.

    Ports:
        - freq_in       (frequency): carrier base frequency in Hertz, wired
                          from note.out or from the previous FM stage's out.
        - modulator_in  (signal):    the signal that bends the frequency.
        - out           (frequency): instantaneous carrier frequency.

    Per sample:

        f_out = f_in * (1 + amount * modulator)

    This is real FM, not phase modulation: the modulator never touches a
    phase directly.  It only scales the Hertz value; the phase accumulator
    lives in the oscillator at the end of the chain.  The oscillator
    integrates the instantaneous frequency and renders whatever waveform
    (sine/saw/square/triangle/wavetable) the user picked — which is what
    makes the stages chainable:

        note.out -> fm0.freq_in -> fm0.out -> fm1.freq_in -> ... -> osc.note_in

    The deviation is proportional to the incoming carrier frequency, so the
    modulation index (and therefore the timbre) is identical on every key.
    Negative instantaneous frequencies (through-zero FM) are fine: the
    oscillator's phase simply runs backwards.

    amount = 0 or no modulator wired: f_out = f_in — the stage is fully
    transparent, so idle links in an FM chain cost nothing.
*/

#pragma once

#include "ProcessorPort.h"

namespace smolfm
{

/**
    Chainable FM stage operating on Hertz values.

    Inputs are the carrier frequency and the modulator signal; the output is
    the instantaneous frequency that drives an oscillator's note_in port.
*/
class FMModulationProcessor final : public Processor
{
public:
    /**
        Create the FM node.

        @param fmAmount atomic pointer to the FM index.  A value of 1 lets
                        the modulator push the carrier up to +/-100 % around
                        its base frequency.
    */
    explicit FMModulationProcessor (std::atomic<float>* fmAmount);

    void prepare (double newSampleRate) override;
    void startNote() override;
    float processSample() override;

    InputPort& getFreqInput() noexcept      { return freqInput; }
    InputPort& getModulatorInput() noexcept { return modulatorInput; }
    OutputPort& getOutput() noexcept        { return output; }

private:
    std::atomic<float>* fmAmount;

    InputPort freqInput { PortType::frequency };
    InputPort modulatorInput { PortType::signal };
    OutputPort output { PortType::frequency, *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FMModulationProcessor)
};

} // namespace smolfm
