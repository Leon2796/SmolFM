/*
    NoteProcessor converts a MIDI note number into a frequency in Hertz.

    It has no input ports and one signal output.  The output value is the
    frequency for the current tick, computed with JUCE's MIDI note-to-Hertz
    conversion.
*/

#pragma once

#include "ProcessorPort.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace smolfm
{

/**
    A node that emits the frequency (in Hz) of a MIDI note.

    This is useful as a frequency source for carrier oscillators that should
    follow the musical pitch of a played key while modulators keep their fixed
    Hertz setting.
*/
class NoteProcessor final : public Processor
{
public:
    NoteProcessor();

    void prepare (double newSampleRate) override;
    void startNote() override;
    float processSample() override;

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

private:
    int currentNote = 60;
    float currentFrequency = 261.625565f;
    std::atomic<bool> enabled { true };
    OutputPort output;
};

} // namespace smolfm
