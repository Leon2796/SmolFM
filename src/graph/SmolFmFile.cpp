/*
    SmolFmFile implementation.

    Each node owns its parameters — the XML mirrors what the UI shows:

    <SmolFM version="2">
      <Nodes>
        <Node id="carrier" x="16" y="16" frequency="440" waveform="0">
          <Pin id="note_in" direction="in"  type="frequency"/>
          <Pin id="out"     direction="out" type="signal"/>
        </Node>
        <Node id="fm" x="336" y="16" amount="0">
          ...
        </Node>
        <Node id="adsr" x="16" y="200" attack="0.01" decay="0.2" sustain="0.8" release="0.5">
          ...
        </Node>
        ...
      </Nodes>
      <Connections>
        <Wire from="note" fromPort="out" to="fm" toPort="freq_in"/>
        ...
      </Connections>
    </SmolFM>

    Loading writes the parameters back into the APVTS, which pushes them into
    every slider attachment and every voice on the next sample.
*/

#include "SmolFmFile.h"
#include "SmolFmXmlParser.h"
#include "SmolFmYamlParser.h"
#include "../gui/DraggablePanel.h"
#include "../gui/DraggableComponent.h"
#include "../gui/PinComponent.h"

namespace smolfm
{

namespace
{
    const char* typeToString (PortType t)
    {
        return t == PortType::frequency ? "frequency" : "signal";
    }

    // Which APVTS parameter ids belong to a concrete instance ("osc3", "fm1").
    // The XML attribute names stay short ("frequency" / "waveform" / "amount");
    // the instance index is taken from the node id.
    struct NodeParameterSpec { juce::String attribute; juce::String parameterId; };

    juce::Array<NodeParameterSpec> parametersForNode (const juce::String& nodeId)
    {
        juce::Array<NodeParameterSpec> specs;

        const juce::String baseId = GraphNodeRegistry::baseIdOf (nodeId);

        if (baseId == "osc")
        {
            const auto wfmId   = GraphNodeRegistry::waveformParameterIdFor   (nodeId);
            const auto modeId  = GraphNodeRegistry::oscModeParameterIdFor    (nodeId);
            const auto statId  = GraphNodeRegistry::oscStaticFreqParameterIdFor (nodeId);
            const auto rateId  = GraphNodeRegistry::oscLfoRateParameterIdFor  (nodeId);

            if (wfmId.isNotEmpty())  specs.add ({ "waveform",  wfmId });
            if (modeId.isNotEmpty()) specs.add ({ "mode",      modeId });
            if (statId.isNotEmpty()) specs.add ({ "staticfreq", statId });
            if (rateId.isNotEmpty()) specs.add ({ "lforate",   rateId });
        }
                else if (baseId == "fm" || baseId == "am")
                {
                    const auto amtId = GraphNodeRegistry::amountParameterIdFor (nodeId);
                    if (amtId.isNotEmpty()) specs.add ({ "amount", amtId });
                }
                else if (baseId == "delay")
                {
                    const auto timeId = GraphNodeRegistry::delayTimeParameterIdFor (nodeId);
                    const auto fbId   = GraphNodeRegistry::delayFeedbackParameterIdFor (nodeId);
                    const auto mixId  = GraphNodeRegistry::delayMixParameterIdFor (nodeId);
                    const auto syncId = GraphNodeRegistry::delaySyncParameterIdFor (nodeId);
                    const auto divId  = GraphNodeRegistry::delayDivisionParameterIdFor (nodeId);
                    if (timeId.isNotEmpty()) specs.add ({ "time",      timeId });
                    if (fbId  .isNotEmpty()) specs.add ({ "feedback",  fbId });
                    if (mixId .isNotEmpty()) specs.add ({ "mix",       mixId });
                    if (syncId.isNotEmpty()) specs.add ({ "sync",      syncId });
                    if (divId .isNotEmpty()) specs.add ({ "division",  divId });
                }
        else if (baseId == "fscale")
        {
            const auto factorId = GraphNodeRegistry::amountParameterIdFor (nodeId);
            if (factorId.isNotEmpty()) specs.add ({ "factor", factorId });
        }
        else if (baseId == "adsr")
        {
            for (const char* which : { "Attack", "Decay", "Sustain", "Release" })
            {
                const auto id = GraphNodeRegistry::adsrParameterIdFor (nodeId, which);
                if (id.isNotEmpty()) specs.add ({ juce::String (which).toLowerCase(), id });
            }
        }
        else if (baseId == "fadsr")
        {
            for (const char* which : { "Attack", "Decay", "Sustain", "Release", "Up", "Down" })
            {
                const auto id = GraphNodeRegistry::fAdsrParameterIdFor (nodeId, which);
                if (id.isNotEmpty()) specs.add ({ juce::String (which).toLowerCase(), id });
            }
        }
        else if (baseId == "gain")
        {
            const auto factorId = GraphNodeRegistry::gainParameterIdFor (nodeId);
            if (factorId.isNotEmpty()) specs.add ({ "factor", factorId });
        }
        else if (baseId == "shape")
        {
            const auto driveId = GraphNodeRegistry::driveParameterIdFor (nodeId);
            const auto shapeId = GraphNodeRegistry::shapeParameterIdFor (nodeId);
            if (driveId.isNotEmpty()) specs.add ({ "drive", driveId });
            if (shapeId.isNotEmpty()) specs.add ({ "shape", shapeId });
        }
        else if (baseId == "filter")
        {
            const auto cutoffId = GraphNodeRegistry::filterCutoffParameterIdFor    (nodeId);
            const auto resId    = GraphNodeRegistry::filterResonanceParameterIdFor (nodeId);
            const auto modeId   = GraphNodeRegistry::filterModeParameterIdFor      (nodeId);
            if (cutoffId.isNotEmpty()) specs.add ({ "cutoff", cutoffId });
            if (resId.isNotEmpty())    specs.add ({ "resonance", resId });
            if (modeId.isNotEmpty())   specs.add ({ "mode", modeId });
        }
        else if (baseId == "output")
        {
            const auto lvlId = GraphNodeRegistry::levelParameterIdFor (nodeId);
            if (lvlId.isNotEmpty()) specs.add ({ "level", lvlId });
        }

        return specs;
    }

