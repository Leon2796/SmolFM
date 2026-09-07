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
    //   Pitch:  the note_in port supplies Hz directly (no hidden fallback).
    //   Static: note_in acts as a GATE only — the oscillator fires while a
    //           note is connected and playing, but always swings at the fixed
    //           staticFreq parameter instead of the port value.
    //   LFO:    same gate behaviour at the fixed low rate.
    const int modeIndex = mode != nullptr
        ? juce::jlimit (0, 2, static_cast<int> (std::round (mode->load())))
        : 0;

    float freq = 0.0f;
    const bool noteActive = noteInput.isConnected() && noteInput.getSample() > 0.0f;

    if (modeIndex == 1 && noteActive)
        freq = juce::jlimit (20.0f, 20000.0f, staticFreq != nullptr ? staticFreq->load() : 440.0f);
    else if (modeIndex == 2 && noteActive)
        freq = juce::jlimit (0.01f, 50.0f, lfoRate != nullptr ? lfoRate->load() : 1.0f);
    else if (modeIndex == 0 && noteInput.isConnected())
        freq = noteInput.getSample();

    oscillator.setFrequency (freq);
    oscillator.setWaveform (waveformFromIndex (static_cast<int> (std::round (waveform->load()))));

    float sample = oscillator.getNextSample (0.0f);
    output.setSample (sample);

    return sample;
}

} // namespace smolfm
