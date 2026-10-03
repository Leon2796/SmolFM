/*

    GraphNodes implementation.



    The set of node types is static, but each type may exist several times on

    the canvas.  Parameter ids carry the instance index: "osc3Frequency" is

    the APVTS id for the frequency of oscillator node "osc3".

*/



#include "GraphNodes.h"



namespace smolfm

{



namespace

{

    std::vector<NodeSpec> buildSpecs()

    {

        std::vector<NodeSpec> specs;



        {

            NodeSpec note;

            note.id = "note";

            note.title = "Note In";

            note.outputPortId = "out";

            note.outputType = PortType::frequency;

            note.extraOutputPortIds = { "vel" };

            note.extraOutputTypes = { PortType::velocity };

            note.inputPortIds = {};

            specs.push_back (note);

        }

        {

            NodeSpec osc;

            osc.id = "osc";

            osc.title = "Oscillator";

            osc.outputPortId = "out";

            osc.outputType = PortType::signal;

            osc.inputPortIds = { "note_in" };

            osc.inputTypes = { PortType::frequency };

            osc.waveformParameterTemplate  = "osc%Waveform";

            specs.push_back (osc);

        }

        {

            NodeSpec fm;

            fm.id = "fm";

            fm.title = "FM Amount";

            fm.outputPortId = "out";

            fm.outputType = PortType::frequency;

            fm.inputPortIds = { "freq_in", "modulator_in", "amount_mod_in", "vel" };

            fm.inputTypes = { PortType::frequency, PortType::signal, PortType::signal, PortType::velocity };

            fm.amountParameterTemplate = "fmAmount%";

            specs.push_back (fm);

        }

        {

            NodeSpec fscale;

            fscale.id = "fscale";

            fscale.title = "Freq Scale";

            fscale.outputPortId = "out";

            fscale.outputType = PortType::frequency;

            fscale.inputPortIds = { "freq_in" };

            fscale.inputTypes = { PortType::frequency };

            // Must match the APVTS id pattern "fscale<N>Factor" used by

            // createParameterLayout() and SynthVoice.

            fscale.amountParameterTemplate = "fscale%Factor";

            specs.push_back (fscale);

        }

        {

            NodeSpec vsc;

            vsc.id = "vsc";

            vsc.title = "Vel Scale";

            vsc.outputPortId = "out";

            vsc.outputType = PortType::velocity;

            vsc.inputPortIds = { "vel_in" };

            vsc.inputTypes = { PortType::velocity };

            // Must match the APVTS id pattern "vsc<N>Scale" used by
            // createParameterLayout() and SynthVoice.  The curve choice
            // shares this template's prefix via the replace-trick
            // ("vsc%Scale" -> "vsc<N>Curve"), like the fscale interval.
            vsc.amountParameterTemplate = "vsc%Scale";

            specs.push_back (vsc);

        }

                {
            NodeSpec ring;
            ring.id = "ring";
            ring.title = "Ring Mod";
            ring.outputPortId = "out";
            ring.outputType = PortType::signal;
            ring.inputPortIds = { "in1", "in2" };
            ring.inputTypes = { PortType::signal, PortType::signal };
            specs.push_back (ring);
        }
                {
            NodeSpec am;
            am.id = "am";
            am.title = "AM";
            am.outputPortId = "out";
            am.outputType = PortType::signal;
            am.inputPortIds = { "carrier_in", "modulator_in" };
            am.inputTypes = { PortType::signal, PortType::signal };
            am.amountParameterTemplate = "am%Amount";
            specs.push_back (am);
        }
        {
            NodeSpec delay;
            delay.id = "delay";
            delay.title = "Delay";
            delay.outputPortId = "out";
            delay.outputType = PortType::signal;
            delay.inputPortIds = { "in", "time_mod_in" };
            delay.inputTypes = { PortType::signal, PortType::signal };
            delay.timeParameterTemplate     = "delay%TimeMs";
            delay.feedbackParameterTemplate = "delay%Feedback";
            delay.mixParameterTemplate      = "delay%Mix";
            delay.syncParameterTemplate     = "delay%SyncMode";
            delay.divisionParameterTemplate = "delay%Division";
            specs.push_back (delay);
        }
        {
            NodeSpec adsr;

            adsr.id = "adsr";

            adsr.title = "ADSR";

            adsr.outputPortId = "out";

            adsr.outputType = PortType::signal;

            adsr.inputPortIds = { "in" };

            adsr.inputTypes = { PortType::signal };

            adsr.adsrParameterTemplate = "adsr%";

            specs.push_back (adsr);

        }

        {
            NodeSpec fadsr;

            fadsr.id = "fadsr";

            fadsr.title = "F-ADSR";

            fadsr.outputPortId = "out";

            fadsr.outputType = PortType::frequency;

            fadsr.inputPortIds = { "freq_in", "vel" };

            fadsr.inputTypes = { PortType::frequency, PortType::velocity };

            fadsr.fAdsrParameterTemplate = "fadsr%";

            specs.push_back (fadsr);

        }

        {
            NodeSpec gain;

            gain.id = "gain";

            gain.title = "Gain";

            gain.outputPortId = "out";

            gain.outputType = PortType::signal;

            gain.inputPortIds = { "in", "factor_mod_in", "vel" };

            gain.inputTypes = { PortType::signal, PortType::signal, PortType::velocity };

            // Must match the APVTS id pattern "gain<N>Factor" used by
            // createParameterLayout(), SynthVoice and the .smolfm loaders —
            // a drift here leaves the knob unattached (shows 0, drags no audio).
            gain.gainParameterTemplate = "gain%Factor";

            specs.push_back (gain);

        }

        {
            NodeSpec shaper;

            shaper.id = "shape";

            shaper.title = "Waveshaper";

            shaper.outputPortId = "out";

            shaper.outputType = PortType::signal;

            shaper.inputPortIds = { "in", "drive_mod_in", "vel" };

            shaper.inputTypes = { PortType::signal, PortType::signal, PortType::velocity };

            shaper.driveParameterTemplate = "shape%";

            specs.push_back (shaper);

        }

        {
            NodeSpec filt;

            filt.id = "filter";

            filt.title = "Filter";

            filt.outputPortId = "out";

            filt.outputType = PortType::signal;

            filt.inputPortIds = { "in", "cutoff_mod_in", "resonance_mod_in", "vel" };

            filt.inputTypes = { PortType::signal, PortType::frequency, PortType::signal, PortType::velocity };

            filt.cutoffParameterTemplate = "filter%Cutoff";

            filt.resonanceParameterTemplate = "filter%Resonance";

            specs.push_back (filt);

        }

        {

            NodeSpec resonator;

            resonator.id = "resonator";

            resonator.title = "Resonator";

            resonator.outputPortId = "out";

            resonator.outputType = PortType::signal;

            resonator.inputPortIds = { "in", "tune", "q_mod_in", "vel" };

            resonator.inputTypes = { PortType::signal, PortType::frequency, PortType::signal, PortType::velocity };

            resonator.tuneParameterTemplate  = "resonator%Tune";

            resonator.qParameterTemplate     = "resonator%Q";

            resonator.decayParameterTemplate = "resonator%Decay";

            specs.push_back (resonator);

        }

        {

            NodeSpec fb;

            fb.id = "feedback";

            fb.title = "Feedback";

            fb.outputPortId = "out";

            fb.outputType = PortType::signal;

            fb.inputPortIds = { "in", "fb_in" };

            fb.inputTypes = { PortType::signal, PortType::signal };

            fb.amountParameterTemplate = "feedback%Amount";

            specs.push_back (fb);

        }

        {

            NodeSpec out;

            out.id = "output";

            out.title = "Output";

            out.outputPortId = "";

            out.outputType = PortType::signal;

            out.inputPortIds = { "in1", "in2", "in3", "in4", "in5", "in6", "in7", "in8" };

            out.inputTypes = std::vector<PortType> (8, PortType::signal);

            out.levelParameterTemplate = "masterLevel";

            specs.push_back (out);

        }



        {
            NodeSpec split;
            split.id = "split";
            split.title = "Splitter";
            split.outputPortId = "outL";
            split.outputType = PortType::signal;
            split.extraOutputPortIds = { "outR" };
            split.outputChannelSides = { ChannelSide::left, ChannelSide::right };
            split.inputPortIds = { "in" };
            split.inputTypes = { PortType::signal };
            specs.push_back (split);
        }
        {
            NodeSpec merge;
            merge.id = "merge";
            merge.title = "Merger";
            merge.outputPortId = "out";
            merge.outputType = PortType::signal;
            merge.inputPortIds = { "in1", "in2" };
            merge.inputTypes = { PortType::signal, PortType::signal };
            merge.inputChannelSides = { ChannelSide::left, ChannelSide::right };
            specs.push_back (merge);
        }

        {
            NodeSpec add;
            add.id = "add";
            add.title = "Add";
            add.outputPortId = "out";
            add.outputType = PortType::signal;
            add.inputPortIds = { "in1", "in2" };
            add.inputTypes = { PortType::signal, PortType::signal };
            specs.push_back (add);
        }

        return specs;

    }



