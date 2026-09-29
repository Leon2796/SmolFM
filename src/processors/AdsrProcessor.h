/*
    AdsrProcessor applies an ADSR envelope to a signal.

    It has one signal input and one signal output.  The output is the input
    frame multiplied per channel by the current envelope value (the envelope
    itself is one note-wide state, identical for L and R).
*/

#pragma once

#include "ProcessorPort.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace smolfm
{

/**
    ADSR envelope node.

    Parameters are read at note-on time, matching JUCE's ADSR recommendation.
    Live ADSR changes therefore take effect for the next note.
*/
class AdsrProcessor final : public Processor
{
public:
    /**
        Create an ADSR node.

        @param attackParam  atomic pointer to attack time in seconds
        @param decayParam   atomic pointer to decay time in seconds
        @param sustainParam atomic pointer to sustain level [0,1]
        @param releaseParam atomic pointer to release time in seconds
    */
    AdsrProcessor (std::atomic<float>* attackParam,
                   std::atomic<float>* decayParam,
                   std::atomic<float>* sustainParam,
                   std::atomic<float>* releaseParam);

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    /**
        Tell the envelope to enter the release phase.
    */
    void noteOff();

    /**
        Silences the envelope immediately (back to idle, value 0).

        applyConnectionPatch calls this so a new patch never inherits a stuck
        sustain: if a note-off was lost (or a held sustain pedal defers it
        indefinitely), the previous patch's ADSR would otherwise keep sounding
        at its sustain level through the next instrument.
    */
    void reset() noexcept;

    /**
        Return true if the envelope is still active.
    */
    bool isActive() const noexcept;

    InputPort& getInput() noexcept
    {
        return input;
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

    InputPort input;
    OutputPort output;
};

} // namespace smolfm
