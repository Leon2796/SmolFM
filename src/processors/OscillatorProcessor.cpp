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

StereoFrame OscillatorProcessor::processSample()
{
    // Bypass: a source has no signal input to pass through, so "bypass" means
    // the oscillator switches off: the output port writes an exact zero frame
    // (downstream must not read a stale frame) while the phase is not
    // advanced — parked, never reset.  Un-bypassing resumes the parked phase,
    // so the operator picks up exactly where the bypass caught it.
    if (isBypassed())
    {
        const StereoFrame silence = StereoFrame::centered (0.0f);
        output.setSample (silence);
        return silence;
    }

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
    const bool noteActive = noteInput.isConnected() && noteInput.getSample().l > 0.0f;

    if (modeIndex == 1 && noteActive)
        freq = juce::jlimit (20.0f, 20000.0f, staticFreq != nullptr ? staticFreq->load() : 440.0f);
    else if (modeIndex == 2 && noteActive)
        freq = juce::jlimit (0.01f, 50.0f, lfoRate != nullptr ? lfoRate->load() : 1.0f);
    else if (modeIndex == 0 && noteInput.isConnected())
        freq = noteInput.getSample().l;

    oscillator.setFrequency (freq);
    // Waveform null-guard mirrors the mode/staticFreq/lfoRate guards above:
    // a null waveform pointer means "no parameter wired" and falls back to
    // the sine default (index 0) instead of crashing the first sample.
    oscillator.setWaveform (waveformFromIndex (static_cast<int> (std::round (waveform != nullptr ? waveform->load() : 0.0f))));

    // Mono phase: the oscillator produces one sample per tick; it is emitted
    // as a centered pair so both output channels carry identical content.
    const float sample = oscillator.getNextSample (0.0f);
    const StereoFrame result = StereoFrame::centered (sample);
    output.setSample (result);

    return result;
}

} // namespace smolfm
