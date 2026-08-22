/*
    OscillatorProcessor implementation.
*/

#include "OscillatorProcessor.h"

namespace smolfm
{

OscillatorProcessor::OscillatorProcessor (std::atomic<float>* frequencyParameter,
                                          std::atomic<float>* waveformParameter)
    : Processor (ProcessorRole::oscillator),
      frequency (frequencyParameter),
      waveform (waveformParameter),
      noteInput (PortType::signal),
      output (PortType::signal, *this)
{
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
    float freq = frequency->load();

    // If a note source is connected (e.g. MIDI note), use its frequency.
    if (noteInput.isConnected())
        freq = noteInput.getSample();

    if (freq <= 0.0f)
        freq = 440.0f;

    oscillator.setFrequency (freq);
    oscillator.setWaveform (waveformFromIndex (static_cast<int> (std::round (waveform->load()))));

    float sample = oscillator.getNextSample (0.0f);
    output.setSample (sample);

    return sample;
}

} // namespace smolfm
