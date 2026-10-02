/*
    NoteProcessor converts a MIDI note number into a frequency in Hertz.

    It has no input ports and two outputs: the note frequency (PortType::
    frequency, "out") and the note velocity (PortType::velocity, "vel"), the
    per-note attack strength of the currently played key.
*/

#pragma once

#include "ProcessorPort.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace smolfm
{

/**
    A node that emits the frequency (in Hz) and the velocity (0-1) of the
    currently played MIDI note.

    This is useful as a frequency source for carrier oscillators that should
    follow the musical pitch of a played key while modulators keep their fixed
    Hertz setting; the velocity output feeds vel inputs (accent wiring).
*/
class NoteProcessor final : public Processor
{
public:
    NoteProcessor();

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    /**
        Set the current MIDI note number.

        Called from SynthVoice::startNote().  Frequencies follow the equal
        tempered scale.
    */
    void setMidiNoteNumber (int midiNoteNumber) noexcept;

    /**
        Disable the note source (e.g. when the UI disconnected the port).

        While disabled, processSample() emits 0 Hz.  Re-enabled by the next
        setMidiNoteNumber() or by an explicit setEnabled(true).
    */
    void setEnabled (bool shouldBeEnabled) noexcept
    {
        enabled.store (shouldBeEnabled, std::memory_order_release);
    }

    bool isEnabled() const noexcept
    {
        return enabled.load (std::memory_order_acquire);
    }

    OutputPort& getOutput() noexcept
    {
        return output;
    }

    /**
        Set the velocity (0-1) of the current note.

        Called from SynthVoice::startNote() alongside setMidiNoteNumber();
        the value is emitted on the vel output until the next note.
    */
    void setVelocity (float velocity) noexcept
    {
        currentVelocity.store (velocity, std::memory_order_release);
    }

    /**
        Velocity output (PortType::velocity): the attack strength (0-1) of
        the current note as a centered frame.  Wire it into a processor's
        vel input to scale a parameter per note (accent gain, drive,
        FM index, cutoff).
    */
    OutputPort& getVelOutput() noexcept
    {
        return velOutput;
    }

private:
    int currentNote = 60;
    float currentFrequency = 261.625565f;
    // Neutral default (1.0) so an early consumer read scales nothing.
    std::atomic<float> currentVelocity { 1.0f };
    std::atomic<bool> enabled { true };
    OutputPort output;
    OutputPort velOutput { PortType::velocity, *this };
};

} // namespace smolfm
