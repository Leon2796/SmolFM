/*
    Effective-control telemetry self-check.

    Telemetry contract (what the ghost needle publishes): the needle is
    slider × mod-port only.  Velocity deliberately stays sound-only — it
    still scales the audio path, but it never moves the needle (a per-hit
    accent is articulation, not a patch move; flickering the 30 Hz display
    with every hit is noise).  The improvement plan
    (doc/improvement-plans/modulated-control-visualisation-gap-analysis.md,
    P1/P2) asks every mod-ported control to publish that needle value —
    otherwise the UI's ghost needle has nothing to poll.

    Since the anti-aliasing upgrade (NeedleWindowTelemetry in
    src/EffectiveRangeTelemetry.h) the published value is the min/max band
    of the effective value over the last 256 rendered samples: a per-sample
    overwrite aliased against the overlay's 30 Hz poll and made the needle
    jump to arbitrary slider points even for a pure sine.  These checks
    exercise the real processors:

      1. FM amount: no-data before first render, static passthrough,
         modulated scaling, both clamps, velocity never moves the needle
      2. Waveshaper drive: static passthrough, modulated scaling, velocity
         never moves the needle
      3. Filter: cutoff_mod_in REPLACES the slider (the strongest UI lie
         the plan names), velocity never moves the needle, resonance
         modulation
      4. Delay: time scaling, converted back to ms (free and synced modes)
      5. Resonator: q modulation, unwired = static
      6. Gain factor: the gain node has a knob and a factor_mod_in, so it
         carries its needle like the others (+ velocity exclusion,
         window-band and window-replaces-history pins)

    Assert-based, no framework.  Run in the Debug configuration whenever a
    telemetry window or an effective-value computation moves: a Release
    (NDEBUG) build compiles the asserts away and exits 1 on purpose.
*/

#include "FMModulationProcessor.h"
#include "WaveshaperProcessor.h"
#include "FilterProcessor.h"
#include "GainProcessor.h"
#include "DelayProcessor.h"
#include "ResonatorProcessor.h"
#include "../EffectiveRangeTelemetry.h"     // telemetry range protocol

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace
{
    // A controllable source the tests wire into any mod port (same helper
    // idea as TailLifetimeTests' DroneSource).  The port type defaults to
    // signal; typed variants are needed because InputPort::connect()
    // rejects mismatched types, so a velocity input refuses a signal source.
    struct TestSource : smolfm::Processor
    {
        smolfm::OutputPort out;

        explicit TestSource (smolfm::PortType portType = smolfm::PortType::signal)
            : smolfm::Processor (smolfm::ProcessorRole::generic), out (portType, *this) {}

        void set (float v) noexcept                  { out.setSample ({ v, v }); }
        void prepare (double) override               {}
        void startNote() override                    {}
        smolfm::StereoFrame processSample() override { return out.getSample(); }
    };

    // Frequency variant (cutoff_mod_in carries PortType::frequency).
    struct FreqSource : smolfm::Processor
    {
        smolfm::OutputPort out { smolfm::PortType::frequency, *this };

        FreqSource() : smolfm::Processor (smolfm::ProcessorRole::generic) {}

        void set (float v) noexcept                  { out.setSample ({ v, v }); }
        void prepare (double) override               {}
        void startNote() override                    {}
        smolfm::StereoFrame processSample() override { return out.getSample(); }
    };

    constexpr int kSr = 44100;
}

// The UI only sees published window bands (NeedleWindowTelemetry): every
// needle assert below renders one full window first, so the fold has run
// and the published pair is the band of exactly those samples.
template <typename ProcessorType>
void renderWindow (ProcessorType& p) { for (int i = 0; i < smolfm::needleWindowSamples; ++i) p.processSample(); }

// Assert helpers around the range protocol (no-data = NaN length).
void expectNoData (const juce::Range<float>& range, const char* message)
{
    assert (! smolfm::effectiveRangeHasData (range) && message);
}

void expectPoint (const juce::Range<float>& range, float value, const char* message)
{
    assert (smolfm::effectiveRangeHasData (range) && message);
    assert (range.getStart() == value && range.getEnd() == value && message);
}

