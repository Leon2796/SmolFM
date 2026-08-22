/*
    SignalGraph.h defines a small directed graph of processors.

    The graph owns its processors and processes them in the order they were
    added.  This is sufficient for a simple FM chain.  A future version can add
    topological sorting if arbitrary connections are allowed.
*/

#pragma once

#include "Processor.h"

namespace smolfm
{

/**
    A container that owns processors and runs them in insertion order.

    The intended use is:
        1. Create processors.
        2. Connect their ports.
        3. Add them to the graph in dependency order (sources first).
        4. Call prepare(), startNote() and processSample() on the graph.
*/
class SignalGraph
{
public:
    SignalGraph() = default;

    /**
        Add a processor to the graph.

        The graph takes ownership.  Processors must be added in execution
        order: a processor must appear after every processor that feeds it.
    */
    void addProcessor (std::unique_ptr<Processor> processor)
    {
        Processor* raw = processor.get();
        ownedProcessors.push_back (std::move (processor));
        processors.push_back (raw);
    }

    /**
        Prepare every processor for a new sample rate.
    */
    void prepare (double newSampleRate)
    {
        for (Processor* proc : processors)
            proc->prepare (newSampleRate);
    }

    /**
        Start a new note on every processor.
    */
    void startNote()
    {
        for (Processor* proc : processors)
            proc->startNote();
    }

    /**
        Run one sample through the whole graph.

        Returns the output of the last processor.
    */
    float processSample()
    {
        float result = 0.0f;

        for (Processor* proc : processors)
            result = proc->processSample();

        return result;
    }

    /**
        Return the number of processors in the graph.
    */
    std::size_t size() const noexcept
    {
        return processors.size();
    }

private:
    std::vector<Processor*> processors;
    std::vector<std::unique_ptr<Processor>> ownedProcessors;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SignalGraph)
};

} // namespace smolfm
