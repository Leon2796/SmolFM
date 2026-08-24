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

namespace
{
    // Resolve (nodeId, portId) to a real InputPort / OutputPort on this voice.
    // The ids here mirror GraphNodes.h; drift between the two will surface
    // as a failed lookup and the wire silently no-ops, which is acceptable
    // for a v1 — adding a new spec must update this function.

    smolfm::OutputPort* findOutputPort (const juce::String& nodeId,
                                        const juce::String& portId,
                                        smolfm::NoteProcessor& note,
                                        smolfm::OscillatorProcessor& carrier,
                                        smolfm::OscillatorProcessor& modulator,
                                        smolfm::FMModulationProcessor& fm,
                                        smolfm::AdsrProcessor& adsr)
    {
        if (portId != "out")
            return nullptr;

        if (nodeId == "note")      return &note.getOutput();
        if (nodeId == "carrier")   return &carrier.getOutput();
        if (nodeId == "modulator") return &modulator.getOutput();
        if (nodeId == "fm")        return &fm.getOutput();
        if (nodeId == "adsr")      return &adsr.getOutput();
        return nullptr;
    }

    smolfm::InputPort* findInputPort (const juce::String& nodeId,
                                      const juce::String& portId,
                                      smolfm::NoteProcessor& /*note*/,
                                      smolfm::OscillatorProcessor& carrier,
                                      smolfm::OscillatorProcessor& modulator,
                                      smolfm::FMModulationProcessor& fm,
                                      smolfm::AdsrProcessor& adsr)
    {
        if (portId == "note_in")
        {
            if (nodeId == "carrier")    return &carrier.getNoteInput();
            if (nodeId == "modulator")  return &modulator.getNoteInput();
            return nullptr;
        }
        if (portId == "carrier_in")
        {
            if (nodeId == "fm")         return &fm.getCarrierInput();
            return nullptr;
        }
        if (portId == "modulator_in")
        {
            if (nodeId == "fm")         return &fm.getModulatorInput();
            return nullptr;
        }
        if (portId == "in")
        {
            if (nodeId == "adsr")       return &adsr.getInput();
            return nullptr;
        }
        return nullptr;
    }
}

SynthVoice::SynthVoice (SynthVoiceParameters params)
    : parameters (params)
{
    buildGraph();
}

void SynthVoice::buildGraph()
{
    // Note source: converts the played MIDI note into Hertz that can drive
    // the carrier oscillator and/or the modulator.
    auto note = std::make_unique<NoteProcessor>();
    noteProcessor = note.get();

    // Carrier oscillator: same node type as the modulator, only different
    // parameter ids.  note_in (Hz from Note In, or its frequency slider when
    // unwired) drives the pitch; out carries the waveform.
    auto carrier = std::make_unique<OscillatorProcessor> (parameters.carrierFrequency,
                                                          parameters.carrierWaveform);
    carrierProcessor = carrier.get();

    // Modulator oscillator — identical structure, "modulator" is just a label.
    auto modulator = std::make_unique<OscillatorProcessor> (parameters.modulatorFrequency,
                                                            parameters.modulatorWaveform);
    modulatorProcessor = modulator.get();

    // FM stage: no oscillator of its own.  It takes the wired carrier and
    // modulator signals and emits sin(carrierPhase + modulator * fmAmount).
    auto fm = std::make_unique<FMModulationProcessor> (parameters.fmAmount);
    fmProcessor = fm.get();

    // ADSR envelope — shapes whichever signal reaches its input.
    auto adsr = std::make_unique<AdsrProcessor> (parameters.attack,
                                                  parameters.decay,
                                                  parameters.sustain,
                                                  parameters.release);
    adsrProcessor = adsr.get();

    // Processors are executed in the order they are added.  Note and both
    // oscillators run first so their outputs are fresh when FM and ADSR read
    // them.  The actual wiring is decided by applyConnectionPatch.
    graph.addProcessor (std::move (note));
    graph.addProcessor (std::move (carrier));
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

void SynthVoice::applyConnectionPatch (const ConnectionPatch& patch)
{
    if (noteProcessor == nullptr || carrierProcessor == nullptr
        || modulatorProcessor == nullptr || fmProcessor == nullptr
        || adsrProcessor == nullptr)
    {
        return;
    }

    // First: disconnect every wired input on this voice so that any port not
    // mentioned in the patch returns to its default (parameter slider).
    carrierProcessor->getNoteInput().disconnect();
    modulatorProcessor->getNoteInput().disconnect();
    fmProcessor->getCarrierInput().disconnect();
    fmProcessor->getModulatorInput().disconnect();
    adsrProcessor->getInput().disconnect();

    // The note node is the only "frequency source".  If nothing is wired from
    // note.out, it is switched off so silence stays silence and both
    // oscillators fall back to their frequency sliders.
    bool noteConnectedToSomething = false;

    for (const auto& conn : patch.connections)
    {
        smolfm::OutputPort* out = findOutputPort (conn.from.nodeId, conn.from.portId,
                                                  *noteProcessor, *carrierProcessor,
                                                  *modulatorProcessor, *fmProcessor,
                                                  *adsrProcessor);
        smolfm::InputPort*  in  = findInputPort  (conn.to.nodeId,   conn.to.portId,
                                                  *noteProcessor, *carrierProcessor,
                                                  *modulatorProcessor, *fmProcessor,
                                                  *adsrProcessor);

        if (out == nullptr || in == nullptr)
            continue;

        in->connect (*out);

        if (conn.from.nodeId == "note")
            noteConnectedToSomething = true;
    }

    noteProcessor->setEnabled (noteConnectedToSomething);
}

} // namespace smolfm
