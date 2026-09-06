/*
    FAdsrProcessor applies an ADSR envelope to a frequency in the frequency
    domain (F-ADSR: frequency ADSR pitch envelope).

    Ports:
        - freq_in (frequency): incoming frequency in Hertz, wired from
                               note.out, an FM stage or another scaler.
        - out     (frequency): the pitch-enveloped frequency in Hertz.

    Per sample:

        f_out = f_in * f(E)

    where E is the ADSR envelope value (0..1) and the scaling factor is
    interpolated between upFactor (at E = 1) and downFactor (at E = 0):

        f(E) = downFactor + (upFactor - downFactor) * E

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
    float processSample() override;

    /**
        Tell the envelope to enter the release phase.
    */
    void noteOff();

    /**
        Return true if the envelope is still active.
    */
    bool isActive() const noexcept;

    InputPort& getFreqInput() noexcept
    {
        return freqInput;
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
    OutputPort output { PortType::frequency, *this };
};

} // namespace smolfm