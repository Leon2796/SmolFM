/*
    OscillatorProcessor implementation.
*/

#include "OscillatorProcessor.h"

namespace smolfm
{

OscillatorProcessor::OscillatorProcessor (std::atomic<float>* waveformParameter)
    : Processor (ProcessorRole::oscillator),
      waveform (waveformParameter),
      noteInput (PortType::frequency),
      output (PortType::signal, *this)
{
    // No default: an unconnected note_in means 0 Hz.  The oscillator stays
    // silent until a NoteProcessor (or a frequency chain ending in one) is
    // wired — MIDI can only reach the carrier through an explicit trace.
}

void OscillatorProcessor::prepare (double newSampleRate)
{
    oscillator.prepare (newSampleRate);
}

void OscillatorProcessor::startNote()
{
    oscillator.resetPhase();
}

float OscillatorProcessor::processSample()
{
    // Only a connected note_in supplies a frequency.  Without that trace the
    // oscillator produces 0 Hz — no hidden slider fallback, no phantom notes.
    const float freq = noteInput.isConnected() ? noteInput.getSample()
                                               : 0.0f;

    oscillator.setFrequency (freq);
    oscillator.setWaveform (waveformFromIndex (static_cast<int> (std::round (waveform->load()))));

    float sample = oscillator.getNextSample (0.0f);
    output.setSample (sample);

    return sample;
}

} // namespace smolfm
