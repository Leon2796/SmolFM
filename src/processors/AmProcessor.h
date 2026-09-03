/*
    AmProcessor applies classic amplitude modulation.

    Ports:
        - carrier_in  (signal): the audio to be shaped.
        - modulator_in (signal): bipolar modulator (oscillator / LFO-style source).
        - out (signal): amplitude-modulated audio.

    Per sample:

        out = carrier * (1.0 - amount + amount * modulator)

    With amount = 0 the modulator has no effect (dry passthrough).
    With amount = 1 the carrier is fully modulated between 0 and 2x amplitude
    (ring-modulation-like at high depth, but biased so the carrier never
    cancels completely unless the modulator hits exactly -1).

    A disconnected input reads as 0, so an unwired node stays silent.
*/

#pragma once

#include "ProcessorPort.h"

#include <atomic>

namespace smolfm
{

/**
    AM modulator with depth control.

    Bipolar modulator in [-1, 1] is linearly mapped into a gain factor in
    [1 - amount, 1 + amount].  Stateless apart from the parameter pointer.
*/
class AmProcessor final : public Processor
{
public:
    AmProcessor();
    AmProcessor (std::atomic<float>* amountParam);

    void prepare (double newSampleRate) override;
    void startNote() override;
    float processSample() override;

    void setAmountParameter (std::atomic<float>* param) noexcept { amount = param; }

    InputPort& getCarrierInput() noexcept   { return carrierIn;  }
    InputPort& getModulatorInput() noexcept { return modulatorIn; }
    OutputPort& getOutput() noexcept        { return output; }

private:
    InputPort carrierIn   { PortType::signal };
    InputPort modulatorIn { PortType::signal };
    OutputPort output     { PortType::signal, *this };

    std::atomic<float>* amount = nullptr;   // APVTS atomic, 0..1

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmProcessor)
};

} // namespace smolfm
