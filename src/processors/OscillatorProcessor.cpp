/*
    OscillatorProcessor implementation.
*/

#include "OscillatorProcessor.h"

namespace smolfm
{

OscillatorProcessor::OscillatorProcessor (std::atomic<float>* waveformParameter,
                                          std::atomic<float>* lfoModeParameter,
                                          std::atomic<float>* lfoRateParameter)
    : Processor (ProcessorRole::oscillator),
      waveform (waveformParameter),
      lfoMode (lfoModeParameter),
      lfoRate (lfoRateParameter),
      noteInput (PortType::frequency),
      output (PortType::signal, *this)
{
    // No default: an unconnected note_in means 0 Hz in pitch mode.  The
    // oscillator stays silent until a NoteProcessor (or a frequency chain
    // ending in one) is wired — or until LFO mode is switched on.
}

void OscillatorProcessor::prepare (double newSampleRate)
{
    oscillator.prepare (newSampleRate);
}

void OscillatorProcessor::startNote()
{
    // LFO mode re-syncs the phase on every note: the modulation starts from
    // the same point each time a key is played.  Pitch mode also resets so
    // carriers keep their per-note phase behaviour.
    oscillator.resetPhase();
}

float OscillatorProcessor::processSample()
{
    // Two frequency sources, chosen by the mode parameter:
    //   pitch mode: only a connected note_in supplies Hz (no hidden fallback).
    //   LFO mode:   the fixed rate parameter; note_in is ignored entirely.
    const float freq = lfoMode != nullptr && lfoMode->load() >= 0.5f
        ? juce::jlimit (0.01f, 50.0f, lfoRate != nullptr ? lfoRate->load() : 1.0f)
        : (noteInput.isConnected() ? noteInput.getSample() : 0.0f);

    oscillator.setFrequency (freq);
    oscillator.setWaveform (waveformFromIndex (static_cast<int> (std::round (waveform->load()))));

    float sample = oscillator.getNextSample (0.0f);
    output.setSample (sample);

    return sample;
}

} // namespace smolfm
