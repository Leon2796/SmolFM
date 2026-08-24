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
      noteInput (PortType::frequency),
      output (PortType::signal, *this)
{
    // 0 Hz on the frequency input means "silence" — exactly what's needed for
    // a disconnected note input.  Anything else uses the slider parameter.
    noteInput.setDefaultValue (0.0f);
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
    float freq = noteInput.isConnected() ? noteInput.getSample()
                                         : frequency->load();

    // Hard guard against accidentally propagating a broken parameter value.
    // If the APVTS ever feeds a non-positive Hz in, the oscillator would
    // spin forever and produce DC.  Clamping here is cheaper than diagnosing.
    if (freq <= 0.0f)
        freq = 440.0f;

    oscillator.setFrequency (freq);
    oscillator.setWaveform (waveformFromIndex (static_cast<int> (std::round (waveform->load()))));

    float sample = oscillator.getNextSample (0.0f);
    output.setSample (sample);

    return sample;
}

} // namespace smolfm
