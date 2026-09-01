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
            fm.inputPortIds = { "freq_in", "modulator_in" };
            fm.inputTypes = { PortType::frequency, PortType::signal };
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
    if (base == "adsr")   return NodeType::adsr;
    if (base == "output") return NodeType::masterOutput;
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
        case NodeType::adsr:           return maxAdsr;
        case NodeType::masterOutput:   return maxMasterOutputs;
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

juce::String GraphNodeRegistry::levelParameterIdFor (const juce::String& nodeId)
{
    const NodeSpec* spec = findSpec (baseIdOf (nodeId));
    if (spec == nullptr || spec->levelParameterTemplate.isEmpty())
        return {};

    return withIndex (spec->levelParameterTemplate, indexOf (nodeId));
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
