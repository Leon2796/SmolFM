/*
    Effective-value telemetry range protocol.

    A published effective (modulated) control value is a degenerate range
    `(v, v)` — a point, not a span.  That collides with juce::Range::isEmpty(),
    which is true exactly when start == end: a real single-voice value looks
    "empty", so emptiness must never serve as the no-data signal on this path
    (an earlier aggregator filtered with isEmpty() and discarded every voice —
    the ghost needle never drew at all).

    No-data is a NaN length instead: this repo's juce::Range constructor
    clamps a reversed pair with jmax (modules/JUCE/modules/juce_core/maths/
    juce_Range.h, line 60: end (jmax (startValue, endValue))), so a range
    with start > end — and with it a negative length — can never be
    constructed.  The old inverted-range sentinel { 1.0f, -1.0f } silently
    became the data range (1, 1), and idle/unwired call sites claimed a
    needle value of 1.0.  The sentinel is (NaN, NaN) now:

        real length (>= 0.0)  →  data: (v, v) or the real [min, max] voice band
        NaN length            →  no data (no active voice / instance missing)

    effectiveRangeHasData() is the one predicate for this; the voice getter
    (SynthVoice::getEffectiveControlRange), the engine aggregator
    (AudioPluginAudioProcessor::getEffectiveControlRange) and the UI overlay
    (ModIndicatorOverlay) all route through it so the protocol cannot drift
    apart again.

    Kept as a leaf header (nothing but juce_core's Range) so every side —
    including the console test target, which links no graphics modules — can
    include it cheaply.
*/

#pragma once

#include <juce_core/juce_core.h>

#include <atomic>
#include <cmath>
#include <limits>

namespace smolfm
{

inline juce::Range<float> noEffectiveData() noexcept
{
    return { std::numeric_limits<float>::quiet_NaN(),
             std::numeric_limits<float>::quiet_NaN() };   // NaN length: no data
}

inline bool effectiveRangeHasData (const juce::Range<float>& range) noexcept
{
    // NaN != NaN detects the no-data sentinel; the >= 0.0f part keeps the
    // protocol valid even against a juce::Range without the jmax clamp,
    // where a negative length could appear as a second no-data carrier.
    const float length = range.getLength();
    return ! std::isnan (length) && length >= 0.0f;
}

/**
    Window length of the ghost-needle telemetry, in samples.

    256 samples is ~5.3 ms at 48 kHz (~5.8 ms at 44.1 kHz): short enough
    that a slow mover's band is a small sliver riding the signal contour,
    long enough that an audio-rate mover completes many periods inside one
    window, so its band spans the true excursion.
*/
inline constexpr int needleWindowSamples = 256;

/**
    Per-control min/max telemetry window for the UI's ghost needle.

    The needle shows the effective (modulated) control value while a mod
    port is wired.  Publishing ONE overwritten value per sample (the first
    implementation) aliased against the overlay's 30 Hz poll: whichever
    sample survived the overwrite is a random phase of any modulating
    signal, and the needle jumped to arbitrary slider points — even for a
    pure sine.

    Instead the processor accumulates the min/max of its effective value
    over `needleWindowSamples` samples (sample()) and folds that band into
    the published atomics when the window completes.  The UI then receives
    a real [min, max] window band:

        slow mover (LFO)   -> a small sliver band riding the contour; the
                              overlay needle follows it and pendulates with
                              the modulation (a sine swings between its
                              true extremes at the true tempo),
        audio-rate mover   -> a band spanning the true excursion; the
                              needle stays bounded inside it instead of
                              flickering to random points,
        constant / unwired -> a degenerate (v, v) band: one resting point.

    Thread model: sample() runs on the audio thread, read() on the message
    thread.  The two stores are individual relaxed writes — a read between
    them can pair the new min with the old max for one frame; both values
    lie inside the true band and the UI unions bands anyway, so no
    handshake is needed.  Velocity deliberately never enters these values
    (sound-only contract, see the processors' telemetry comments).

    The member keeps the uiEffective<Control> name of the old per-sample
    atomic, so processor call sites stay name-stable.
*/
struct NeedleWindowTelemetry
{
    /** Accumulate one effective value on the audio thread. */
    void sample (float v) noexcept
    {
        if (v < accumMin) accumMin = v;
        if (v > accumMax) accumMax = v;

        if (++samplesInWindow >= needleWindowSamples)
        {
            windowMin.store (accumMin, std::memory_order_relaxed);
            windowMax.store (accumMax, std::memory_order_relaxed);
            hasData.store (true, std::memory_order_relaxed);
            samplesInWindow = 0;
            accumMin = std::numeric_limits<float>::max();
            accumMax = std::numeric_limits<float>::lowest();
        }
    }

    /** UI-side read: the completed window's band; noEffectiveData() while
        the very first window has not completed yet — a processor that has
        not rendered publishes no data, never a stale 0. */
    juce::Range<float> read() const noexcept
    {
        if (! hasData.load (std::memory_order_relaxed))
            return noEffectiveData();

        return juce::Range<float> (windowMin.load (std::memory_order_relaxed),
                                   windowMax.load (std::memory_order_relaxed));
    }

private:
    std::atomic<bool>  hasData   { false };
    std::atomic<float> windowMin { 0.0f };
    std::atomic<float> windowMax { 0.0f };

    // Window accumulators: audio thread only, plain members.
    float accumMin = std::numeric_limits<float>::max();
    float accumMax = std::numeric_limits<float>::lowest();
    int   samplesInWindow = 0;
};

} // namespace smolfm