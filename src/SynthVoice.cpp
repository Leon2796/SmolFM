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
                                     std::array<VelocityScaleProcessor*, GraphNodeRegistry::maxVelocityScales>& velocityScalers,
                                     std::array<NoteProcessor*, GraphNodeRegistry::maxNotes>& noteSources,
                                     std::array<AdsrProcessor*, GraphNodeRegistry::maxAdsr>& adsrProcessors,
                                     std::array<FAdsrProcessor*, GraphNodeRegistry::maxFAdsr>& fAdsrProcessors,
                                     MasterOutputProcessor* masterOutput,
                                     std::array<OscillatorProcessor*, GraphNodeRegistry::maxOscillators>& oscillators,
                                                                          std::array<FMModulationProcessor*, GraphNodeRegistry::maxFmAmounts>& fmProcessors,
                                     std::array<FrequencyScaleProcessor*, GraphNodeRegistry::maxFrequencyScales>& frequencyScalers,
                                                                          std::array<RingModulatorProcessor*, GraphNodeRegistry::maxRingModulators>& ringModulators,
                                                                          std::array<AmProcessor*, GraphNodeRegistry::maxAmModulators>& amModulators,
                                                                          std::array<DelayProcessor*, GraphNodeRegistry::maxDelays>& delays,
                                                                          std::array<SampleHoldProcessor*, GraphNodeRegistry::maxSampleHolds>& holdProcessors,
                                                                          std::array<GainProcessor*, GraphNodeRegistry::maxGains>& gainProcessors,
                                     std::array<WaveshaperProcessor*, GraphNodeRegistry::maxWaveshapers>& waveshapers,
                                     std::array<FilterProcessor*, GraphNodeRegistry::maxFilters>& filters,
                                     std::array<SplitterProcessor*, GraphNodeRegistry::maxSplitters>& splitters,
                                     std::array<MergerProcessor*, GraphNodeRegistry::maxMergers>& mergers,
                                     std::array<AdderProcessor*, GraphNodeRegistry::maxAdds>& adders,
                                      std::array<FeedbackProcessor*, GraphNodeRegistry::maxFeedbacks>& feedbackProcessors,
                                      std::array<ResonatorProcessor*, GraphNodeRegistry::maxResonators>& resonators)
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
                    if (portId == "time_mod_in")
                        return &delays[static_cast<size_t> (index)]->getTimeModInput();
                    return nullptr;
                }
                if (type == NodeType::sampleHold
                 && index >= 0 && index < GraphNodeRegistry::maxSampleHolds
                 && holdProcessors[static_cast<size_t> (index)] != nullptr)
                {
                    if (portId == "in")
                        return &holdProcessors[static_cast<size_t> (index)]->getInput();
                    if (portId == "rate_mod_in")
                        return &holdProcessors[static_cast<size_t> (index)]->getRateModInput();
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

            if (portId == "amount_mod_in")
                return &fmProcessors[static_cast<size_t> (index)]->getAmountModInput();

            if (portId == "vel")
                return &fmProcessors[static_cast<size_t> (index)]->getVelInput();

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

        if (type == NodeType::velocityScale
         && index >= 0 && index < GraphNodeRegistry::maxVelocityScales
          && velocityScalers[static_cast<size_t> (index)] != nullptr)
        {
            if (portId == "vel_in")
                return &velocityScalers[static_cast<size_t> (index)]->getVelInput();

            return nullptr;
        }

        if (type == NodeType::adsr
         && index >= 0 && index < GraphNodeRegistry::maxAdsr
         && adsrProcessors[static_cast<size_t> (index)] != nullptr
         && portId == "in")
            return &adsrProcessors[static_cast<size_t> (index)]->getInput();

        if (type == NodeType::fAdsr
         && index >= 0 && index < GraphNodeRegistry::maxFAdsr
         && fAdsrProcessors[static_cast<size_t> (index)] != nullptr)
        {
            if (portId == "freq_in") return &fAdsrProcessors[static_cast<size_t> (index)]->getFreqInput();
            if (portId == "vel")     return &fAdsrProcessors[static_cast<size_t> (index)]->getVelInput();
            return nullptr;
        }

        if (type == NodeType::gain
         && index >= 0 && index < GraphNodeRegistry::maxGains
         && gainProcessors[static_cast<size_t> (index)] != nullptr)
        {
            if (portId == "in")           return &gainProcessors[static_cast<size_t> (index)]->getInput();
            if (portId == "factor_mod_in") return &gainProcessors[static_cast<size_t> (index)]->getModInput();
            if (portId == "vel")           return &gainProcessors[static_cast<size_t> (index)]->getVelInput();
            return nullptr;
        }

        if (type == NodeType::waveshaper
         && index >= 0 && index < GraphNodeRegistry::maxWaveshapers
         && waveshapers[static_cast<size_t> (index)] != nullptr)
        {
            if (portId == "in")            return &waveshapers[static_cast<size_t> (index)]->getInput();
            if (portId == "drive_mod_in")  return &waveshapers[static_cast<size_t> (index)]->getDriveModInput();
            if (portId == "vel")           return &waveshapers[static_cast<size_t> (index)]->getVelInput();
            return nullptr;
        }

        if (type == NodeType::filter
         && index >= 0 && index < GraphNodeRegistry::maxFilters
         && filters[static_cast<size_t> (index)] != nullptr)
        {
            if (portId == "in")             return &filters[static_cast<size_t> (index)]->getInput();
            if (portId == "cutoff_mod_in")  return &filters[static_cast<size_t> (index)]->getCutoffModInput();
            if (portId == "resonance_mod_in") return &filters[static_cast<size_t> (index)]->getResonanceModInput();
            if (portId == "vel")            return &filters[static_cast<size_t> (index)]->getVelInput();
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

        if (type == NodeType::splitter
         && index >= 0 && index < GraphNodeRegistry::maxSplitters
         && splitters[static_cast<size_t> (index)] != nullptr)
         {
             if (portId == "in")
                 return &splitters[static_cast<size_t> (index)]->getInput();
             return nullptr;
         }

        if (type == NodeType::merger
         && index >= 0 && index < GraphNodeRegistry::maxMergers
         && mergers[static_cast<size_t> (index)] != nullptr)
         {
             if (portId == "in1") return &mergers[static_cast<size_t> (index)]->getLeftInput();
             if (portId == "in2") return &mergers[static_cast<size_t> (index)]->getRightInput();
             return nullptr;
         }

        if (type == NodeType::adder
         && index >= 0 && index < GraphNodeRegistry::maxAdds
         && adders[static_cast<size_t> (index)] != nullptr)
         {
             if (portId == "in1") return &adders[static_cast<size_t> (index)]->getInput1();
             if (portId == "in2") return &adders[static_cast<size_t> (index)]->getInput2();
             return nullptr;
         }

        if (type == NodeType::resonator
         && index >= 0 && index < GraphNodeRegistry::maxResonators
         && resonators[static_cast<size_t> (index)] != nullptr)
         {
             if (portId == "in")         return &resonators[static_cast<size_t> (index)]->getInput();
             if (portId == "tune")       return &resonators[static_cast<size_t> (index)]->getTuneInput();
             if (portId == "q_mod_in")   return &resonators[static_cast<size_t> (index)]->getQModInput();
             if (portId == "vel")        return &resonators[static_cast<size_t> (index)]->getVelInput();
             if (portId == "strike_vel") return &resonators[static_cast<size_t> (index)]->getStrikeVelInput();
             return nullptr;
         }

        if (type == NodeType::feedback
         && index >= 0 && index < GraphNodeRegistry::maxFeedbacks
         && feedbackProcessors[static_cast<size_t> (index)] != nullptr)
         {
             if (portId == "in")    return &feedbackProcessors[static_cast<size_t> (index)]->getInput();
             if (portId == "fb_in") return &feedbackProcessors[static_cast<size_t> (index)]->getFeedbackInput();
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
                                       std::array<FAdsrProcessor*, GraphNodeRegistry::maxFAdsr>& fAdsrProcessors,
                                       MasterOutputProcessor* masterOutput,
                                       std::array<OscillatorProcessor*, GraphNodeRegistry::maxOscillators>& oscillators,
                                                                              std::array<FMModulationProcessor*, GraphNodeRegistry::maxFmAmounts>& fmProcessors,
                                       std::array<FrequencyScaleProcessor*, GraphNodeRegistry::maxFrequencyScales>& frequencyScalers,
                                                                              std::array<RingModulatorProcessor*, GraphNodeRegistry::maxRingModulators>& ringModulators,
                                                                              std::array<AmProcessor*, GraphNodeRegistry::maxAmModulators>& amModulators,
                                                                              std::array<DelayProcessor*, GraphNodeRegistry::maxDelays>& delays,
                                                                              std::array<SampleHoldProcessor*, GraphNodeRegistry::maxSampleHolds>& holdProcessors,
                                                                              std::array<GainProcessor*, GraphNodeRegistry::maxGains>& gainProcessors,
                                                                              std::array<WaveshaperProcessor*, GraphNodeRegistry::maxWaveshapers>& waveshapers,
                                       std::array<FilterProcessor*, GraphNodeRegistry::maxFilters>& filters,
                                       std::array<SplitterProcessor*, GraphNodeRegistry::maxSplitters>& splitters,
                                       std::array<MergerProcessor*, GraphNodeRegistry::maxMergers>& mergers,
                                       std::array<AdderProcessor*, GraphNodeRegistry::maxAdds>& adders,
                                        std::array<FeedbackProcessor*, GraphNodeRegistry::maxFeedbacks>& feedbackProcessors,
                                        std::array<ResonatorProcessor*, GraphNodeRegistry::maxResonators>& resonators,
                                        std::array<VelocityScaleProcessor*, GraphNodeRegistry::maxVelocityScales>& velocityScalers)
    {
        if (portId != "out" && portId != "outL" && portId != "outR" && portId != "vel")
            return nullptr;

        const NodeType type = GraphNodeRegistry::typeOf (nodeId);
        const int index = GraphNodeRegistry::indexOf (nodeId);

        if (type == NodeType::note
         && index >= 0 && index < GraphNodeRegistry::maxNotes
         && noteSources[static_cast<size_t> (index)] != nullptr)
        {
            if (portId == "vel")   // velocity send: separate domain
                return &noteSources[static_cast<size_t> (index)]->getVelOutput();

            return &noteSources[static_cast<size_t> (index)]->getOutput();
        }

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

        if (type == NodeType::velocityScale
         && index >= 0 && index < GraphNodeRegistry::maxVelocityScales
          && velocityScalers[static_cast<size_t> (index)] != nullptr)
            return &velocityScalers[static_cast<size_t> (index)]->getOutput();

                if (type == NodeType::adsr
         && index >= 0 && index < GraphNodeRegistry::maxAdsr
         && adsrProcessors[static_cast<size_t> (index)] != nullptr)
            return &adsrProcessors[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::fAdsr
         && index >= 0 && index < GraphNodeRegistry::maxFAdsr
         && fAdsrProcessors[static_cast<size_t> (index)] != nullptr)
            return &fAdsrProcessors[static_cast<size_t> (index)]->getOutput();

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

        if (type == NodeType::sampleHold
         && index >= 0 && index < GraphNodeRegistry::maxSampleHolds
         && holdProcessors[static_cast<size_t> (index)] != nullptr)
            return &holdProcessors[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::gain
         && index >= 0 && index < GraphNodeRegistry::maxGains
         && gainProcessors[static_cast<size_t> (index)] != nullptr)
            return &gainProcessors[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::waveshaper
         && index >= 0 && index < GraphNodeRegistry::maxWaveshapers
         && waveshapers[static_cast<size_t> (index)] != nullptr)
            return &waveshapers[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::filter
         && index >= 0 && index < GraphNodeRegistry::maxFilters
         && filters[static_cast<size_t> (index)] != nullptr)
            return &filters[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::resonator
         && index >= 0 && index < GraphNodeRegistry::maxResonators
         && resonators[static_cast<size_t> (index)] != nullptr)
            return &resonators[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::feedback
         && index >= 0 && index < GraphNodeRegistry::maxFeedbacks
         && feedbackProcessors[static_cast<size_t> (index)] != nullptr)
            return &feedbackProcessors[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::splitter
         && index >= 0 && index < GraphNodeRegistry::maxSplitters
         && splitters[static_cast<size_t> (index)] != nullptr)
         {
             if (portId == "outL") return &splitters[static_cast<size_t> (index)]->getLeftOutput();
             if (portId == "outR") return &splitters[static_cast<size_t> (index)]->getRightOutput();
             return nullptr;
         }

        if (type == NodeType::merger
         && index >= 0 && index < GraphNodeRegistry::maxMergers
         && mergers[static_cast<size_t> (index)] != nullptr)
            return &mergers[static_cast<size_t> (index)]->getOutput();

        if (type == NodeType::adder
         && index >= 0 && index < GraphNodeRegistry::maxAdds
         && adders[static_cast<size_t> (index)] != nullptr)
            return &adders[static_cast<size_t> (index)]->getOutput();

        return nullptr;
    }

    //==========================================================================
    // End of the anonymous helper namespace; the smolfm namespace closes at
    // the bottom of the file after the SynthVoice member definitions.
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
        auto osc = std::make_unique<OscillatorProcessor> (parameters.oscWaveform [static_cast<size_t> (i)],
                                                          parameters.oscMode       [static_cast<size_t> (i)],
                                                          parameters.oscStaticFreq [static_cast<size_t> (i)],
                                                          parameters.oscLfoRate    [static_cast<size_t> (i)]);
        oscillators[static_cast<size_t> (i)] = osc.get();
        osc->setBypassParam (parameters.oscBypass [static_cast<size_t> (i)]);
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
        auto fscale = std::make_unique<FrequencyScaleProcessor> (parameters.freqScaleFactor [static_cast<size_t> (i)],
                                                                 parameters.freqScaleInterval[static_cast<size_t> (i)]);
        frequencyScalers[static_cast<size_t> (i)] = fscale.get();
        graph.addProcessor (std::move (fscale));
    }

    // Velocity scale pool — velocity-domain scalers.  Placed with the other
    // domain transformers (after the note/fm/fscale sources, before every
    // vel consumer) so accent chains read a fresh velocity in the same tick.
    for (int i = 0; i < GraphNodeRegistry::maxVelocityScales; ++i)
    {
        auto vsc = std::make_unique<VelocityScaleProcessor> (parameters.velScaleCurve [static_cast<size_t> (i)],
                                                             parameters.velScaleScale [static_cast<size_t> (i)]);
        velocityScalers[static_cast<size_t> (i)] = vsc.get();
        graph.addProcessor (std::move (vsc));
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
        adsr->setBypassParam (parameters.adsrBypass [static_cast<size_t> (i)]);
        graph.addProcessor (std::move (adsr));
    }

    // F-ADSR pitch-envelope pool — frequency-domain envelopes; placed after
    // note/fm/fscale producers and before the signal-domain stages.
    for (int i = 0; i < GraphNodeRegistry::maxFAdsr; ++i)
    {
        auto fadsr = std::make_unique<FAdsrProcessor> (parameters.fAdsrAttack [static_cast<size_t> (i)],
                                                       parameters.fAdsrDecay  [static_cast<size_t> (i)],
                                                       parameters.fAdsrSustain[static_cast<size_t> (i)],
                                                       parameters.fAdsrRelease[static_cast<size_t> (i)],
                                                       parameters.fAdsrUp     [static_cast<size_t> (i)],
                                                       parameters.fAdsrDown   [static_cast<size_t> (i)]);
        fAdsrProcessors[static_cast<size_t> (i)] = fadsr.get();
        fadsr->setBypassParam (parameters.fAdsrBypass [static_cast<size_t> (i)]);
        graph.addProcessor (std::move (fadsr));
    }

        // Splitter pool — stereo channel dividers with per-side outputs
        // (outL = (L,0), outR = (0,R)).  Placed after every frequency
        // producer and before the signal-domain stages so split branches
        // feed ring/am/delay/gain/shape/filter and the master output in the
        // same tick.  A producer from one of the later signal pools feeds
        // the splitter/merger one tick late (inaudible; creates no feedback).
        for (int i = 0; i < GraphNodeRegistry::maxSplitters; ++i)
        {
            auto split = std::make_unique<SplitterProcessor>();
            splitters[static_cast<size_t> (i)] = split.get();
            graph.addProcessor (std::move (split));
        }

        // Merger pool — stereo channel combiners (out = (in1.l, in2.r)).
        for (int i = 0; i < GraphNodeRegistry::maxMergers; ++i)
        {
            auto merge = std::make_unique<MergerProcessor>();
            mergers[static_cast<size_t> (i)] = merge.get();
            graph.addProcessor (std::move (merge));
        }

        // Adder pool — samplewise signal sum (out = in1 + in2).  The bus
        // builder: folds parallel signal sources into one mono/stereo signal
        // BEFORE the shared filters/shapers/envelopes consume it.  Positioned
        // after the splitter/merger topology group and before the signal
        // stages, so osc->add and add->downstream stay in the same tick;
        // feeding the splitter from an adder costs one tick (same concession
        // as every other later signal pool).
        for (int i = 0; i < GraphNodeRegistry::maxAdds; ++i)
        {
            auto add = std::make_unique<AdderProcessor>();
            adders[static_cast<size_t> (i)] = add.get();
            graph.addProcessor (std::move (add));
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
            delay->setBypassParam (parameters.delayBypass [static_cast<size_t> (i)]);
            graph.addProcessor (std::move (delay));
        }

        // Gain pool — stateless boost/attenuate stages.
        for (int i = 0; i < GraphNodeRegistry::maxGains; ++i)
        {
            auto g = std::make_unique<GainProcessor> (parameters.gainFactor[static_cast<size_t> (i)]);
            gainProcessors[static_cast<size_t> (i)] = g.get();
            g->setBypassParam (parameters.gainBypass [static_cast<size_t> (i)]);
            graph.addProcessor (std::move (g));
        }

        // Waveshaper pool — four transfer functions with drive.
        for (int i = 0; i < GraphNodeRegistry::maxWaveshapers; ++i)
        {
            auto ws = std::make_unique<WaveshaperProcessor> (parameters.shaperDrive [static_cast<size_t> (i)],
                                                             parameters.shaperShape[static_cast<size_t> (i)]);
            waveshapers[static_cast<size_t> (i)] = ws.get();
            ws->setBypassParam (parameters.shaperBypass [static_cast<size_t> (i)]);
            graph.addProcessor (std::move (ws));
        }

        // Filter pool — multi-mode SVF; sits with the other signal-domain
        // stages, after every frequency producer and before envelopes/output.
        for (int i = 0; i < GraphNodeRegistry::maxFilters; ++i)
        {
            auto f = std::make_unique<FilterProcessor> (parameters.filterCutoff   [static_cast<size_t> (i)],
                                                        parameters.filterResonance[static_cast<size_t> (i)],
                                                        parameters.filterMode     [static_cast<size_t> (i)]);
            filters[static_cast<size_t> (i)] = f.get();
            f->setBypassParam (parameters.filterBypass [static_cast<size_t> (i)]);
            graph.addProcessor (std::move (f));
        }

        // Sample & hold pool - rate-grid quantiser (Bitcrush time axis /
        // stepped mod). Sits at the END of the signal band: the node reads
        // every earlier signal producer fresh in the same tick; only the
        // feedback bridge reaches it with the inherent one-sample delay.
        for (int i = 0; i < GraphNodeRegistry::maxSampleHolds; ++i)
        {
            auto sh = std::make_unique<SampleHoldProcessor> (parameters.holdRate     [static_cast<size_t> (i)],
                                                             parameters.holdSync     [static_cast<size_t> (i)],
                                                             parameters.holdDivision [static_cast<size_t> (i)]);
            holdProcessors[static_cast<size_t> (i)] = sh.get();
            graph.addProcessor (std::move (sh));
        }

        // Feedback pool — bounded cycle breakers (plan P3.6).  They sit
        // BEFORE the resonator pool: the node's `out` carries the loop entry
        // value fresh into its downstream body, while its `fb_in` return
        // wire points back to a node added later, whose frame is read with
        // the inherent one-sample delay that makes the cycle legal.  The
        // `in` producers (exciters upstream of the loop) live earlier.
        for (int i = 0; i < GraphNodeRegistry::maxFeedbacks; ++i)
        {
            auto fb = std::make_unique<FeedbackProcessor> (parameters.feedbackAmount[static_cast<size_t> (i)]);
            feedbackProcessors[static_cast<size_t> (i)] = fb.get();
            graph.addProcessor (std::move (fb));
        }

        // Resonator pool — excited damped second-order bodies (plan P2.2);
        // fed directly by earlier exciter chains or by a feedback entry.
        for (int i = 0; i < GraphNodeRegistry::maxResonators; ++i)
        {
            auto r = std::make_unique<ResonatorProcessor> (parameters.resonatorTune        [static_cast<size_t> (i)],
                                                           parameters.resonatorQ           [static_cast<size_t> (i)],
                                                           parameters.resonatorDecay       [static_cast<size_t> (i)],
                                                           parameters.resonatorVelSpread   [static_cast<size_t> (i)],
                                                           parameters.resonatorStrikeLevel [static_cast<size_t> (i)],
                                                           parameters.resonatorStrikeTime  [static_cast<size_t> (i)]);
            resonators[static_cast<size_t> (i)] = r.get();
            graph.addProcessor (std::move (r));
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
    keyHeld = true;

    for (auto* note : noteSources)
        if (note != nullptr)
        {
            note->setMidiNoteNumber (midiNoteNumber);
            note->setVelocity (velocity);   // vel sources emit per note-on
        }

    graph.startNote();
}

void SynthVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
        {
        for (auto* adsr : adsrProcessors)
            if (adsr != nullptr)
                adsr->noteOff();

        for (auto* fadsr : fAdsrProcessors)
            if (fadsr != nullptr)
                fadsr->noteOff();
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

            // The graph returns a stereo frame; L/R go to output channels 0/1.
            const StereoFrame frame = graph.processSample();

            auto* leftChannel  = outputBuffer.getWritePointer (0);
            auto* rightChannel = outputBuffer.getWritePointer (
                outputBuffer.getNumChannels() > 1 ? 1 : 0);

            // Velocity no longer scales here: it flows through the graph
            // as PortType::velocity from note.vel into the consumers' vel
            // pins (removing this central multiply avoids double scaling).
            leftChannel[startSample + sampleIndex]  += frame.l * fade;
            rightChannel[startSample + sampleIndex] += frame.r * fade;
        }

        clearCurrentNote();
        return;
    }

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        // graph.processSample() runs the whole chain; the master output is
        // added last, so its return value is the final stereo frame.  L/R go
        // to the host's output channels 0 and 1.
        const StereoFrame frame = graph.processSample();

        auto* leftChannel  = outputBuffer.getWritePointer (0);
        auto* rightChannel = outputBuffer.getWritePointer (
            outputBuffer.getNumChannels() > 1 ? 1 : 0);

        leftChannel[startSample + sampleIndex]  += frame.l;
        rightChannel[startSample + sampleIndex] += frame.r;
    }
}

