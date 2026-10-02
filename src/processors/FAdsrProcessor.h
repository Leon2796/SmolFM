/*
    FAdsrProcessor applies an ADSR envelope to a frequency in the frequency
    domain (F-ADSR: frequency ADSR pitch envelope).

    Ports:
        - freq_in (frequency): incoming frequency in Hertz, wired from
                               note.out, an FM stage or another scaler.
        - vel     (velocity):  optional per-note velocity input (0-1).
                               Connected: it scales the ANCHOR DEPTH — each
                               anchor slides from neutral 1.0 toward its
                               configured value (pull-to-neutral, below).
                               Unconnected: fully transparent.
        - out     (frequency): the pitch-enveloped frequency in Hertz.

    Per sample:

        f_out = f_in * f_v(E)

    where E is the ADSR envelope value (0..1) and the scaling factor is
    interpolated between the effective up and down factors:

        f(E) = downEff + (upEff - downEff) * E

    Velocity pull-to-neutral (accent depth): velocity does not stretch the
    anchors arbitrarily — each anchor depth slides on its own rail from the
    neutral factor 1.0 to the configured value:

        upEff   = 1 + v * (upValue   - 1)
        downEff = 1 + v * (downValue - 1)

    So v = 0 flattens the envelope to factor 1 (byte-identical to the
    unwired behaviour), v = 1 sits exactly on the configured anchors, and v
    linearly scales how far both anchors deviate from neutral — the anchor
    gap shrinks proportionally (gap = v * (upValue - downValue)), while the
    envelope's shape (A/D/S/R times) and the anchor directions stay exactly
    as configured.  Example: up = 3, down = 0.5 at half velocity swings only
    up 2.0 / down 0.75 — a soft "hint" of the same pitch gesture.

    Neutral state: upFactor = downFactor = 1.0 makes the node transparent —
    the envelope value has no influence on the pitch.

    Wraps juce::ADSR; parameters are read at note-on time (JUCE
    recommendation), matching AdsrProcessor.  The envelope is also stopped
    when the voice is released (noteOff), so the release segment controls
    the tail pitch movement.
*/

#pragma once

#include "ProcessorPort.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace smolfm
{

/**
    Pitch-envelope node operating in the frequency domain.

    The six parameters shape both the envelope timing (A/D/S/R) and how the
    frequency is scaled at the envelope extremes (up/down factor).  Because
    it sits in the frequency domain, it can drive an oscillator's note_in
    port directly, or feed an FM stage's freq_in.
*/
class FAdsrProcessor final : public Processor
{
public:
    /**
        Create the F-ADSR pitch-envelope node.

        @param attackParam     atomic pointer to attack time in seconds
        @param decayParam      atomic pointer to decay time in seconds
        @param sustainParam    atomic pointer to sustain level [0,1]
        @param releaseParam    atomic pointer to release time in seconds
        @param upParam         atomic pointer to factor applied at E = 1
        @param downParam       atomic pointer to factor applied at E = 0
    */
    FAdsrProcessor (std::atomic<float>* attackParam,
                    std::atomic<float>* decayParam,
                    std::atomic<float>* sustainParam,
                    std::atomic<float>* releaseParam,
                    std::atomic<float>* upParam,
                    std::atomic<float>* downParam);

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    /**
        Tell the envelope to enter the release phase.
    */
    void noteOff();

    /**
        Silences the envelope immediately (back to idle, value 0).

        Same purpose as AdsrProcessor::reset(): a new patch must not inherit
        a stuck sustain from the previous one.
    */
    void reset() noexcept;

    /**
        Return true if the envelope is still active.
    */
    bool isActive() const noexcept;

    InputPort& getFreqInput() noexcept
    {
        return freqInput;
    }

    InputPort& getVelInput() noexcept
    {
        return velIn;
    }

    OutputPort& getOutput() noexcept
    {
        return output;
    }

private:
    juce::ADSR envelope;
    double sampleRate = 44100.0;

    std::atomic<float>* attack;
    std::atomic<float>* decay;
    std::atomic<float>* sustain;
    std::atomic<float>* release;
    std::atomic<float>* up;
    std::atomic<float>* down;

    InputPort freqInput { PortType::frequency };
    InputPort velIn     { PortType::velocity };
    OutputPort output   { PortType::frequency, *this };
};

} // namespace smolfm