/*
    OscillatorProcessor implementation.
*/

#include "OscillatorProcessor.h"

namespace smolfm
{

OscillatorProcessor::OscillatorProcessor (std::atomic<float>* waveformParameter,
                                          std::atomic<float>* modeParameter,
                                          std::atomic<float>* staticFreqParameter,
                                          std::atomic<float>* lfoRateParameter)
    : Processor (ProcessorRole::oscillator),
      waveform (waveformParameter),
      mode (modeParameter),
      staticFreq (staticFreqParameter),
      lfoRate (lfoRateParameter),
      noteInput (PortType::frequency),
      output (PortType::signal, *this)
{
    // Pitch mode default: an unconnected note_in means 0 Hz.  The oscillator
    // stays silent until a NoteProcessor (or a frequency chain ending in one)
    // is wired — or until Static/LFO mode is selected.
}

void OscillatorProcessor::prepare (double newSampleRate)
{
    oscillator.prepare (newSampleRate);
}

void OscillatorProcessor::startNote()
{
    // Static/LFO mode re-sync the phase on every note: the modulation starts
    // from the same point each time a key is played.  Pitch mode also resets
    // so carriers keep their per-note phase behaviour.
    oscillator.resetPhase();
}

float OscillatorProcessor::processSample()
{
    // Three frequency sources, chosen by the mode parameter:
    //   Pitch:  only a connected note_in supplies Hz (no hidden fallback).
    //   Static: the fixed audio-range frequency parameter; note_in ignored.
    //   LFO:    the fixed low-rate parameter; note_in ignored entirely.
    const int modeIndex = mode != nullptr
        ? juce::jlimit (0, 2, static_cast<int> (std::round (mode->load())))
        : 0;

    float freq = 0.0f;
    if (modeIndex == 1)
        freq = juce::jlimit (20.0f, 20000.0f, staticFreq != nullptr ? staticFreq->load() : 440.0f);
    else if (modeIndex == 2)
        freq = juce::jlimit (0.01f, 50.0f, lfoRate != nullptr ? lfoRate->load() : 1.0f);
    else if (noteInput.isConnected())
        freq = noteInput.getSample();

    oscillator.setFrequency (freq);
    oscillator.setWaveform (waveformFromIndex (static_cast<int> (std::round (waveform->load()))));

    float sample = oscillator.getNextSample (0.0f);
    output.setSample (sample);

    return sample;
}

} // namespace smolfm
