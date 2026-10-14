/*
    SampleHoldProcessor freezes the input signal on a rate grid (sample
    and hold / sample-rate reduction).  It is the time axis of a bitcrusher:
    the output carries the same frozen L/R pair for one hold period and then
    latches a fresh copy of the input.

    Ports:
        - in            (signal): audio to freeze.
        - rate_mod_in   (signal): optional multiplier on the hold rate.  The
          connected value's stereo mean is clamped to [-1, +1] and the
          effective rate becomes rate * (1 + m) in both modes (free Hz and
          host-sync).  Unconnected: base rate as-is.
        - out (signal): the held audio (a staircase in time).

    Behaviour:
        - Free mode: the hold period is `sampleRate / rateHz` samples
          (rate 0.5 .. 20000 Hz).
        - Sync mode: one hold per note division (1/2, 1/4, 1/8, 1/16) of
          the host tempo — the grid snaps to the beat instead of a clock.
        - startNote() re-anchors the grid: the first sample of every note
          latches immediately, so the staircase always starts on the note.
        - The period is an integer sample count and the held value is a
          StereoFrame, so left and right freeze independently.  No buffer,
          no feedback, no tail: a hold node never rings out, so the voice
          lifetime policy does not need it.
        - Per-sample processing is real-time safe: one StereoFrame of held
          state, one int counter, atomics only.

    The sync schema (free/sync + division) mirrors DelayProcessor's; the
    class only holds instead of delaying.
*/

#pragma once

#include "ProcessorPort.h"

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>

namespace smolfm
{

/**
    Rate-grid sample-and-hold with the delay node's sync schema.

    The hold rate can be set directly in Hertz or derived from a host tempo
    (via setHostTempo) and a note division.  The APVTS parameters select the
    mode and the numeric value; the processor computes the hold period in
    samples each sample from the current atomics.
*/
class SampleHoldProcessor final : public Processor
{
public:
    SampleHoldProcessor();
    SampleHoldProcessor (std::atomic<float>* rateHzParam,
                         std::atomic<float>* syncParam,
                         std::atomic<float>* divisionParam);

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    /** Called from the processor's owner when the host playhead moves. */
    void setHostTempo (double bpm) noexcept { currentBpm = bpm; }

    InputPort& getInput() noexcept        { return input; }
    InputPort& getRateModInput() noexcept { return rateMod; }
    OutputPort& getOutput() noexcept      { return output; }

private:
    /**
        Current hold rate in Hz from the parameter state and the host BPM
        (when sync is active).  Called on the audio thread; only touches
        atomics and arithmetic, no allocation.
    */
    float getHoldRate() const noexcept;

    InputPort input   { PortType::signal };
    InputPort rateMod { PortType::signal };
    OutputPort output { PortType::signal, *this };

    std::atomic<float>* rateHz   = nullptr;   // hold rate in Hz (free mode)
    std::atomic<float>* syncMode = nullptr;   // 0=free, 1=sync to host
    std::atomic<float>* division = nullptr;   // 0=1/2, 1=1/4, 2=1/8, 3=1/16

    double sampleRate = 44100.0;
    double currentBpm = 120.0;

    StereoFrame held { 0.0f, 0.0f };   // frozen L/R pair
    int samplesUntilHold = 1;          // latches on the first sample after start

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleHoldProcessor)
};

} // namespace smolfm