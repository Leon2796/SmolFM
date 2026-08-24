/*
    FMModulationProcessor applies frequency modulation between two signals.

    It has two inputs and one output, all plain signal ports (no Hertz):
        - carrier:    the waveform whose phase is modulated
        - modulator:  the signal that pushes the carrier's phase

    The node owns only a phase accumulator.  Per sample it measures the
    carrier input's period via zero-crossings and evaluates
    sin(phase + modulator * fmAmount).  That is real phase-modulation FM:
    the modulator stays a signal, it is never turned back into a frequency.
*/

#pragma once

#include "ProcessorPort.h"

namespace smolfm
{

/**
    Two-signal FM node.

    Both inputs are signal ports.  The output is a sine whose phase follows
    the wired carrier and is offset by (modulator * fmAmount).
*/
class FMModulationProcessor final : public Processor
{
public:
    /**
        Create the FM node.

        @param fmAmount atomic pointer to the FM index (radians of phase
                        offset per unit of modulator signal)
    */
    explicit FMModulationProcessor (std::atomic<float>* fmAmount);

    void prepare (double newSampleRate) override;
    void startNote() override;
    float processSample() override;

    InputPort& getCarrierInput() noexcept   { return carrierInput; }
    InputPort& getModulatorInput() noexcept { return modulatorInput; }
    OutputPort& getOutput() noexcept        { return output; }

private:
    std::atomic<float>* fmAmount;

    InputPort carrierInput { PortType::signal };
    InputPort modulatorInput { PortType::signal };
    OutputPort output { PortType::signal, *this };

    // Carrier pitch is measured from the wired signal's zero-crossings, so
    // the node works with any oscillator the user connects instead of
    // re-creating the waveform internally.
    double sampleRate = 44100.0;
    float phase = 0.0f;
    float lastCarrierSample = 0.0f;
    int   samplesSinceCrossing = 0;
    float lastMeasuredPeriodInSamples = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FMModulationProcessor)
};

} // namespace smolfm