    juce::String withIndex (juce::String t, int index)

    {

        return t.replace ("%", juce::String (index));

    }

}



//==============================================================================

const std::vector<NodeSpec>& GraphNodeRegistry::getAllSpecs()

{

    static const std::vector<NodeSpec> specs = buildSpecs();

    return specs;

}



const NodeSpec* GraphNodeRegistry::findSpec (const juce::String& baseId)

{

    for (const auto& spec : getAllSpecs())

        if (spec.id == baseId)

            return &spec;



    return nullptr;

}



NodeType GraphNodeRegistry::typeOf (const juce::String& nodeId)

{

    const juce::String base = baseIdOf (nodeId);

    if (base == "note")   return NodeType::note;

    if (base == "osc")    return NodeType::oscillator;

    if (base == "fm")     return NodeType::fmAmount;

    if (base == "fscale") return NodeType::frequencyScale;
    if (base == "vsc")    return NodeType::velocityScale;

        if (base == "ring")    return NodeType::ringModulator;
    if (base == "am")      return NodeType::amModulator;
    if (base == "add")    return NodeType::adder;
    if (base == "delay")   return NodeType::delay;
    if (base == "adsr")   return NodeType::adsr;

    if (base == "fadsr") return NodeType::fAdsr;

    if (base == "gain")  return NodeType::gain;

    if (base == "shape") return NodeType::waveshaper;

    if (base == "filter") return NodeType::filter;

    if (base == "resonator") return NodeType::resonator;
    if (base == "feedback") return NodeType::feedback;

    if (base == "output") return NodeType::masterOutput;

    if (base == "split")  return NodeType::splitter;
    if (base == "merge")  return NodeType::merger;

    return NodeType::unknown;

}



juce::String GraphNodeRegistry::baseIdOf (const juce::String& nodeId)

{

    // Strip trailing digits: "osc3" -> "osc", "note" -> "note".

    int end = nodeId.length();

    while (end > 0 && juce::CharacterFunctions::isDigit (nodeId[end - 1]))

        --end;



    return nodeId.substring (0, end);

}



int GraphNodeRegistry::indexOf (const juce::String& nodeId)

{

    int end = nodeId.length();

    while (end > 0 && juce::CharacterFunctions::isDigit (nodeId[end - 1]))

        --end;



    if (end == nodeId.length())

        return 0;   // no trailing digits: single-instance node



    return nodeId.substring (end).getIntValue();

}



juce::String GraphNodeRegistry::makeInstanceId (const juce::String& baseId, int index)

{

    const int max = maxInstancesOf (typeOf (baseId));



    if (max == 1)

        return baseId;              // "adsr", "output" need no index



    return baseId + juce::String (index);

}



int GraphNodeRegistry::maxInstancesOf (NodeType type)

{

    switch (type)

    {

        case NodeType::note:           return maxNotes;

        case NodeType::oscillator:     return maxOscillators;

        case NodeType::fmAmount:       return maxFmAmounts;

        case NodeType::frequencyScale: return maxFrequencyScales;
        case NodeType::velocityScale:  return maxVelocityScales;

        case NodeType::ringModulator:  return maxRingModulators;
        case NodeType::amModulator:    return maxAmModulators;
        case NodeType::adder:          return maxAdds;
        case NodeType::delay:          return maxDelays;
        case NodeType::adsr:           return maxAdsr;
        case NodeType::fAdsr:          return maxFAdsr;
        case NodeType::gain:           return maxGains;
        case NodeType::waveshaper:     return maxWaveshapers;
        case NodeType::filter:         return maxFilters;

        case NodeType::resonator:      return maxResonators;
        case NodeType::feedback:       return maxFeedbacks;

        case NodeType::masterOutput:   return maxMasterOutputs;

        case NodeType::splitter:       return maxSplitters;
        case NodeType::merger:         return maxMergers;

        case NodeType::unknown:        break;

    }

    return 0;

}



juce::String GraphNodeRegistry::frequencyParameterIdFor (const juce::String& nodeId)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr || spec->frequencyParameterTemplate.isEmpty())

        return {};



    return withIndex (spec->frequencyParameterTemplate, indexOf (nodeId));

}



