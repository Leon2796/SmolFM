/*
    NoteProcessor implementation.
*/

#include "NoteProcessor.h"

namespace smolfm
{

NoteProcessor::NoteProcessor()
    : Processor (ProcessorRole::generic),
      output (PortType::signal, *this)
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

float NoteProcessor::processSample()
{
    output.setSample (currentFrequency);
    return currentFrequency;
}

void NoteProcessor::setMidiNoteNumber (int midiNoteNumber) noexcept
{
    currentNote = midiNoteNumber;
    currentFrequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (currentNote));
}

} // namespace smolfm
