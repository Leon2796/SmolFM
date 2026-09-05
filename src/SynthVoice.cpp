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
    // lookup â€” no allocations, safe for the message thread while audio runs.

    smolfm::OutputPort* findOutputPort (const juce::String& nodeId,
                                        const juce::String& portId,
                                        smolfm::SynthVoice& voice);
                                        // implemented below via the pools

    smolfm::InputPort* resolveInput (const juce::String& nodeId,
                                     const juce::String& portId,
                                     SynthVoiceParameters& params,
                                     std::array<NoteProcessor*, GraphNodeRegistry::maxNotes>& noteSources,
                                     std::array<AdsrProcessor*, GraphNodeRegistry::maxAdsr>& adsrProcessors,
                                     MasterOutputProcessor* masterOutput,
                                     std::array<OscillatorProcessor*, GraphNodeRegistry::maxOscillators>& oscillators,
                                                                          std::array<FMModulationProcessor*, GraphNodeRegistry::maxFmAmounts>& fmProcessors,
                                     std::array<FrequencyScaleProcessor*, GraphNodeRegistry::maxFrequencyScales>& frequencyScalers,
                                                                          std::array<RingModulatorProcessor*, GraphNodeRegistry::maxRingModulators>& ringModulators,
                                                                          std::array<AmProcessor*, GraphNodeRegistry::maxAmModulators>& amModulators,
                                                                          std::array<DelayProcessor*, GraphNodeRegistry::maxDelays>& delays)
    {
        juce::ignoreUnused (params, noteSources);

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

                if (type == NodeType::ringModulator
                         && index >= 0 && index < GraphNodeRegistry::maxRingModulators
                         && ringModulators[static_cast<size_t> (index)] != nullptr)
                        {
                            if (portId == "in1") return &ringModulators[static_cast<size_t> (index)]->getInput1();
                            if (portId == "in2") return &ringModulators[static_cast<size_t> (index)]->getInput2();
                            return nullptr;
                }

                if (type == NodeType::amModulator
                 && index >= 0 && index < GraphNodeRegistry::maxAmModulators
                 && amModulators[static_cast<size_t> (index)] != nullptr)
                {
                    if (portId == "carrier_in")   return &amModulators[static_cast<size_t> (index)]->getCarrierInput();
                    if (portId == "modulator_in") return &amModulators[static_cast<size_t> (index)]->getModulatorInput();
                    return nullptr;
                }

                if (type == NodeType::delay
                 && index >= 0 && index < GraphNodeRegistry::maxDelays
                 && delays[static_cast<size_t> (index)] != nullptr)
                {
                    if (portId == "in")
                        return &delays[static_cast<size_t> (index)]->getInput();
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

        if (type == NodeType::frequencyScale
         && index >= 0 && index < GraphNodeRegistry::maxFrequencyScales
         && frequencyScalers[static_cast<size_t> (index)] != nullptr)
        {
            if (portId == "freq_in")
                return &frequencyScalers[static_cast<size_t> (index)]->getFreqInput();

            return nullptr;
        }

        if (type == NodeType::adsr
         && index >= 0 && index < GraphNodeRegistry::maxAdsr
         && adsrProcessors[static_cast<size_t> (index)] != nullptr
         && portId == "in")
            return &adsrProcessors[static_cast<size_t> (index)]->getInput();

                if (type == NodeType::ringModulator
         && index >= 0 && index < GraphNodeRegistry::maxRingModulators
         && ringModulators[static_cast<size_t> (index)] != nullptr)
        {
            if (portId == "in1") return &ringModulators[static_cast<size_t> (index)]->getInput1();
            if (portId == "in2") return &ringModulators[static_cast<size_t> (index)]->getInput2();
            return nullptr;
        }

        if (type == NodeType::masterOutput && masterOutput != nullptr)
        {
            if (portId.startsWith ("in"))
            {
                const int inIndex = portId.substring (2).getIntValue() - 1;
                if (inIndex >= 0 && inIndex < MasterOutputProcessor::numInputs)
                    return &masterOutput->getInput (inIndex);
            }
            return nullptr;
        }

        return nullptr;
    }

    smolfm::OutputPort* resolveOutput (const juce::String& nodeId,
                                       const juce::String& portId,
                                       std::array<NoteProcessor*, GraphNodeRegistry::maxNotes>& noteSources,
                                       std::array<AdsrProcessor*, GraphNodeRegistry::maxAdsr>& adsrProcessors,
                                       MasterOutputProcessor* masterOutput,
                                       std::array<OscillatorProcessor*, GraphNodeRegistry::maxOscillators>& oscillators,
                                                                              std::array<FMModulationProcessor*, GraphNodeRegistry::maxFmAmounts>& fmProcessors,
                                       std::array<FrequencyScaleProcessor*, GraphNodeRegistry::maxFrequencyScales>& frequencyScalers,
                                                                              std::array<RingModulatorProcessor*, GraphNodeRegistry::maxRingModulators>& ringModulators,
                                                                              std::array<AmProcessor*, GraphNodeRegistry::maxAmModulators>& amModulators,
                                                                              std::array<DelayProcessor*, GraphNodeRegistry::maxDelays>& delays)
    {
        if (portId != "out")
            return nullptr;

        const NodeType type = GraphNodeRegistry::typeOf (nodeId);
        const int index = GraphNodeRegistry::indexOf (nodeId);

        if (type == NodeType::note
         && index >= 0 && index < GraphNodeRegistry::maxNotes
         && noteSources[static_cast<size_t> (index)] != nullptr)
            return &noteSources[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::oscillator
         && index >= 0 && index < GraphNodeRegistry::maxOscillators
         && oscillators[static_cast<size_t> (index)] != nullptr)
            return &oscillators[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::fmAmount
         && index >= 0 && index < GraphNodeRegistry::maxFmAmounts
         && fmProcessors[static_cast<size_t> (index)] != nullptr)
            return &fmProcessors[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::frequencyScale
         && index >= 0 && index < GraphNodeRegistry::maxFrequencyScales
         && frequencyScalers[static_cast<size_t> (index)] != nullptr)
            return &frequencyScalers[static_cast<size_t> (index)]->getOutput();

                if (type == NodeType::adsr
         && index >= 0 && index < GraphNodeRegistry::maxAdsr
         && adsrProcessors[static_cast<size_t> (index)] != nullptr)
            return &adsrProcessors[static_cast<size_t> (index)]->getOutput();

                if (type == NodeType::ringModulator
         && index >= 0 && index < GraphNodeRegistry::maxRingModulators
         && ringModulators[static_cast<size_t> (index)] != nullptr)
            return &ringModulators[static_cast<size_t> (index)]->getOutput();

                if (type == NodeType::amModulator
         && index >= 0 && index < GraphNodeRegistry::maxAmModulators
         && amModulators[static_cast<size_t> (index)] != nullptr)
            return &amModulators[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::delay
         && index >= 0 && index < GraphNodeRegistry::maxDelays
         && delays[static_cast<size_t> (index)] != nullptr)
            return &delays[static_cast<size_t> (index)]->getOutput();

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
    // Note source pool â€” every instance mirrors the same played MIDI note
    // on its own output, so several "note" boxes can feed different chains.
    for (int i = 0; i < GraphNodeRegistry::maxNotes; ++i)
    {
        auto note = std::make_unique<NoteProcessor>();
        noteSources[static_cast<size_t> (i)] = note.get();
        graph.addProcessor (std::move (note));
    }

    // Oscillator pool â€” all created up front, all fed by their own parameter
    // pair.  An osc box that is never wired just renders (harmlessly, it's
    // cheap) and its output is never read by anyone.
    for (int i = 0; i < GraphNodeRegistry::maxOscillators; ++i)
    {
        auto osc = std::make_unique<OscillatorProcessor> (parameters.oscWaveform [static_cast<size_t> (i)]);
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

    // Frequency scale pool.
    for (int i = 0; i < GraphNodeRegistry::maxFrequencyScales; ++i)
    {
        auto fscale = std::make_unique<FrequencyScaleProcessor> (parameters.freqScaleFactor[static_cast<size_t> (i)]);
        frequencyScalers[static_cast<size_t> (i)] = fscale.get();
        graph.addProcessor (std::move (fscale));
    }

    // ADSR pool â€” pure envelope filters; the master output is the end of
    // the chain, not an ADSR.
    for (int i = 0; i < GraphNodeRegistry::maxAdsr; ++i)
    {
        auto adsr = std::make_unique<AdsrProcessor> (parameters.adsrAttack [static_cast<size_t> (i)],
                                                     parameters.adsrDecay  [static_cast<size_t> (i)],
                                                     parameters.adsrSustain[static_cast<size_t> (i)],
                                                     parameters.adsrRelease[static_cast<size_t> (i)]);
        adsrProcessors[static_cast<size_t> (i)] = adsr.get();
        graph.addProcessor (std::move (adsr));
    }

        // Ring modulator pool â€” pure signal multipliers, no parameters.
    for (int i = 0; i < GraphNodeRegistry::maxRingModulators; ++i)
    {
        auto ring = std::make_unique<RingModulatorProcessor>();
        ringModulators[static_cast<size_t> (i)] = ring.get();
        graph.addProcessor (std::move (ring));
    }

        // AM modulator pool — depth-controlled amplitude modulators.
        for (int i = 0; i < GraphNodeRegistry::maxAmModulators; ++i)
        {
            auto am = std::make_unique<AmProcessor> (parameters.amAmount[static_cast<size_t> (i)]);
            amModulators[static_cast<size_t> (i)] = am.get();
            graph.addProcessor (std::move (am));
        }

        // Delay pool — tempo-sync or free-running with feedback.
        for (int i = 0; i < GraphNodeRegistry::maxDelays; ++i)
        {
            auto delay = std::make_unique<DelayProcessor> (parameters.delayTimeMs  [static_cast<size_t> (i)],
                                                           parameters.delayFeedback[static_cast<size_t> (i)],
                                                           parameters.delayMix     [static_cast<size_t> (i)],
                                                           parameters.delaySync    [static_cast<size_t> (i)],
                                                           parameters.delayDivision[static_cast<size_t> (i)]);
            delays[static_cast<size_t> (i)] = delay.get();
            graph.addProcessor (std::move (delay));
        }

        // Master output (singleton).
    auto master = std::make_unique<MasterOutputProcessor> (parameters.masterLevel);
    masterOutput = master.get();
    graph.addProcessor (std::move (master));
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
    keyHeld = true;

    for (auto* note : noteSources)
        if (note != nullptr)
            note->setMidiNoteNumber (midiNoteNumber);

    graph.startNote();
}

void SynthVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
        {
        for (auto* adsr : adsrProcessors)
            if (adsr != nullptr)
                adsr->noteOff();
    }
    else
    {
        clearCurrentNote();
    }

    // Key is up either way.  With allowTailOff the envelopes (and any delay
    // tail) decide when the voice is done; without it the voice is silenced
    // immediately by clearCurrentNote.
    keyHeld = false;
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
    if (masterOutput == nullptr)
    {
        clearCurrentNote();
        return;
    }

        // Idle voices (no note assigned) must output nothing — JUCE calls
    // renderNextBlock on every voice every block, active or not.
    if (! isVoiceActive())
        return;

    // Voice lifetime policy: render only while the key is held, an envelope
    // is sounding, or a delay tail is still ringing.  Fixes two bugs:
    //   1. A patch with no wired envelope used to drone forever (note sources
    //      keep emitting the last note frequency).
    //   2. Delay tails used to be chopped the moment the last envelope
    //      finished; now the voice stays alive while the tail rings out.
    if (! hasReasonToLive())
    {
        // Last gasp: fade this whole block to zero so stopping mid-signal
        // does not click, then free the voice.
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            const float fade = 1.0f - static_cast<float> (sampleIndex)
                                    / static_cast<float> (numSamples);
            const float outputSample = graph.processSample() * currentVelocity * fade;

            for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
                outputBuffer.getWritePointer (channel)[startSample + sampleIndex] += outputSample;
        }

        clearCurrentNote();
        return;
    }

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        // graph.processSample() runs the whole chain; the master output is
        // added last, so its return value is the final sample.
        const float outputSample = graph.processSample() * currentVelocity;

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            auto* channelData = outputBuffer.getWritePointer (channel);
            channelData[startSample + sampleIndex] += outputSample;
        }
    }
}