juce::String GraphNodeRegistry::waveformParameterIdFor (const juce::String& nodeId)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr || spec->waveformParameterTemplate.isEmpty())

        return {};



    return withIndex (spec->waveformParameterTemplate, indexOf (nodeId));

}



juce::String GraphNodeRegistry::oscModeParameterIdFor (const juce::String& nodeId)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr || spec->waveformParameterTemplate.isEmpty())

        return {};



    return withIndex (spec->waveformParameterTemplate, indexOf (nodeId)).replace ("Waveform", "Mode");

}



juce::String GraphNodeRegistry::oscStaticFreqParameterIdFor (const juce::String& nodeId)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr || spec->waveformParameterTemplate.isEmpty())

        return {};



    return withIndex (spec->waveformParameterTemplate, indexOf (nodeId)).replace ("Waveform", "StaticFreq");

}



juce::String GraphNodeRegistry::oscLfoRateParameterIdFor (const juce::String& nodeId)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr || spec->waveformParameterTemplate.isEmpty())

        return {};



    return withIndex (spec->waveformParameterTemplate, indexOf (nodeId)).replace ("Waveform", "LfoRate");

}



juce::String GraphNodeRegistry::amountParameterIdFor (const juce::String& nodeId)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr || spec->amountParameterTemplate.isEmpty())

        return {};



    return withIndex (spec->amountParameterTemplate, indexOf (nodeId));

}



