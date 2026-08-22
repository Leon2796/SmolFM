/*
    ProcessorPort.h defines the connection points between processors.

    An OutputPort belongs to a processor and provides one sample per tick.
    An InputPort points to exactly one OutputPort and reads its sample.

    Ports are typed so that invalid connections can be rejected at setup time.
*/

#pragma once

#include "Processor.h"
#include <juce_core/juce_core.h>

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

private:
    PortType type;
    Processor& owner;
    float currentSample = 0.0f;
};

/**
    A sink that reads from exactly one OutputPort.
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

        connection = &source;
        return true;
    }

    /**
        Read the current sample from the connected output.

        Returns 0.0f if nothing is connected.
    */
    float getSample() const noexcept
    {
        if (connection == nullptr)
            return 0.0f;

        return connection->getSample();
    }

    /**
        Return true if this input is connected to a source.
    */
    bool isConnected() const noexcept
    {
        return connection != nullptr;
    }

private:
    PortType type;
    OutputPort* connection = nullptr;
};

} // namespace smolfm
