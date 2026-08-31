/*
    SynthVoice.cpp contains the per-note audio rendering logic.

    The voice holds pools of processors; which of them actually run is decided
    by the ConnectionPatch applied from the UI.
*/

#include "SynthVoice.h"
#include "SynthSound.h"

namespace smolfm
{

namespace
{
    // Resolve a (nodeId, portId) pair to the matching port in this voice.
    // Node ids carry the instance index ("osc3", "fm1"), so this is a pure
    // lookup — no allocations, safe for the message thread while audio runs.

    smolfm::OutputPort* findOutputPort (const juce::String& nodeId,
                                        const juce::String& portId,
                                        smolfm::SynthVoice& voice);
                                        // implemented below via the pools

    smolfm::InputPort* resolveInput (const juce::String& nodeId,
                                     const juce::String& portId,
                                     SynthVoiceParameters& params,
                                     NoteProcessor* note,
                                     AdsrProcessor* adsr,
                                     std::array<OscillatorProcessor*, GraphNodeRegistry::maxOscillators>& oscillators,
                                     std::array<FMModulationProcessor*, GraphNodeRegistry::maxFmAmounts>& fmProcessors)
    {
        juce::ignoreUnused (params, note);

        const NodeType type = GraphNodeRegistry::typeOf (nodeId);
        const int index = GraphNodeRegistry::indexOf (nodeId);

        if (type == NodeType::oscillator
         && index >= 0 && index < GraphNodeRegistry::maxOscillators
         && oscillators[static_cast<size_t> (index)] != nullptr)
        {
            if (portId == "note_in")
                return &oscillators[static_cast<size_t> (index)]->getNoteInput();

            return nullptr;
        }

        if (type == NodeType::fmAmount
         && index >= 0 && index < GraphNodeRegistry::maxFmAmounts
         && fmProcessors[static_cast<size_t> (index)] != nullptr)
        {
            if (portId == "freq_in")
                return &fmProcessors[static_cast<size_t> (index)]->getFreqInput();

            if (portId == "modulator_in")
                return &fmProcessors[static_cast<size_t> (index)]->getModulatorInput();

            return nullptr;
        }

        if (type == NodeType::adsr && adsr != nullptr && portId == "in")
            return &adsr->getInput();

        return nullptr;
    }

    smolfm::OutputPort* resolveOutput (const juce::String& nodeId,
                                       const juce::String& portId,
                                       NoteProcessor* note,
                                       AdsrProcessor* adsr,
                                       std::array<OscillatorProcessor*, GraphNodeRegistry::maxOscillators>& oscillators,
                                       std::array<FMModulationProcessor*, GraphNodeRegistry::maxFmAmounts>& fmProcessors)
    {
        if (portId != "out")
            return nullptr;

        const NodeType type = GraphNodeRegistry::typeOf (nodeId);
        const int index = GraphNodeRegistry::indexOf (nodeId);

        if (type == NodeType::note && note != nullptr)
            return &note->getOutput();

        if (type == NodeType::oscillator
         && index >= 0 && index < GraphNodeRegistry::maxOscillators
         && oscillators[static_cast<size_t> (index)] != nullptr)
            return &oscillators[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::fmAmount
         && index >= 0 && index < GraphNodeRegistry::maxFmAmounts
         && fmProcessors[static_cast<size_t> (index)] != nullptr)
            return &fmProcessors[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::adsr && adsr != nullptr)
            return &adsr->getOutput();

        return nullptr;
    }
}

//==============================================================================
SynthVoice::SynthVoice (SynthVoiceParameters params)
    : parameters (params)
{
    buildGraph();
}

void SynthVoice::buildGraph()
{
    // Note source (singleton).
    auto note = std::make_unique<NoteProcessor>();
    noteProcessor = note.get();
    graph.addProcessor (std::move (note));

    // Oscillator pool — all created up front, all fed by their own parameter
    // pair.  An osc box that is never wired just renders (harmlessly, it's
    // cheap) and its output is never read by anyone.
    for (int i = 0; i < GraphNodeRegistry::maxOscillators; ++i)
    {
        auto osc = std::make_unique<OscillatorProcessor> (parameters.oscFrequency[static_cast<size_t> (i)],
                                                          parameters.oscWaveform [static_cast<size_t> (i)]);
        oscillators[static_cast<size_t> (i)] = osc.get();
        graph.addProcessor (std::move (osc));
    }

    // FM pool.
    for (int i = 0; i < GraphNodeRegistry::maxFmAmounts; ++i)
    {
        auto fm = std::make_unique<FMModulationProcessor> (parameters.fmAmount[static_cast<size_t> (i)]);
        fmProcessors[static_cast<size_t> (i)] = fm.get();
        graph.addProcessor (std::move (fm));
    }

    // ADSR (singleton).
    auto adsr = std::make_unique<AdsrProcessor> (parameters.attack,
                                                 parameters.decay,
                                                 parameters.sustain,
                                                 parameters.release);
    adsrProcessor = adsr.get();
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

    if (noteProcessor != nullptr)
        noteProcessor->setMidiNoteNumber (midiNoteNumber);

    graph.startNote();
}

void SynthVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        if (adsrProcessor != nullptr)
            adsrProcessor->noteOff();
    }
    else
    {
        clearCurrentNote();
    }
}

void SynthVoice::pitchWheelMoved (int) {}
void SynthVoice::controllerMoved (int, int) {}

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                  int startSample,
                                  int numSamples)
{
    if (adsrProcessor != nullptr && ! adsrProcessor->isActive())
    {
        clearCurrentNote();
        return;
    }

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        float outputSample = graph.processSample() * currentVelocity;

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            auto* channelData = outputBuffer.getWritePointer (channel);
            channelData[startSample + sampleIndex] += outputSample;
        }
    }

    if (adsrProcessor != nullptr && ! adsrProcessor->isActive())
        clearCurrentNote();
}

void SynthVoice::applyConnectionPatch (const ConnectionPatch& patch)
{
    if (noteProcessor == nullptr || adsrProcessor == nullptr)
        return;

    // -- 1. Disconnect everything -------------------------------------------
    for (auto* osc : oscillators)
        if (osc != nullptr)  osc->getNoteInput().disconnect();

    for (auto* fm : fmProcessors)
        if (fm != nullptr)
        {
            fm->getFreqInput().disconnect();
            fm->getModulatorInput().disconnect();
        }

    adsrProcessor->getInput().disconnect();

    // -- 2. Rewire according to the patch ------------------------------------
    bool noteConnected = false;

    for (const auto& conn : patch.connections)
    {
        OutputPort* out = resolveOutput (conn.from.nodeId, conn.from.portId,
                                         noteProcessor, adsrProcessor,
                                         oscillators, fmProcessors);
        InputPort*  in  = resolveInput  (conn.to.nodeId,   conn.to.portId,
                                         const_cast<SynthVoiceParameters&> (parameters),
                                         noteProcessor, adsrProcessor,
                                         oscillators, fmProcessors);

        if (out == nullptr || in == nullptr)
            continue;

        in->connect (*out);

        if (GraphNodeRegistry::typeOf (conn.from.nodeId) == NodeType::note)
            noteConnected = true;
    }

    noteProcessor->setEnabled (noteConnected);
}

} // namespace smolfm