juce::String GraphNodeRegistry::adsrParameterIdFor (const juce::String& nodeId, const juce::String& which)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr || spec->adsrParameterTemplate.isEmpty())

        return {};



    return withIndex (spec->adsrParameterTemplate, indexOf (nodeId)) + which;

}



juce::String GraphNodeRegistry::fAdsrParameterIdFor (const juce::String& nodeId, const juce::String& which)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr || spec->fAdsrParameterTemplate.isEmpty())

        return {};



    return withIndex (spec->fAdsrParameterTemplate, indexOf (nodeId)) + which;

}



juce::String GraphNodeRegistry::gainParameterIdFor (const juce::String& nodeId)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr || spec->gainParameterTemplate.isEmpty())

        return {};



    return withIndex (spec->gainParameterTemplate, indexOf (nodeId));

}



juce::String GraphNodeRegistry::driveParameterIdFor (const juce::String& nodeId)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr || spec->driveParameterTemplate.isEmpty())

        return {};



    return withIndex (spec->driveParameterTemplate, indexOf (nodeId)) + "Drive";

}



juce::String GraphNodeRegistry::shapeParameterIdFor (const juce::String& nodeId)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr || spec->driveParameterTemplate.isEmpty())

        return {};



    return withIndex (spec->driveParameterTemplate, indexOf (nodeId)) + "Shape";

}