float SynthVoice::getMasterPeakLevel() const noexcept
{
    return masterOutput != nullptr ? masterOutput->getPeakLevel() : 0.0f;
}

juce::Range<float> SynthVoice::getEffectiveControlRange (const NodeType type,
                                                         const int instanceIndex,
                                                         const int controlIndex) const noexcept
{
    // Each processor publishes its NeedleWindowTelemetry band (min/max of
    // the effective value over the needle window); read() itself returns
    // noEffectiveData() until the first window completed, so a live but
    // not-yet-rendered instance reports no data, never a stale value.
    switch (type)
    {
        case NodeType::fmAmount:
            if (controlIndex == 0 && instanceIndex >= 0
             && instanceIndex < GraphNodeRegistry::maxFmAmounts
             && fmProcessors[static_cast<size_t> (instanceIndex)] != nullptr)
                return fmProcessors[static_cast<size_t> (instanceIndex)]
                           ->getEffectiveAmountRange();
            return noEffectiveData();

        case NodeType::waveshaper:
            if (controlIndex == 0 && instanceIndex >= 0
             && instanceIndex < GraphNodeRegistry::maxWaveshapers
             && waveshapers[static_cast<size_t> (instanceIndex)] != nullptr)
                return waveshapers[static_cast<size_t> (instanceIndex)]
                           ->getEffectiveDriveRange();
            return noEffectiveData();

        case NodeType::filter:
            if (instanceIndex >= 0 && instanceIndex < GraphNodeRegistry::maxFilters
             && filters[static_cast<size_t> (instanceIndex)] != nullptr)
            {
                if (controlIndex == 0)
                    return filters[static_cast<size_t> (instanceIndex)]
                               ->getEffectiveCutoffRange();
                if (controlIndex == 1)
                    return filters[static_cast<size_t> (instanceIndex)]
                               ->getEffectiveResonanceRange();
            }
            return noEffectiveData();

        case NodeType::delay:
            if (controlIndex == 0 && instanceIndex >= 0
             && instanceIndex < GraphNodeRegistry::maxDelays
             && delays[static_cast<size_t> (instanceIndex)] != nullptr)
                return delays[static_cast<size_t> (instanceIndex)]
                           ->getEffectiveTimeMsRange();
            return noEffectiveData();

        case NodeType::resonator:
            if (controlIndex == 0 && instanceIndex >= 0
             && instanceIndex < GraphNodeRegistry::maxResonators
             && resonators[static_cast<size_t> (instanceIndex)] != nullptr)
                return resonators[static_cast<size_t> (instanceIndex)]
                           ->getEffectiveQRange();
            return noEffectiveData();

        case NodeType::gain:
            if (controlIndex == 0 && instanceIndex >= 0
             && instanceIndex < GraphNodeRegistry::maxGains
             && gainProcessors[static_cast<size_t> (instanceIndex)] != nullptr)
                return gainProcessors[static_cast<size_t> (instanceIndex)]
                           ->getEffectiveGainRange();
            return noEffectiveData();

        default:
            return noEffectiveData();
    }
}

