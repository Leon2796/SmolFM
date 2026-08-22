/*
    FMModulationProcessor applies frequency modulation to a carrier oscillator.

    It has two inputs and one output:
        - modulator: the signal used as the phase offset source
        - carrier:   the oscillator whose phase is modulated

    The carrier input is expected to already be an oscillator that accepts a
    phase offset.  This processor reads the modulator sample, scales it by the
    FM amount parameter, and passes it as a phase offset to the carrier
    oscillator.
*/

#pragma once

#include "../SimpleOscillator.h"
#include "ProcessorPort.h"

namespace smolfm
{

/**
    FM modulator node.

    Internally owns the carrier oscillator.  The modulator signal comes from a
    connected OutputPort.
*/
class FMModulationProcessor final : public Processor
{
public:
    /**
        Create an FM modulator node.

        @param carrierFrequency atomic pointer to carrier frequency in Hz
        @param carrierWaveform  atomic pointer to carrier waveform index
        @param fmAmount         atomic pointer to FM amount
    */
    FMModulationProcessor (std::atomic<float>* carrierFrequency,
                           std::atomic<float>* carrierWaveform,
                           std::atomic<float>* fmAmount);

    void prepare (double newSampleRate) override;
    void startNote() override;
    float processSample() override;

    /**
        Access the input ports for graph wiring.
    */
    InputPort& getModulatorInput() noexcept
    {
        return modulatorInput;
    }

    /**
        Optional note input that overrides the carrier frequency parameter.
    */
    InputPort& getCarrierNoteInput() noexcept
    {
        return carrierNoteInput;
    }

    OutputPort& getOutput() noexcept
    {
        return output;
    }

private:
    SimpleOscillator carrier;

    std::atomic<float>* carrierFrequency;
    std::atomic<float>* carrierWaveform;
    std::atomic<float>* fmAmount;

    InputPort modulatorInput;
    InputPort carrierNoteInput;
    OutputPort output;
};

} // namespace smolfm
