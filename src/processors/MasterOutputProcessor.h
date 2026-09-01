/*
    MasterOutputProcessor is the final stage of the per-voice graph.

    It mixes up to eight signal inputs into one output sample and applies the
    master volume.  The current output level is published via an atomic peak
    value so the UI can show a meter without touching the audio thread.

    Per sample:

        out = masterLevel * sum(in1..in8)

    The ADSR envelopes are no longer the end of the chain — they shape their
    own input and pass the result on, typically into one of these inputs.
*/

#pragma once

#include "ProcessorPort.h"

namespace smolfm
{

/**
    Stereo-summer / master stage with a level meter tap.

    The processor owns no state beyond the level smoothing.  The peak value
    decays slowly so a UI timer can poll it at ~30 Hz without flicker.
*/
class MasterOutputProcessor final : public Processor
{
public:
    static constexpr int numInputs = 8;

    explicit MasterOutputProcessor (std::atomic<float>* levelParameter);

    void prepare (double newSampleRate) override;
    void startNote() override;
    float processSample() override;

    InputPort& getInput (int index) noexcept
    {
        return inputs[static_cast<size_t> (index)];
    }

    /** Current peak level in [0, ~1+] for the UI meter.  Thread-safe. */
    float getPeakLevel() const noexcept
    {
        return peakLevel.load (std::memory_order_acquire);
    }

private:
    std::atomic<float>* level;

    std::array<InputPort, numInputs> inputs;

    // Peak follower with fast attack and slow release, polled by the UI.
    std::atomic<float> peakLevel { 0.0f };
    float smoothedPeak = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterOutputProcessor)
};

} // namespace smolfm
