/*
    Processor.h defines the base class for all signal-processing nodes in SmolFM.

    The graph is deliberately simple: every node produces one scalar sample per
    call to processSample().  Connections are explicit output->input ports, so
    the graph can later be traversed, validated and visualised without touching
    the audio math.
*/

#pragma once

#include <juce_core/juce_core.h>

namespace smolfm
{

/**
    Semantic type tags for ports.

    A port carries a single sample value.  Using an enum class prevents
    accidental connections between incompatible signal kinds.
*/
enum class PortType
{
    signal,

    /**
        Carries a frequency in Hertz rather than an amplitude sample.

        Frequencies are conceptually a different "kind" of value from audio
        samples.  They only flow from sources that know how to produce
        Hertz (NoteProcessor) into an oscillator's note_in port.
    */
    frequency
};

/**
    Every processor has a unique role so the graph knows what kind of node it is.
*/
enum class ProcessorRole
{
    oscillator,
    fmModulator,
    frequencyScale,
    adsr,
    masterOutput,
    generic
};

/**
    Base class for every node in the SmolFM processing graph.

    A Processor is owned by one voice.  It is prepared once, started for each
    note, and then called once per sample.  All memory must be pre-allocated;
    processSample() must be real-time safe.
*/
class Processor
{
public:
    explicit Processor (ProcessorRole roleInGraph)
        : role (roleInGraph)
    {
    }

    virtual ~Processor() = default;

    /**
        Configure the processor for a new sample rate.

        Called from SynthVoice::prepare().
    */
    virtual void prepare (double newSampleRate) = 0;

    /**
        Reset state for a new note.

        Called from SynthVoice::startNote().
    */
    virtual void startNote() = 0;

    /**
        Produce one output sample and advance internal state.

        The processor reads its connected inputs before producing output.
    */
    virtual float processSample() = 0;

    /**
        Return the role of this processor in the graph.
    */
    ProcessorRole getRole() const noexcept
    {
        return role;
    }

private:
    ProcessorRole role;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
};

} // namespace smolfm