float SynthVoice::getMasterPeakLevel() const noexcept
{
    return masterOutput != nullptr ? masterOutput->getPeakLevel() : 0.0f;
}

void SynthVoice::setHostTempo (double bpm) noexcept
{
    for (auto* d : delays)
        if (d != nullptr)
            d->setHostTempo (bpm);
}

bool SynthVoice::hasReasonToLive() const noexcept
{
    if (keyHeld)
        return true;

    for (auto* adsr : adsrProcessors)
        if (adsr != nullptr && adsr->getInput().isConnected() && adsr->isActive())
            return true;

    for (auto* d : delays)
        if (d != nullptr && d->getInput().isConnected() && d->hasEnergy())
            return true;

    return false;
}

void SynthVoice::applyConnectionPatch (const ConnectionPatch& patch)
{
    if (masterOutput == nullptr)
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

    for (auto* fscale : frequencyScalers)
        if (fscale != nullptr)  fscale->getFreqInput().disconnect();

        for (auto* adsr : adsrProcessors)
        if (adsr != nullptr)  adsr->getInput().disconnect();

        for (auto* ring : ringModulators)
        if (ring != nullptr)
        {
            ring->getInput1().disconnect();
            ring->getInput2().disconnect();
        }

        for (auto* am : amModulators)
        if (am != nullptr)
        {
            am->getCarrierInput().disconnect();
            am->getModulatorInput().disconnect();
        }

        for (auto* d : delays)
        if (d != nullptr)
        {
            d->getInput().disconnect();
            // A new patch must not inherit the previous patch's delay tail:
            // clear the line so no stale signal survives an instrument change.
            d->reset();
        }

    for (int i = 0; i < MasterOutputProcessor::numInputs; ++i)
        masterOutput->getInput (i).disconnect();

    for (auto* note : noteSources)
        if (note != nullptr)  note->setEnabled (false);

    // -- 2. Rewire according to the patch ------------------------------------
    for (const auto& conn : patch.connections)
    {
                OutputPort* out = resolveOutput (conn.from.nodeId, conn.from.portId,
                                                   noteSources, adsrProcessors, masterOutput,
                                                   oscillators, fmProcessors, frequencyScalers, ringModulators, amModulators, delays);
                InputPort*  in  = resolveInput  (conn.to.nodeId,   conn.to.portId,
                                                   const_cast<SynthVoiceParameters&> (parameters),
                                                   noteSources, adsrProcessors, masterOutput,
                                                   oscillators, fmProcessors, frequencyScalers, ringModulators, amModulators, delays);

        if (out == nullptr || in == nullptr)
            continue;

        in->connect (*out);

        if (GraphNodeRegistry::typeOf (conn.from.nodeId) == NodeType::note)
        {
            const int noteIndex = GraphNodeRegistry::indexOf (conn.from.nodeId);
            if (noteIndex >= 0 && noteIndex < GraphNodeRegistry::maxNotes
             && noteSources[static_cast<size_t> (noteIndex)] != nullptr)
                noteSources[static_cast<size_t> (noteIndex)]->setEnabled (true);
        }
    }

    // Note sources that are not wired stay silent until the next patch or note.
}

} // namespace smolfm
