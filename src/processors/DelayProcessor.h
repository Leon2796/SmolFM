/*
    DelayProcessor applies a digital delay with feedback and optional tempo
    synchronisation to the host BPM.

    Ports:
        - in           (signal): audio to delay.
        - time_mod_in  (signal): optional multiplier on the delay time.  The
          connected value's stereo mean is clamped to [-1, +1] and the
          effective delay becomes base_time * (1 + m) in both modes (free
          milliseconds and host-sync).  Unconnected: base time as-is.
          Time near zero clamps to the minimum line length (flanger style).
        - out (signal): delayed audio (wet + dry mix).

    Behaviour:
        - Millisecond mode: delay time set directly in ms (1 - 2000 ms).
        - Sync mode: delay time derived from host tempo and a note division
          (1/2, 1/4, 1/8, 1/16).  The effective delay in ms is
          `(60000 / bpm) * factor` where factor is 2.0, 1.0, 0.5, 0.25.
        - Feedback feeds a scaled amount of the delay line back into itself;
          values close to 1.0 run for a long tail without oscillating.

    Per-sample processing is real-time safe: one buffer write, one read (no
    interpolation — the nearest sample is used).  The buffer holds StereoFrames
    (2 channels), is pre-allocated to maxDelay at prepare() and never resized
    on the audio thread.
*/

#pragma once

#include "ProcessorPort.h"

#include "../EffectiveRangeTelemetry.h"      // NeedleWindowTelemetry

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <vector>

namespace smolfm
{

/**
    Tempo-synced or free-running delay line with feedback.

    The delay time can be set directly in milliseconds or derived from a
    host tempo (via setHostTempo) and a note division.  The APVTS parameters
    select which mode is active and the actual numeric value; the processor
    computes the delay in samples each sample from the current atomics.
*/
class DelayProcessor final : public Processor
{
public:
    DelayProcessor();
    DelayProcessor (std::atomic<float>* timeMs,
                    std::atomic<float>* feedbackParam,
                    std::atomic<float>* mixParam,
                    std::atomic<float>* syncParam,
                    std::atomic<float>* divisionParam);

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    /** Called from the processor's owner when the host playhead moves. */
    void setHostTempo (double bpm) noexcept { currentBpm = bpm; }

    /**
        True while the delay line still holds audible tail energy.

        Used by the voice lifetime policy: a voice may keep rendering after
        key release so its delay tail rings out.  The estimate is a peak
        follower updated while samples are written; it does not change the
        processor's audio behaviour.
    */
    bool hasEnergy() const noexcept { return tailPeak > 1.0e-3f; }

    /** Clear the delay line and tail estimate (used when a new patch is loaded). */
    void reset() noexcept;

    InputPort& getInput() noexcept  { return input; }
    InputPort& getTimeModInput() noexcept { return timeMod; }
    OutputPort& getOutput() noexcept { return output; }

    /** Effective delay-time band currently in use (ms), for the UI's
        ghost needle: min/max over the telemetry window.  Covers both
        modes: free-running and host-sync.  Display-only telemetry,
        thread-safe. */
    juce::Range<float> getEffectiveTimeMsRange() const noexcept
    {
        return uiEffectiveTimeMs.read();
    }

private:
    /**
        Recompute the current delay length in samples from the parameter
        state and the host BPM (when sync is active).  Called on the audio
        thread; only touches atomics and arithmetic, no allocation.
    */
    float getDelaySamples() const noexcept;

    InputPort input  { PortType::signal };
    InputPort timeMod { PortType::signal };
    OutputPort output { PortType::signal, *this };

    std::atomic<float>* timeMs = nullptr;     // free-running delay in ms
    std::atomic<float>* feedback = nullptr;   // 0..0.95
    std::atomic<float>* mix = nullptr;        // 0..1 dry/wet
    std::atomic<float>* syncMode = nullptr;   // 0=free, 1=sync to host
    std::atomic<float>* division = nullptr;   // 0=1/2, 1=1/4, 2=1/8, 3=1/16

    // Telemetry for the UI's mod indicator (ghost needle): min/max of the
    // effective delay time in ms (time_mod_in scaling + buffer clamp) over
    // the needle window — a per-sample overwrite would alias against the
    // overlay's 30 Hz poll (see NeedleWindowTelemetry).
    NeedleWindowTelemetry uiEffectiveTimeMs;

    double sampleRate = 44100.0;
    double currentBpm = 120.0;

    static constexpr double maxDelaySeconds = 2.0;
    std::vector<StereoFrame> buffer;   // two channels, pre-allocated
    int writePos = 0;

    // O(1) tail-energy estimate: a peak follower over the samples written
    // into the buffer.  Decays slowly so it never under-reports a ringing
    // tail by much; zero when the line is cleared.
    float tailPeak = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayProcessor)
};

} // namespace smolfm
