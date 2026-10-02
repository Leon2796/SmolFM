/*
    NoteProcessor implementation.
*/

#include "NoteProcessor.h"

namespace smolfm
{

NoteProcessor::NoteProcessor()
    : Processor (ProcessorRole::generic),
      output (PortType::frequency, *this)
{
}

void NoteProcessor::prepare (double /*newSampleRate*/)
{
    // Nothing to prepare; conversion from note to Hertz is sample-rate
    // independent.  The port keeps its current value until the next note.
}

void NoteProcessor::startNote()
{
    // Frequency is set via setMidiNoteNumber(), which is called before startNote().
}

StereoFrame NoteProcessor::processSample()
{
    const StereoFrame f = StereoFrame::centered (isEnabled() ? currentFrequency : 0.0f);
    output.setSample (f);

    const StereoFrame v = StereoFrame::centered (isEnabled()
        ? currentVelocity.load (std::memory_order_acquire) : 0.0f);
    velOutput.setSample (v);

    return f;
}

void NoteProcessor::setMidiNoteNumber (int midiNoteNumber) noexcept
{
    currentNote = midiNoteNumber;
    currentFrequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (currentNote));
    enabled.store (true, std::memory_order_release);
}

} // namespace smolfm
