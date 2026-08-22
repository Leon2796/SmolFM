/*
    SynthVoice.cpp contains the per-note audio rendering logic.

    The actual signal processing is now delegated to the processor graph in
    src/processors.  This file builds the graph per voice and orchestrates it.
*/

#include "SynthVoice.h"
#include "SynthSound.h"
#include "processors/NoteProcessor.h"
#include "processors/OscillatorProcessor.h"
#include "processors/FMModulationProcessor.h"
#include "processors/AdsrProcessor.h"

namespace smolfm
{

SynthVoice::SynthVoice (SynthVoiceParameters params)
    : parameters (params)
{
    buildGraph();
}

void SynthVoice::buildGraph()
{
    // Optional: convert the played MIDI note into a Hertz value that can drive
    // the carrier oscillator while leaving the modulator on its fixed Hertz
    // parameter.
    auto note = std::make_unique<NoteProcessor>();
    noteProcessor = note.get();

    // Modulator oscillator feeds the FM amount stage.  It always uses the fixed
    // Hertz parameter.
    auto modulator = std::make_unique<OscillatorProcessor> (parameters.modulatorFrequency,
                                                               parameters.modulatorWaveform);
    modulatorProcessor = modulator.get();

    // FM modulator owns the carrier oscillator and applies the phase offset.
    // The carrier gets its frequency from the MIDI note source, so the synth
    // follows the played key pitch.
    auto fm = std::make_unique<FMModulationProcessor> (parameters.carrierFrequency,
                                                          parameters.carrierWaveform,
                                                          parameters.fmAmount);
    fmProcessor = fm.get();

    // IMPORTANT: connect() must NOT live inside jassert().  In release builds
    // jassert expands to nothing, so the expression would be eliminated and the
    // ports would never be connected -- the ADSR would read 0.0f forever and
    // the plugin would stay silent.  All ports carry PortType::signal, so these
    // calls can only fail if the port types are changed in the future.
    const bool modulatorConnected = fmProcessor->getModulatorInput().connect (modulatorProcessor->getOutput());
    const bool carrierNoteConnected = fmProcessor->getCarrierNoteInput().connect (noteProcessor->getOutput());
    jassert (modulatorConnected && carrierNoteConnected);
    juce::ignoreUnused (modulatorConnected, carrierNoteConnected);

    // ADSR shapes the FM output.
    auto adsr = std::make_unique<AdsrProcessor> (parameters.attack,
                                                  parameters.decay,
                                                  parameters.sustain,
                                                  parameters.release);
    adsrProcessor = adsr.get();

    const bool adsrConnected = adsrProcessor->getInput().connect (fmProcessor->getOutput());
    jassert (adsrConnected);
    juce::ignoreUnused (adsrConnected);

    // Processors must be added in execution order:
    // note → carrier oscillator inside FM, modulator → FM → ADSR.
    // The note processor is processed first so its frequency value is ready.
    graph.addProcessor (std::move (note));
    graph.addProcessor (std::move (modulator));
    graph.addProcessor (std::move (fm));
    graph.addProcessor (std::move (adsr));
}

void SynthVoice::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    graph.prepare (sampleRate);
}

void SynthVoice::startNote (int midiNoteNumber,
                            float velocity,
                            juce::SynthesiserSound* /*sound*/,
                            int /*currentPitchWheelPosition*/)
{
    currentVelocity = velocity;

    // Feed the played MIDI note into the carrier frequency source.
    if (noteProcessor != nullptr)
        noteProcessor->setMidiNoteNumber (midiNoteNumber);

    graph.startNote();
}

void SynthVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        // Enter the release phase; the voice will keep rendering until the
        // envelope has finished.
        adsrProcessor->noteOff();
    }
    else
    {
        // Stop immediately and mark the voice as free for the next note.
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
    if (! adsrProcessor->isActive())
    {
        clearCurrentNote();
        return;
    }

    // Generate the requested number of samples by traversing the graph.
    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        float outputSample = graph.processSample() * currentVelocity;

        // Add the result to every output channel.  JUCE mixes voices by
        // summing their output buffers, so we must use += here.
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            auto* channelData = outputBuffer.getWritePointer (channel);
            channelData[startSample + sampleIndex] += outputSample;
        }
    }

    // If the envelope has just ended during this block, release the voice.
    if (! adsrProcessor->isActive())
        clearCurrentNote();
}

} // namespace smolfm
