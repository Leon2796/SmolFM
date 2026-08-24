/*
    ProcessorPort.h defines the connection points between processors.

    An OutputPort belongs to a processor and provides one sample per tick.
    An InputPort points (atomically) to exactly one OutputPort and reads its
    sample, or a user-supplied default if no source is connected.

    Ports are typed so that invalid connections can be rejected at setup time.
*/

#pragma once

#include "Processor.h"
#include <juce_core/juce_core.h>

#include <atomic>

namespace smolfm
{

/**
    A source of one sample per tick.

    Processors expose OutputPorts so that downstream nodes can read from them.
*/
class OutputPort
{
public:
    OutputPort (PortType portType, Processor& owningProcessor)
        : type (portType), owner (owningProcessor)
    {
    }

    /**
        Read the current sample from this port.

        The owning processor must have produced its output for this tick before
        this is called.
    */
    float getSample() const noexcept
    {
        return currentSample;
    }

    /**
        Update the cached sample for the current tick.

        Called by the owning processor after processSample().
    */
    void setSample (float sample) noexcept
    {
        currentSample = sample;
    }

    /**
        Return the type of signal this port carries.
    */
    PortType getType() const noexcept
    {
        return type;
    }

    /**
        Return the Processor that owns this port.
    */
    Processor& getOwner() const noexcept
    {
        return owner;
    }

private:
    PortType type;
    Processor& owner;
    float currentSample = 0.0f;
};

/**
    A sink that reads from exactly one OutputPort.

    The connection pointer is atomic so the UI thread can rewire the graph
    without stopping audio.  Audio-thread consumers only need the relaxed
    "current source + its latest sample" pair; with one writer (the message
    thread), release/acquire on the pointer is sufficient.
*/
class InputPort
{
public:
    explicit InputPort (PortType portType)
        : type (portType)
    {
    }

    /**
        Connect this input to an output.

        The function checks that both ports carry the same signal type.
    */
    bool connect (OutputPort& source)
    {
        if (source.getType() != type)
            return false;

        connection.store (&source, std::memory_order_release);
        return true;
    }

    /**
        Disconnect this input from its current output.

        Always succeeds and is safe to call from any thread.
    */
    void disconnect() noexcept
    {
        connection.store (nullptr, std::memory_order_release);
    }

    /**
        Default value used when no output is connected (or after disconnect).

        Signal inputs default to 0.0f, frequency inputs to 440.0f.
    */
    void setDefaultValue (float value) noexcept
    {
        defaultValue = value;
    }

    /**
        Read the current sample from the connected output.

        Returns the default value if nothing is connected.  Used by processor
        code inside processSample().
    */
    float getSample() const noexcept
    {
        OutputPort* src = connection.load (std::memory_order_acquire);
        return src != nullptr ? src->getSample() : defaultValue;
    }

    /**
        Return true if this input is connected to a source.
    */
    bool isConnected() const noexcept
    {
        return connection.load (std::memory_order_acquire) != nullptr;
    }

    /**
        Return the connected OutputPort, or nullptr.

        Used by the UI to render existing connections.
    */
    OutputPort* getConnectedSource() const noexcept
    {
        return connection.load (std::memory_order_acquire);
    }

    /**
        Return the type of signal this port expects.
    */
    PortType getType() const noexcept
    {
        return type;
    }

private:
    PortType type;
    std::atomic<OutputPort*> connection { nullptr };
    float defaultValue = 0.0f;
};

} // namespace smolfm