    float getParameter (juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
    {
        const std::atomic<float>* raw = apvts.getRawParameterValue (id);
        return raw != nullptr ? raw->load() : 0.0f;
    }

    void setParameter (juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& id, float value)
    {
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id)))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    }

    float parameterDefault (juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
    {
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id)))
            return param->getDefaultValue();
        return 0.0f;
    }

    // -- Format detection ----------------------------------------------------
    // The .smolfm format has an XML and a YAML flavour (doc/formats/);
    // content sniffing picks the parser, XML wins on its canonical marker.
    std::unique_ptr<ISmolFmParser> createParser (const juce::File& file)
    {
        if (auto xml = std::make_unique<SmolFmXmlParser>(); xml->canParse (file))
            return xml;

        if (auto yaml = std::make_unique<SmolFmYamlParser>(); yaml->canParse (file))
            return yaml;

        return nullptr;
    }

    bool parseFile (const juce::File& file, SmolFmData& data)
    {
        auto parser = createParser (file);
        return parser != nullptr && parser->parse (file, data);
    }
}

bool SmolFmFile::save (gui::DraggablePanel& panel,
                       juce::AudioProcessorValueTreeState& apvts,
                       const juce::File& file)
{
    // Version 3 splits the document: <Graph> carries the semantic part
    // (processors, their sound parameters and the wiring), <Layout> the
    // presentation (canvas boxes; future UI settings extend this).  Enum
    // parameters are written as their names (saw, static, quarter, ...).
    juce::XmlElement root ("SmolFM");
    root.setAttribute ("version", 3);
    root.setAttribute ("name", file.getFileNameWithoutExtension());

    auto* graph = root.createNewChildElement ("Graph");
    auto* nodes = graph->createNewChildElement ("Nodes");
    auto* boxes = root.createNewChildElement ("Layout")->createNewChildElement ("Boxes");

    for (const juce::String& boxId : panel.getBoxIds())
    {
        const auto bounds = panel.getBoxBounds (boxId);

        auto* nodeXml = nodes->createNewChildElement ("Node");
        nodeXml->setAttribute ("id", boxId);

        const juce::String baseId = GraphNodeRegistry::baseIdOf (boxId);

        // The node bundles its own sound-defining parameters (enum values
        // as their names, everything else as numbers).
        for (const auto& p : parametersForNode (boxId))
            nodeXml->setAttribute (p.attribute,
                                  enumValueText (p.attribute, baseId, getParameter (apvts, p.parameterId)));

        if (const NodeSpec* spec = GraphNodeRegistry::findSpec (boxId))
        {
            for (const juce::String& pinId : spec->inputPortIds)
            {
                auto* pinXml = nodeXml->createNewChildElement ("Pin");
                pinXml->setAttribute ("id", pinId);
                pinXml->setAttribute ("direction", "in");
                pinXml->setAttribute ("type", typeToString (GraphNodeRegistry::findPortInfo (boxId, pinId, false).second));
            }
            if (spec->outputPortId.isNotEmpty())
            {
                auto* pinXml = nodeXml->createNewChildElement ("Pin");
                pinXml->setAttribute ("id", spec->outputPortId);
                pinXml->setAttribute ("direction", "out");
                pinXml->setAttribute ("type", typeToString (spec->outputType));
            }
        }

        // Presentation: the canvas position lives in the layout part only.
        auto* boxXml = boxes->createNewChildElement ("Box");
        boxXml->setAttribute ("id", boxId);
        boxXml->setAttribute ("x", bounds.getX());
        boxXml->setAttribute ("y", bounds.getY());
    }

    auto* wires = graph->createNewChildElement ("Connections");

    for (const auto& c : panel.getCurrentPatch().connections)
    {
        auto* wire = wires->createNewChildElement ("Wire");
        wire->setAttribute ("from",     c.from.nodeId);
        wire->setAttribute ("fromPort", c.from.portId);
        wire->setAttribute ("to",       c.to.nodeId);
        wire->setAttribute ("toPort",   c.to.portId);
    }

    return root.writeTo (file.withFileExtension (".smolfm"));
}

