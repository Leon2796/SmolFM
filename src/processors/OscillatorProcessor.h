/*
    OscillatorProcessor is a graph node that produces a waveform at a fixed
    frequency in Hertz.

    It can be used as either a carrier or a modulator source.  The waveform and
    frequency are read from atomic parameter pointers every sample so that UI
    changes are reflected live.
*/

#pragma once

#include "../SimpleOscillator.h"
#include "ProcessorPort.h"

namespace smolfm
{

/**
    An oscillator node with one signal output.

    The output port carries the raw oscillator sample for the current tick.
*/
class OscillatorProcessor final : public Processor
{
public:
    /**
        Create an oscillator node.

        @param waveformParameter  atomic pointer to the waveform index

        The frequency comes exclusively from the note_in port; without a
        connection the oscillator stays silent (0 Hz).
    */
    explicit OscillatorProcessor (std::atomic<float>* waveformParameter);

    void prepare (double newSampleRate) override;
    void startNote() override;
    float processSample() override;

    /**
        Access the output port so that other nodes can connect to it.
    */
    OutputPort& getOutput() noexcept
    {
        return output;
    }

    /**
        Optional frequency source driven by a MIDI note.

        When this input is connected, its Hertz value overrides the frequency
        parameter.  This lets a carrier oscillator follow a played MIDI key.
    */
    InputPort& getNoteInput() noexcept
    {
        return noteInput;
    }

private:
    SimpleOscillator oscillator;

    std::atomic<float>* waveform;

    InputPort noteInput;
    OutputPort output;
};

} // namespace smolfm
