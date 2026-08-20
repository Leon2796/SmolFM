/*
    SynthVoice.cpp contains the per-note audio rendering logic.

    This is the only place in the plugin where samples are actually generated,
    so it is written to be small, predictable and real-time safe.  There are no
    memory allocations, file accesses or locks inside the audio callback path.
*/

#include "SynthVoice.h"
#include "SynthSound.h"

namespace smolfm
{

SynthVoice::SynthVoice (SynthVoiceParameters params)
    : parameters (params)
{
}

void SynthVoice::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    // Tell every oscillator and the ADSR envelope about the sample rate.
    carrier.prepare (sampleRate);
    modulator.prepare (sampleRate);
    adsr.setSampleRate (sampleRate);
}

void SynthVoice::startNote (int midiNoteNumber,
                            float velocity,
                            juce::SynthesiserSound* /*sound*/,
                            int /*currentPitchWheelPosition*/)
{
    currentVelocity = velocity;

    // Convert the MIDI note number to a fundamental frequency in Hertz.
    double midiFrequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    // The UI controls frequency ratios, not absolute frequencies.  This makes
    // the synth automatically follow the musical pitch of the played note.
    float carrierFrequency  = static_cast<float> (midiFrequency * parameters.carrierRatio->load());
    float modulatorFrequency = static_cast<float> (midiFrequency * parameters.modulatorRatio->load());

    // Update oscillator frequencies and waveforms from the current parameters.
    carrier.setFrequency (carrierFrequency);
    modulator.setFrequency (modulatorFrequency);

    updateOscillatorWaveform (carrier,  parameters.carrierWaveform);
    updateOscillatorWaveform (modulator, parameters.modulatorWaveform);

    // Reset the oscillator phase so each note starts from a clean state.
    carrier.resetPhase();
    modulator.resetPhase();

    // Read the ADSR parameters now, at note-on time.  JUCE's ADSR documentation
    // warns that parameters should not be changed while the envelope is active;
    // applying new settings to the next note is the safe, simple strategy.
    juce::ADSR::Parameters adsrParameters;
    adsrParameters.attack  = parameters.attack->load();
    adsrParameters.decay   = parameters.decay->load();
    adsrParameters.sustain = parameters.sustain->load();
    adsrParameters.release = parameters.release->load();

    adsr.reset();
    adsr.setParameters (adsrParameters);
    adsr.noteOn();
}

void SynthVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        // Enter the release phase; the voice will keep rendering until the
        // envelope has finished.
        adsr.noteOff();
    }
    else
    {
        // Stop immediately and mark the voice as free for the next note.
        adsr.reset();
        clearCurrentNote();
    }
}

void SynthVoice::pitchWheelMoved (int /*newPitchWheelValue*/)
{
    // Pitch bend is not implemented in version 1.
}

void SynthVoice::controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/)
{
    // MIDI controllers other than note on/off are not used in version 1.
}

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    // Our only sound is a SynthSound.  Returning true for that type keeps the
    // voice/sound contract correct without adding unnecessary complexity.
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                  int startSample,
                                  int numSamples)
{
    // If the envelope has finished, release this voice so JUCE can reuse it.
    if (! adsr.isActive())
    {
        clearCurrentNote();
        return;
    }

    // Read the live FM amount on the audio thread.  Frequency ratio and FM
    // depth changes are allowed while a note is playing; ADSR changes only take
    // effect for the next note.
    const float fmAmount = parameters.fmAmount->load();

    // Optional: also allow live carrier/modulator ratio changes while playing.
    // We avoid reallocations and just update the oscillator frequency each block.
    carrier.setFrequency (static_cast<float> (
        juce::MidiMessage::getMidiNoteInHertz (getCurrentlyPlayingNote())
        * parameters.carrierRatio->load()));

    modulator.setFrequency (static_cast<float> (
        juce::MidiMessage::getMidiNoteInHertz (getCurrentlyPlayingNote())
        * parameters.modulatorRatio->load()));

    updateOscillatorWaveform (carrier,  parameters.carrierWaveform);
    updateOscillatorWaveform (modulator, parameters.modulatorWaveform);

    // Generate the requested number of samples.
    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        // 1. Generate the modulator sample.  It has its own independent phase.
        float modulatorSample = modulator.getNextSample (0.0f);

        // 2. Convert the modulator sample into a phase offset in radians.
        //    A modulator value of 0 produces no phase shift.  A larger FM amount
        //    pushes the carrier phase further, creating more sidebands.
        float phaseModulation = modulatorSample * fmAmount;

        // 3. Generate the carrier using the modulated phase.
        float carrierSample = carrier.getNextSample (phaseModulation);

        // 4. Apply the ADSR envelope and the note velocity.
        float envelope = adsr.getNextSample();
        float outputSample = carrierSample * envelope * currentVelocity;

        // 5. Add the result to every output channel.  JUCE mixes voices by
        //    summing their output buffers, so we must use += here.
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            auto* channelData = outputBuffer.getWritePointer (channel);
            channelData[startSample + sampleIndex] += outputSample;
        }
    }

    // If the envelope has just ended during this block, release the voice.
    if (! adsr.isActive())
        clearCurrentNote();
}

void SynthVoice::updateOscillatorWaveform (SimpleOscillator& oscillator,
                                           std::atomic<float>* waveformParameter)
{
    oscillator.setWaveform (waveformIndexToEnum (
        static_cast<int> (std::round (waveformParameter->load()))));
}

Waveform SynthVoice::waveformIndexToEnum (int index)
{
    switch (index)
    {
        case 0:  return Waveform::sine;
        case 1:  return Waveform::saw;
        case 2:  return Waveform::square;
        default: return Waveform::sine;
    }
}

} // namespace smolfm