bool SmolFmFile::load (gui::DraggablePanel& panel,
                       juce::AudioProcessorValueTreeState& apvts,
                       const juce::File& file)
{
    SmolFmData data;
    if (! parseFile (file, data) || ! data.isValid())
        return false;

    bool appliedAnything = false;

    for (const auto& node : data.nodes)
    {
        const juce::String& id = node.id;

        // If this node isn't on the canvas, ask the editor to create it.
        // panel owns a factory hook that knows how to build the content
        // for an instance.  PluginEditor configures it at startup.
        if (panel.getBoxIds().contains (id) == false
            && panel.onCreateMissingNode != nullptr)
        {
            panel.onCreateMissingNode (id);
        }

        // Position (arrangement).
        if (node.x >= 0 && node.y >= 0)
        {
            panel.setBoxPosition (id, { node.x, node.y });
            appliedAnything = true;
        }

            // Parameters (sound) live on the node itself.  Attributes the
            // file does not carry are RESET to the parameter's declared
            // default instead of keeping the previous patch's value —
            // without this, loading a patch that omits (e.g.) the osc mode
            // inherits Static from an earlier drum-patch load.
            for (const auto& p : parametersForNode (id))
            {
                const auto it = node.parameters.find (p.attribute);
                const float value = it != node.parameters.end()
                    ? it->second
                    : parameterDefault (apvts, p.parameterId);

                setParameter (apvts, p.parameterId, value);
                appliedAnything = true;
            }
    }

    // Rebuild the wiring and push it to the panel (which notifies the processor).
    // A file without a Connections section keeps the current wiring; a file
    // with the section (even empty) replaces it. save() always writes it.
    if (data.hasConnections)
    {
        ConnectionPatch patch;

        for (const auto& c : data.connections)
        {
            ConnectionPatch::Connection conn;
            conn.from.nodeId = c.from;
            conn.from.portId = c.fromPort;
            conn.to.nodeId   = c.to;
            conn.to.portId   = c.toPort;
            patch.connections.push_back (conn);
        }

        // One wire per input pin: later wires in the file replace earlier
        // ones onto the same input, mirroring tryConnect()'s rule.  Without
        // this, a file with duplicate fan-in draws two wires while the voices
        // only wire the last one — the earlier source looks live but is dead.
        {
            std::map<juce::String, std::size_t> inputToIndex;
            std::vector<ConnectionPatch::Connection> deduped;

            for (const auto& c : patch.connections)
            {
                const juce::String key = c.to.nodeId + ":" + c.to.portId;
                const auto it = inputToIndex.find (key);

                if (it != inputToIndex.end())
                    deduped[it->second] = c;    // replace earlier wire
                else
                {
                    inputToIndex[key] = deduped.size();
                    deduped.push_back (c);
                }
            }

            patch.connections = std::move (deduped);
        }

        panel.applyPatch (patch);

        // Show only nodes that are actively wired; everything else stays
        // hidden to keep the canvas uncluttered.
        panel.updateVisibilityFromConnections();

        appliedAnything = true;
    }

    return appliedAnything;
}

juce::String SmolFmFile::readInstrumentName (const juce::File& file)
{
    // Works for both format flavours (see doc/formats/).
    SmolFmData data;
    if (parseFile (file, data) && data.name.isNotEmpty())
        return data.name;

    return file.getFileNameWithoutExtension();
}

bool SmolFmFile::writeInstrumentName (const juce::File& file, const juce::String& name)
{
    std::unique_ptr<juce::XmlElement> root (juce::XmlDocument::parse (file));

    if (root == nullptr || ! root->hasTagName ("SmolFM"))
        return false;

    root->setAttribute ("name", name);
    return root->writeTo (file);
}

} // namespace smolfm
