/*
    FMModulationProcessor applies true frequency modulation in the
    frequency domain.

    Ports:
        - freq_in       (frequency): carrier base frequency in Hertz, wired
                          from note.out or from the previous FM stage's out.
        - modulator_in  (signal):    the signal that bends the frequency.
        - amount_mod_in (signal):    optional multiplier on the FM amount.
                          The connected value's stereo mean is clamped to
                          [-1, +1] and the effective index becomes
                          amount * (1 + m), clamped to 0..10.  Unconnected:
                          the static amount runs as-is.
        - vel           (velocity): optional per-note velocity input (0-1).
          Connected: the stereo mean scales the FM index (harder hits gain
          brighter, more metallic sidebands).  Unconnected: transparent.
        - out           (frequency): instantaneous carrier frequency.

    Per sample (modulator collapsed to one scalar — see below):

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

    Stereo compatibility: modulator_in carries a StereoFrame pair, but the
    stage emits exactly one instantaneous frequency (the oscillator owns a
    single phase).  The pair is collapsed by averaging (L + R) * 0.5, so
    centered (mono) modulators keep exactly the modulation depth they had
    before the stereo conversion; a plain sum would double the deviation.
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
    StereoFrame processSample() override;

    InputPort& getFreqInput() noexcept      { return freqInput; }
    InputPort& getModulatorInput() noexcept { return modulatorInput; }
    InputPort& getAmountModInput() noexcept { return amountMod; }
    InputPort& getVelInput() noexcept       { return velIn; }
    OutputPort& getOutput() noexcept        { return output; }

private:
    std::atomic<float>* fmAmount;

    InputPort freqInput { PortType::frequency };
    InputPort modulatorInput { PortType::signal };
    InputPort amountMod { PortType::signal };
    InputPort velIn      { PortType::velocity };
    OutputPort output { PortType::frequency, *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FMModulationProcessor)
};

} // namespace smolfm