juce::String GraphNodeRegistry::levelParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->levelParameterTemplate.isEmpty())
        return {};

    return withIndex (spec->levelParameterTemplate, indexOf (nodeId));
}

juce::String GraphNodeRegistry::filterCutoffParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->cutoffParameterTemplate.isEmpty())
        return {};

    return withIndex (spec->cutoffParameterTemplate, indexOf (nodeId));
}

juce::String GraphNodeRegistry::filterResonanceParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->resonanceParameterTemplate.isEmpty())
        return {};

    return withIndex (spec->resonanceParameterTemplate, indexOf (nodeId));
}

juce::String GraphNodeRegistry::filterModeParameterIdFor (const juce::String& nodeId)
{
    // Mode shares the cutoff template prefix ("filter%"), suffixed "Mode" —
    // mirrors the waveshaper drive/shape pattern.
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->cutoffParameterTemplate.isEmpty())
        return {};

    return withIndex (spec->cutoffParameterTemplate, indexOf (nodeId)).replace ("Cutoff", "Mode");
}

juce::String GraphNodeRegistry::delayTimeParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->timeParameterTemplate.isEmpty())
        return {};
    return withIndex (spec->timeParameterTemplate, indexOf (nodeId));
}

juce::String GraphNodeRegistry::delayFeedbackParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->feedbackParameterTemplate.isEmpty())
        return {};
    return withIndex (spec->feedbackParameterTemplate, indexOf (nodeId));
}

juce::String GraphNodeRegistry::delayMixParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->mixParameterTemplate.isEmpty())
        return {};
    return withIndex (spec->mixParameterTemplate, indexOf (nodeId));
}

juce::String GraphNodeRegistry::delaySyncParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->syncParameterTemplate.isEmpty())
        return {};
    return withIndex (spec->syncParameterTemplate, indexOf (nodeId));
}

juce::String GraphNodeRegistry::delayDivisionParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->divisionParameterTemplate.isEmpty())
        return {};
    return withIndex (spec->divisionParameterTemplate, indexOf (nodeId));
}

juce::String GraphNodeRegistry::resonatorTuneParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->tuneParameterTemplate.isEmpty())
        return {};
    return withIndex (spec->tuneParameterTemplate, indexOf (nodeId));
}

juce::String GraphNodeRegistry::resonatorQParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->qParameterTemplate.isEmpty())
        return {};
    return withIndex (spec->qParameterTemplate, indexOf (nodeId));
}

juce::String GraphNodeRegistry::resonatorDecayParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->decayParameterTemplate.isEmpty())
        return {};
    return withIndex (spec->decayParameterTemplate, indexOf (nodeId));
}

juce::String GraphNodeRegistry::resonatorVelSpreadParameterIdFor (const juce::String& nodeId)
{
    // VelSpread shares the decay template prefix ("resonator%Decay") via the
    // replace trick — mirrors freqScaleInterval and the osc waveform family.
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->decayParameterTemplate.isEmpty())
        return {};
    return withIndex (spec->decayParameterTemplate, indexOf (nodeId)).replace ("Decay", "VelSpread");
}

