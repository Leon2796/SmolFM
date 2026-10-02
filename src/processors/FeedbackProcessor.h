/*
    FeedbackProcessor breaks cycles in the per-voice graph (TR-808 plan
    P3.6, A7: the real BD body's feedback/damping, Karplus-Strong, waveguide
    recirculation).

    The graph executes in insertion order, so a wiring loop always contains
    at least one edge whose producer runs *later* than its consumer in a
    tick; that consumer then reads the source's previous-tick frame — every
    cycle is automatically a 1-sample-delay cycle.  This node declares the
    loop entry explicitly and scales what circulates:

        out = in + amount * fb_in

    Ports:
        - in    (signal): the signal entering the loop (e.g. the exciter).
        - fb_in (signal): the loop return (wired from anywhere — the
          resonator body, a gain, or this node's own out for a plain
          one-pole loop).  Reads the previous tick's frame whenever its
          producer sits later in the render order, which is exactly the
          1-sample delay the cycle needs; the loop is stable for all
          amount <= 0.95 because every pass is attenuated.
        - out   (signal): the loop's entry value, carried on for the
          downstream chain (resonator body, shaper, output...).

    With amount = 0 the node is a transparent pass-through; with 0 < amount
    < 1 the recirculated energy decays per pass, like the delay's feedback.

    Feedback feeds back what it emitted last sample — sustained loops sit at
    in / (1 - amount) in the one-pole case, bounded by the 0.95 ceiling.
    Per-sample processing is real-time safe: two atomic reads, no state.
*/

#pragma once

#include "ProcessorPort.h"

#include <atomic>

namespace smolfm
{

/**
    Cycle breaker with per-pass damping; the circulating signal carries
    over between notes (like the delay tail) until the loop runs dry.
*/
class FeedbackProcessor final : public Processor
{
public:
    /**
        Create the feedback node.

        @param amountParameter atomic pointer to the per-pass amount (0-0.95)
    */
    explicit FeedbackProcessor (std::atomic<float>* amountParameter);

    void prepare (double newSampleRate) override;

    /**
        Keep the loop across notes: the recirculated body energy is part of
        the sound (a BD hit while the last one still rings adds onto it).
        Only reset() (new patch) clears the tail estimate.
    */
    void startNote() override;

    StereoFrame processSample() override;

    /**
        Clear only the tail estimate.  The loop's stored signal lives in the
        wiring (ports and downstream states), which the resonator's reset()
        and the next tick's silence drain; nothing here can hold energy.
    */
    void reset() noexcept;

    /**
        True while the loop still carries audible energy (voice lifetime
        policy, same contract as delay/resonator): the tail estimate follows
        the circulating frames with a slow decay constant.
    */
    bool hasEnergy() const noexcept { return tailPeak > 1.0e-3f; }

    InputPort& getInput()          noexcept { return input; }
    InputPort& getFeedbackInput()  noexcept { return fbIn; }
    OutputPort& getOutput()        noexcept { return output; }

private:
    std::atomic<float>* amount = nullptr;

    InputPort input { PortType::signal };
    InputPort fbIn  { PortType::signal };
    OutputPort output { PortType::signal, *this };

    // O(1) tail-energy estimate over the written frames; decays slowly so
    // it never under-reports a still-circulating loop by much.
    float tailPeak = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeedbackProcessor)
};

} // namespace smolfm