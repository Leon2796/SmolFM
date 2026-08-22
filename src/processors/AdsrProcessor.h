/*
    AdsrProcessor applies an ADSR envelope to a signal.

    It has one signal input and one signal output.  The output is the input
    sample multiplied by the current envelope value and the note velocity.
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
    float processSample() override;

    /**
        Tell the envelope to enter the release phase.
    */
    void noteOff();

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