juce::String GraphNodeRegistry::freqScaleIntervalParameterIdFor (const juce::String& nodeId)
{
    // Interval shares the amount template prefix ("fscale%Factor"),
    // suffixed "Interval" — mirrors the waveshaper drive/shape and the
    // osc waveform/static/lfo replace pattern.  Only the fscale spec has it.
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->id != "fscale" || spec->amountParameterTemplate.isEmpty())
        return {};

    return withIndex (spec->amountParameterTemplate, indexOf (nodeId)).replace ("Factor", "Interval");
}

juce::String GraphNodeRegistry::velocityScaleCurveParameterIdFor (const juce::String& nodeId)
{
    // Curve shares the amount template prefix ("vsc%Scale") and re-skins it
    // "Scale" -> "Curve" — the same replace-trick as the fscale interval.
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->id != "vsc" || spec->amountParameterTemplate.isEmpty())
        return {};

    return withIndex (spec->amountParameterTemplate, indexOf (nodeId)).replace ("Scale", "Curve");
}



std::pair<bool, PortType> GraphNodeRegistry::findPortInfo (const juce::String& nodeId,

                                                           const juce::String& portId,

                                                           bool isOutput)

{

    const NodeSpec* spec = findSpec (baseIdOf (nodeId));

    if (spec == nullptr)

        return { false, PortType::signal };



    if (isOutput)

    {

        if (spec->outputPortId.isNotEmpty() && spec->outputPortId == portId)

            return { true, spec->outputType };

        // Extra outputs: an entry in extraOutputTypes overrides (note.vel is
        // velocity while note.out is frequency); otherwise the primary
        // output's type applies (splitter outR is signal like outL).

        if (spec->extraOutputPortIds.indexOf (portId) >= 0)

        {

            const int extra = spec->extraOutputPortIds.indexOf (portId);

            const PortType t = extra < static_cast<int> (spec->extraOutputTypes.size())

                                   ? spec->extraOutputTypes[static_cast<size_t> (extra)]

                                   : spec->outputType;

            return { true, t };

        }

        return { false, PortType::signal };

    }



    const int idx = spec->inputPortIds.indexOf (portId);

    if (idx < 0)

        return { false, PortType::signal };



    PortType type = PortType::signal;

    if (idx < static_cast<int> (spec->inputTypes.size()))

        type = spec->inputTypes[idx];



    return { true, type };

}



//==============================================================================

juce::String ConnectionPatchIO::serializeToJson (const ConnectionPatch& patch)

{

    juce::Array<juce::var> wires;

    for (const auto& c : patch.connections)

    {

        auto* entry = new juce::DynamicObject();

        entry->setProperty ("from", c.from.nodeId + "." + c.from.portId);

        entry->setProperty ("to",   c.to.nodeId   + "." + c.to.portId);

        wires.add (juce::var (entry));

    }



    auto* root = new juce::DynamicObject();

    root->setProperty ("connections", wires);

    return juce::JSON::toString (juce::var (root));

}



bool ConnectionPatchIO::parseFromJson (const juce::String& json, ConnectionPatch& outPatch)

{

    outPatch.connections.clear();



    const juce::var doc = juce::JSON::parse (json);

    if (! doc.isObject())

        return false;



    const juce::var wires = doc.getProperty ("connections", juce::var());

    if (! wires.isArray())

        return false;



    auto parseEndpoint = [] (const juce::String& s, ConnectionPatch::Endpoint& out) -> bool

    {

        const int dot = s.indexOfChar ('.');

        if (dot <= 0)

            return false;



        out.nodeId = s.substring (0, dot);

        out.portId = s.substring (dot + 1);

        return out.nodeId.isNotEmpty() && out.portId.isNotEmpty();

    };



    for (const auto& w : *wires.getArray())

    {

        const juce::String from = w.getProperty ("from", juce::String());

        const juce::String to   = w.getProperty ("to",   juce::String());



        ConnectionPatch::Connection conn;

        if (parseEndpoint (from, conn.from) && parseEndpoint (to, conn.to))

            outPatch.connections.push_back (conn);

    }



    return true;

}



} // namespace smolfm


