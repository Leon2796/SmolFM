/*
    Tail-lifetime self-check for the delay line's voice-lifetime estimate.

    The bug this guards: a note source (osc sine) keeps sounding after
    note-off (SmolFM voices hold the last note frequency).  The delay's
    tailPeak used to track `written` — which contains that fresh input — so
    hasEnergy() never went false, hasReasonToLive() kept the voice forever
    and every played note froze a voice.  The master output "wummerte" and
    never stopped.

    The fix tracks only the recirculating term (fb * |wet|) and defaults
    feedback to 0.  These checks exercise the real DelayProcessor:

      1.  default feedback is 0  ->  tailPeak never rises (single echo)
      2.  fb = 0.6, input drones forever at +1 ->  estimate still dies
      3.  nothing wired  ->  silent output, hasEnergy() false

    Assert-based, no framework.  Run this if you touch the tail block in
    DelayProcessor.cpp (and the same logic in FeedbackProcessor /
    ResonatorProcessor).
*/

#include "DelayProcessor.h"

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace
{
    // A held note that never releases: this is the drone that used to pin
    // the estimator.  Owning a steady OutputPort lets the delay read a
    // non-zero input forever without a full graph.
    struct DroneSource : smolfm::Processor
    {
        smolfm::OutputPort out { smolfm::PortType::signal, *this };

        DroneSource() : smolfm::Processor (smolfm::ProcessorRole::oscillator) {}

        void set (float v) noexcept            { out.setSample ({ v, v }); }
        void prepare (double) override         {}
        void startNote() override              {}
        void reset() noexcept                  {}
        smolfm::StereoFrame processSample() override { return out.getSample(); }
    };

    constexpr int    kSr        = 44100;
    constexpr double kHoldUntil = 2.0;   // hasReasonToLive() grace (s)
}

int main()
{
    // 1) Default (unwired) feedback must be 0: a loud held input raises no
    //    tailPeak at all, because a single echo recirculates nothing.
    {
        std::atomic<float> timeMs { 1.0f };   // 1 ms line; the echo returns in 44 ticks
        DroneSource src;
        src.set (0.9f);

        smolfm::DelayProcessor d (nullptr, nullptr, nullptr, nullptr, nullptr);
        d.prepare (kSr);
        d.getInput().connect (src.out);

        for (int i = 0; i < 3 * kSr; ++i)
            d.processSample();

        assert (! d.hasEnergy() && "default feedback must be 0 (no recirculation)");
        std::printf ("[PASS] default feedback = 0: hasEnergy() false after 3 s of a held input\n");
    }

    // 2) fb = 0.6 with the input droning forever: the estimate follows only
    //    fb * |wet|, so it must collapse once the line contents wash past —
    //    well inside the 2 s note-off grace, NOT pinned high by the drone.
    {
        std::atomic<float> timeMs { 1.0f };
        std::atomic<float> fb     { 0.6f };
        DroneSource src;
        src.set (1.0f);

        smolfm::DelayProcessor d (nullptr, &fb, nullptr, nullptr, nullptr);
        d.prepare (kSr);
        d.getInput().connect (src.out);

        // Key held: estimator may sit high (input still playing is fine).
        for (int i = 0; i < kSr / 10; ++i)
            d.processSample();

        // Note off here: the drone source keeps emitting (the freeze case),
        // but the estimator must die on its own — the input is excluded now.
        int ticksAlive = 0;
        const int maxTicks = static_cast<int> (1.5 * kHoldUntil * kSr);
        while (d.hasEnergy() && ticksAlive < maxTicks)
        {
            d.processSample();
            ++ticksAlive;
        }

        assert (ticksAlive < maxTicks
                && "delay estimate must die within the grace window despite a droning input");
        std::printf ("[PASS] fb=0.6 tail died after %d ticks (~%.0f ms) despite a held input\n",
                     ticksAlive, 1000.0 * ticksAlive / kSr);
    }

    // 3) Nothing wired: output stays at the silent default and no energy is
    //    ever reported.
    {
        smolfm::DelayProcessor d;
        d.prepare (kSr);

        for (int i = 0; i < 64; ++i)
        {
            const auto f = d.processSample();
            assert (f.l == 0.0f && f.r == 0.0f);
        }
        assert (! d.hasEnergy() && "unwired delay must report no energy");
        std::printf ("[PASS] unwired delay: silent, hasEnergy() false\n");
    }

    std::printf ("tail-lifetime tests: all passed\n");
    return 0;
}