// Delay times are integer-sample rounded back to ms: compare approximately.
void expectPointNear (const juce::Range<float>& range, float value, const char* message)
{
    assert (smolfm::effectiveRangeHasData (range) && message);
    assert (std::abs (range.getStart() - value) < 0.01f
         && std::abs (range.getEnd() - value) < 0.01f
         && message);
}

int main()
{
#ifdef NDEBUG
    // Release builds compile every assert() below away: such an EXE would
    // print "all passed" without checking anything.  Never let that run look
    // green - report failure and point to the Debug configuration instead.
    std::printf ("telemetry tests: NDEBUG build (asserts compiled out), run the Debug configuration\n");
    return 1;
#endif

    // 1) FM amount -----------------------------------------------------------
    {
        std::atomic<float> amount { 2.0f };
        smolfm::FMModulationProcessor fm (&amount);

        expectNoData (fm.getEffectiveAmountRange(),
                      "a processor that never rendered publishes no data, not a stale 0");
        renderWindow (fm);
        expectPoint (fm.getEffectiveAmountRange(), 2.0f,
                     "unmodulated FM must publish the static amount");

        TestSource mod;  mod.set (0.5f);
        fm.getAmountModInput().connect (mod.out);
        renderWindow (fm);
        expectPoint (fm.getEffectiveAmountRange(), 3.0f,
                     "amount_mod_in 0.5 must scale the amount to 1.5x");

        mod.set (-1.0f);
        renderWindow (fm);
        expectPoint (fm.getEffectiveAmountRange(), 0.0f,
                     "amount_mod_in -1 must clamp the amount at the bottom");

        TestSource vel (smolfm::PortType::velocity);  vel.set (0.5f);
        const bool fmVelWired = fm.getVelInput().connect (vel.out);
        assert (fmVelWired
                && "velocity input must accept a velocity-typed TestSource");
        mod.set (1.0f);   // amount * (1+1) = 4 — the needle ignores velocity
        renderWindow (fm);
        expectPoint (fm.getEffectiveAmountRange(), 4.0f,
                     "velocity never moves the needle: mod position amount * (1+m) = 4 even at vel 0.5 (the audio index is halved below)");
    }

    // 2) Waveshaper drive ---------------------------------------------------
    {
        std::atomic<float> drive { 0.25f };
        std::atomic<float> shape { 0.0f };
        smolfm::WaveshaperProcessor ws (&drive, &shape);

        renderWindow (ws);
        expectPoint (ws.getEffectiveDriveRange(), 0.25f,
                     "unmodulated waveshaper must publish the static drive");

        TestSource mod;  mod.set (1.0f);
        ws.getDriveModInput().connect (mod.out);
        renderWindow (ws);
        expectPoint (ws.getEffectiveDriveRange(), 0.5f,
                     "drive_mod_in 1.0 must double the drive (0.25 x 2)");

        TestSource vel (smolfm::PortType::velocity);  vel.set (0.5f);
        const bool wsVelWired = ws.getVelInput().connect (vel.out);
        assert (wsVelWired
                && "velocity input must accept a velocity-typed TestSource");
        renderWindow (ws);
        expectPoint (ws.getEffectiveDriveRange(), 0.5f,
                     "velocity never moves the needle: mod drive stays 0.5 at vel 0.5 (the audio drive is halved)");
    }

    // 3) Filter: cutoff replacement + resonance -----------------------------
    {
        std::atomic<float> cutoff    { 5000.0f };
        std::atomic<float> resonance { 0.4f };
        std::atomic<float> mode      { 0.0f };
        smolfm::FilterProcessor f (&cutoff, &resonance, &mode);

        renderWindow (f);
        expectPoint (f.getEffectiveCutoffRange(), 5000.0f,
                     "unwired cutoff_mod_in: effective cutoff = slider value");
        expectPoint (f.getEffectiveResonanceRange(), 0.4f,
                     "unwired resonance_mod_in: effective resonance = slider value");

        FreqSource cut;  cut.set (400.0f);
        f.getCutoffModInput().connect (cut.out);
        renderWindow (f);
        expectPoint (f.getEffectiveCutoffRange(), 400.0f,
                     "wired cutoff_mod_in replaces the static slider entirely");

        TestSource vel (smolfm::PortType::velocity);  vel.set (0.5f);
        const bool fVelWired = f.getVelInput().connect (vel.out);
        assert (fVelWired
                && "velocity input must accept a velocity-typed TestSource");
        renderWindow (f);
        expectPoint (f.getEffectiveCutoffRange(), 400.0f,
                     "velocity never moves the needle: cutoff stays at the mod position even at vel 0.5 (the audio cutoff is halved)");

        vel.set (1.0f);   // full velocity on both channels: mean 1 -> neutral
        renderWindow (f);
        expectPoint (f.getEffectiveCutoffRange(), 400.0f,
                     "full velocity is still neutral on the needle (vel mean of {1, 1} = 1)");

        TestSource mod;  mod.set (0.5f);
        f.getResonanceModInput().connect (mod.out);
        renderWindow (f);
        expectPoint (f.getEffectiveResonanceRange(), 0.6f,
                     "resonance_mod_in 0.5 scales the resonance by 1.5");
    }

    // 4) Delay: ms conversion + line clamp ---------------------------------
    {
        std::atomic<float> timeMs { 250.0f };
        std::atomic<float> fb     { 0.0f };
        std::atomic<float> mix    { 0.3f };
        std::atomic<float> sync   { 0.0f };
        std::atomic<float> div    { 1.0f };
        smolfm::DelayProcessor d (&timeMs, &fb, &mix, &sync, &div);
        d.prepare (kSr);

        renderWindow (d);
        expectPointNear (d.getEffectiveTimeMsRange(), 250.0f,
                         "unmodulated delay publishes its ms value (250 ms @ 44.1k)");

        TestSource mod;  mod.set (0.4f);
        d.getTimeModInput().connect (mod.out);
        renderWindow (d);
        expectPointNear (d.getEffectiveTimeMsRange(), 350.0f,
                         "time_mod_in 0.4 must scale the effective delay to 1.4x (350 ms)");

        // Sync mode publishes the same ms quantity (1/4 note @ 120 bpm).
        sync.store (1.0f);
        d.getTimeModInput().disconnect();
        d.setHostTempo (120.0);
        renderWindow (d);
        expectPointNear (d.getEffectiveTimeMsRange(), 500.0f,
                         "sync mode publishes the effective delay in ms too (1/4 @ 120 bpm)");
    }

    // 5) Resonator Q ---------------------------------------------------------
    {
        std::atomic<float> tune        { 100.0f };
        std::atomic<float> q           { 0.5f };
        std::atomic<float> decay       { 0.3f };
        std::atomic<float> spread      { 1.0f };
        std::atomic<float> strikeLevel { 0.0f };
        std::atomic<float> strikeTime  { 0.0f };
        smolfm::ResonatorProcessor r (&tune, &q, &decay, &spread, &strikeLevel, &strikeTime);
        r.prepare (kSr);

        renderWindow (r);
        expectPoint (r.getEffectiveQRange(), 0.5f,
                     "unwired q_mod_in: effective q = slider value");

        TestSource mod;  mod.set (-1.0f);
        r.getQModInput().connect (mod.out);
        renderWindow (r);
        expectPoint (r.getEffectiveQRange(), 0.0f,
                     "q_mod_in -1 clamps the effective q at the bottom");

        mod.set (0.5f);
        renderWindow (r);
        expectPoint (r.getEffectiveQRange(), 0.75f,
                     "q_mod_in 0.5 scales the effective q by 1.5");
    }

    // 6) Gain factor ---------------------------------------------------------
    // The gain node has a knob AND a factor_mod_in port — it qualifies for a
    // needle exactly like the others.  Its vel port stays sound-only: the
    // exclusion contract of FM/Waveshaper/Filter above applies unchanged.
    {
        std::atomic<float> gainFactor { 2.0f };
        smolfm::GainProcessor g (&gainFactor);

        renderWindow (g);
        expectPoint (g.getEffectiveGainRange(), 2.0f,
                     "unwired factor_mod_in: effective gain = slider value");

        TestSource mod;  mod.set (0.5f);
        g.getModInput().connect (mod.out);
        renderWindow (g);
        expectPoint (g.getEffectiveGainRange(), 3.0f,
                     "factor_mod_in 0.5 must scale the gain to 1.5x");

        TestSource vel (smolfm::PortType::velocity);  vel.set (0.5f);
        const bool gVelWired = g.getVelInput().connect (vel.out);
        assert (gVelWired
                && "velocity input must accept a velocity-typed TestSource");
        renderWindow (g);
        expectPoint (g.getEffectiveGainRange(), 3.0f,
                     "velocity never moves the needle: gain stays 3.0 even at vel 0.5 (the audio gain is halved)");

        // Window pins (the anti-aliasing upgrade): a mover publishes the
        // TRUE min/max of the window — whichever sample happened to win at
        // poll time no longer decides the dot.  Alternate the mod port
        // between 0 and 1 across one whole window: factor lands at 2.0/4.0
        // exactly, so the band edges are pinned float-exactly.
        TestSource sweep;
        g.getModInput().disconnect();
        g.getModInput().connect (sweep.out);
        float mover = 0.0f;
        for (int i = 0; i < smolfm::needleWindowSamples; ++i)
        {
            sweep.set (mover);
            g.processSample();
            mover = (mover == 0.0f) ? 1.0f : 0.0f;
        }
        const juce::Range<float> windowBand = g.getEffectiveGainRange();
        assert (smolfm::effectiveRangeHasData (windowBand)
                && "a moving modulator publishes a real band, not a point");
        assert (windowBand.getStart() == 2.0f && windowBand.getEnd() == 4.0f
                && "the window band carries the mover's true min (2.0) and max (4.0) — no aliased sample survived");

        // The published window replaces (not accumulates): a following
        // constant window collapses the band back to a point.
        sweep.set (0.0f);
        renderWindow (g);
        expectPoint (g.getEffectiveGainRange(), 2.0f,
                     "the window band replaces history: a constant mover collapses to a point again");
    }

    // 7) Range protocol: degenerate ranges are DATA --------------------------
    // The engine aggregator and the ModIndicatorOverlay must never use
    // juce::Range::isEmpty() as their no-data signal: it is true exactly
    // when start == end, so a real single-voice value (v, v) looks "empty"
    // and gets discarded — that bug had the aggregator drop EVERY voice,
    // and the ghost needle never drew at all (mono playback showed nothing).
    // The no-data signal is a NaN length instead: this repo's juce::Range
    // constructor clamps a reversed pair with jmax, so a negative length
    // can never exist — the old inverted-range sentinel silently turned
    // into the data range (1, 1).  See the protocol comment in
    // src/EffectiveRangeTelemetry.h and its uses in PluginProcessor.cpp
    // and ModIndicatorOverlay.cpp.
    {
        const juce::Range<float> single { 3.0f, 3.0f };
        assert (smolfm::effectiveRangeHasData (single)
                && "degenerate (v, v) is one real value, not no-data");
        assert (single.isEmpty()
                && "sanity: isEmpty() alone cannot separate (v, v) from no-data");
        assert (! smolfm::effectiveRangeHasData (smolfm::noEffectiveData())
                && "the NaN sentinel is no data, not a needle value");
        assert (std::isnan (smolfm::noEffectiveData().getLength())
                && "the no-data sentinel really carries a NaN length");
        // The old "any inverted (start > end) range means no data" assert is
        // gone on purpose: the clamping juce::Range constructor cannot build
        // such a range; the NaN sentinel is the only no-data value.
        assert (smolfm::effectiveRangeHasData (juce::Range<float> (0.0f, 0.0f))
                && "an effective value of exactly 0.0 is still data");
        const juce::Range<float> merged = single.getUnionWith (juce::Range<float> (2.0f, 5.0f));
        assert (merged.getStart() == 2.0f && merged.getEnd() == 5.0f
                && "voice min/max merging keeps a single-point value in the band");
    }

    std::printf ("telemetry tests: all passed\n");
    return 0;
}