void SynthVoice::setHostTempo (double bpm) noexcept
{
    for (auto* d : delays)
        if (d != nullptr)
            d->setHostTempo (bpm);

    for (auto* h : holdProcessors)
        if (h != nullptr)
            h->setHostTempo (bpm);
}

bool SynthVoice::hasReasonToLive() const noexcept
{
    if (keyHeld)
        return true;

    for (auto* adsr : adsrProcessors)
        if (adsr != nullptr && adsr->getInput().isConnected() && adsr->isActive())
            return true;

    for (auto* fadsr : fAdsrProcessors)
        if (fadsr != nullptr && fadsr->getFreqInput().isConnected() && fadsr->isActive())
            return true;

    // Delay tails ring out after note-off; the voice must stay alive while
    // any wired delay line still holds energy.  The tail estimate tracks the
    // recirculating line only (fresh input excluded) so a note source that
    // keeps sounding after note-off cannot freeze the voice here.
    for (auto* d : delays)
        if (d != nullptr && d->getInput().isConnected() && d->hasEnergy())
            return true;

    // Resonator tails ring out after note-off as well — a pure exciter ->
    // resonator -> output patch has no envelope to hold the voice alive,
    // so the tail estimate must carry it (same contract as the delay).
    for (auto* r : resonators)
        if (r != nullptr && r->getInput().isConnected() && r->hasEnergy())
            return true;

    // Feedback loops circulate their energy; the voice lives while a wired
    // loop still carries audio (the amount ceiling guarantees it decays).
    for (auto* fb : feedbackProcessors)
        if (fb != nullptr && fb->hasEnergy())
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
            fm->getAmountModInput().disconnect();
            fm->getVelInput().disconnect();
        }

    for (auto* fscale : frequencyScalers)
        if (fscale != nullptr)  fscale->getFreqInput().disconnect();

        for (auto* adsr : adsrProcessors)
        if (adsr != nullptr)  adsr->getInput().disconnect();

        for (auto* fadsr : fAdsrProcessors)
        if (fadsr != nullptr)
        {
            fadsr->getFreqInput().disconnect();
            fadsr->getVelInput().disconnect();
        }

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
            d->getTimeModInput().disconnect();
            // A new patch must not inherit the previous patch's delay tail:
            // clear the line so no stale signal survives an instrument change.
            d->reset();
        }

        for (auto* h : holdProcessors)
        if (h != nullptr)
        {
            h->getInput().disconnect();
            h->getRateModInput().disconnect();
        }

        // A new patch must not inherit the previous patch's envelopes either:
        // if a note-off was lost (or a held sustain pedal defers it forever),
        // the old ADSR would keep sustaining at its sustain level and drone
        // through the next instrument — the reported "stuck signal" bug.
        for (auto* adsr : adsrProcessors)
        if (adsr != nullptr)  adsr->reset();

        for (auto* fadsr : fAdsrProcessors)
        if (fadsr != nullptr)  fadsr->reset();

        // Filters must not ring over from the previous patch either.
        for (auto* f : filters)
        if (f != nullptr)  f->reset();

        for (auto* g : gainProcessors)
        if (g != nullptr)
        {
            g->getInput().disconnect();
            g->getModInput().disconnect();
            g->getVelInput().disconnect();
        }

        for (auto* ws : waveshapers)
        if (ws != nullptr)
        {
            ws->getInput().disconnect();
            ws->getDriveModInput().disconnect();
            ws->getVelInput().disconnect();
        }

        for (auto* f : filters)
        if (f != nullptr)
        {
            f->getInput().disconnect();
            f->getCutoffModInput().disconnect();
            f->getResonanceModInput().disconnect();
            f->getVelInput().disconnect();
        }

        // Feedback nodes: clear the loop entry/return and the tail estimate
        // so no previous patch's circulation survives an instrument change.
        for (auto* fb : feedbackProcessors)
        if (fb != nullptr)
        {
            fb->getInput().disconnect();
            fb->getFeedbackInput().disconnect();
            fb->reset();
        }

        // Resonators must not ring over from the previous patch either
        // (their state IS the sound — clear the whole tail).
        for (auto* r : resonators)
        if (r != nullptr)
        {
            r->getInput().disconnect();
            r->getTuneInput().disconnect();
            r->getQModInput().disconnect();
            r->getVelInput().disconnect();
            r->getStrikeVelInput().disconnect();
            r->reset();
        }

        for (auto* s : splitters)
        if (s != nullptr)  s->getInput().disconnect();

        for (auto* m : mergers)
        if (m != nullptr)
        {
            m->getLeftInput().disconnect();
            m->getRightInput().disconnect();
        }

        for (auto* a : adders)
        if (a != nullptr)
        {
            a->getInput1().disconnect();
            a->getInput2().disconnect();
        }

    for (int i = 0; i < MasterOutputProcessor::numInputs; ++i)
        masterOutput->getInput (i).disconnect();

    for (auto* note : noteSources)
        if (note != nullptr)  note->setEnabled (false);

    // -- 2. Rewire according to the patch ------------------------------------
    for (const auto& conn : patch.connections)
    {
                OutputPort* out = resolveOutput (conn.from.nodeId, conn.from.portId,
                                                   noteSources, adsrProcessors, fAdsrProcessors, masterOutput,
                                                   oscillators, fmProcessors, frequencyScalers, ringModulators, amModulators, delays, holdProcessors, gainProcessors, waveshapers, filters,
                                                   splitters, mergers, adders, feedbackProcessors, resonators, velocityScalers);
                InputPort*  in  = resolveInput  (conn.to.nodeId,   conn.to.portId,
                                                   const_cast<SynthVoiceParameters&> (parameters),
                                                   velocityScalers,
                                                   noteSources, adsrProcessors, fAdsrProcessors, masterOutput,
                                                   oscillators, fmProcessors, frequencyScalers, ringModulators, amModulators, delays, holdProcessors, gainProcessors, waveshapers, filters,
                                                   splitters, mergers, adders, feedbackProcessors, resonators);

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
