/*
    OscillatorProcessor is a graph node that produces a waveform at a fixed
    frequency in Hertz.

    It can be used as either a carrier or a modulator source.  The waveform and
    frequency are read from atomic parameter pointers every sample so that UI
    changes are reflected live.

    LFO mode: when the mode parameter is active, the oscillator runs at a fixed
    rate (the rate parameter, 0.01-50 Hz) instead of following the note_in
    port.  Note-on re-syncs the phase so every note starts its modulation from
    the same point; the rate itself stays constant.  This turns any oscillator
    into a note-triggered modulation source for the graph.
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
        @param lfoModeParameter   atomic pointer to the LFO-mode flag (0/1)
        @param lfoRateParameter   atomic pointer to the LFO rate in Hz (0.01-50)

        The frequency comes exclusively from the note_in port in pitch mode;
        without a connection the oscillator stays silent (0 Hz).  In LFO mode
        the rate parameter drives the frequency instead.
    */
    OscillatorProcessor (std::atomic<float>* waveformParameter,
                         std::atomic<float>* lfoModeParameter,
                         std::atomic<float>* lfoRateParameter);

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
    std::atomic<float>* lfoMode;
    std::atomic<float>* lfoRate;

    InputPort noteInput;
    OutputPort output;
};

} // namespace smolfm
