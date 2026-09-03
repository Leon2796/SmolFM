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
            const auto wfmId  = GraphNodeRegistry::waveformParameterIdFor  (nodeId);

            if (wfmId.isNotEmpty())  specs.add ({ "waveform",  wfmId });
        }
                else if (baseId == "fm" || baseId == "am")
        {
            const auto amtId = GraphNodeRegistry::amountParameterIdFor (nodeId);
            if (amtId.isNotEmpty()) specs.add ({ "amount", amtId });
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
}

bool SmolFmFile::save (gui::DraggablePanel& panel,
                       juce::AudioProcessorValueTreeState& apvts,
                       const juce::File& file)
{
    juce::XmlElement root ("SmolFM");
    root.setAttribute ("version", 2);
    root.setAttribute ("name", file.getFileNameWithoutExtension());

    auto* nodes = root.createNewChildElement ("Nodes");

    for (const juce::String& boxId : panel.getBoxIds())
    {
        const auto bounds = panel.getBoxBounds (boxId);
        auto* nodeXml = nodes->createNewChildElement ("Node");
        nodeXml->setAttribute ("id", boxId);
        nodeXml->setAttribute ("x", bounds.getX());
        nodeXml->setAttribute ("y", bounds.getY());

        // The node bundles its own sound-defining parameters.
        for (const auto& p : parametersForNode (boxId))
            nodeXml->setAttribute (p.attribute, getParameter (apvts, p.parameterId));

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
    }

    auto* wires = root.createNewChildElement ("Connections");

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
    const std::unique_ptr<juce::XmlElement> root (juce::XmlDocument::parse (file));

    if (root == nullptr || ! root->hasTagName ("SmolFM"))
        return false;

    bool appliedAnything = false;

    if (auto* nodes = root->getChildByName ("Nodes"))
    {
        for (auto* nodeXml = nodes->getFirstChildElement();
             nodeXml != nullptr;
             nodeXml = nodeXml->getNextElement())
        {
            if (! nodeXml->hasTagName ("Node"))
                continue;

            const juce::String id = nodeXml->getStringAttribute ("id");

            // If this node isn't on the canvas, ask the editor to create it.
            if (panel.getBoxIds().contains (id) == false)
            {
                // panel owns a factory hook that knows how to build the content
                // for an instance.  PluginEditor configures it at startup.
                if (panel.onCreateMissingNode != nullptr)
                    panel.onCreateMissingNode (id);
            }

            // Position (arrangement).
            const int x = nodeXml->getIntAttribute ("x", -1);
            const int y = nodeXml->getIntAttribute ("y", -1);
            if (x >= 0 && y >= 0)
            {
                panel.setBoxPosition (id, { x, y });
                appliedAnything = true;
            }

            // Parameters (sound) live on the node itself.
            for (const auto& p : parametersForNode (id))
            {
                if (nodeXml->hasAttribute (p.attribute))
                {
                    setParameter (apvts, p.parameterId,
                                  static_cast<float> (nodeXml->getDoubleAttribute (p.attribute)));
                    appliedAnything = true;
                }
            }
        }
    }

    // Rebuild the wiring and push it to the panel (which notifies the processor).
    if (auto* wires = root->getChildByName ("Connections"))
    {
        ConnectionPatch patch;

        for (auto* wireXml = wires->getFirstChildElement();
             wireXml != nullptr;
             wireXml = wireXml->getNextElement())
        {
            if (! wireXml->hasTagName ("Wire"))
                continue;

            ConnectionPatch::Connection c;
            c.from.nodeId = wireXml->getStringAttribute ("from");
            c.from.portId = wireXml->getStringAttribute ("fromPort");
            c.to.nodeId   = wireXml->getStringAttribute ("to");
            c.to.portId   = wireXml->getStringAttribute ("toPort");

            if (c.from.nodeId.isNotEmpty() && c.from.portId.isNotEmpty()
             && c.to.nodeId.isNotEmpty()   && c.to.portId.isNotEmpty())
                patch.connections.push_back (c);
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
    const std::unique_ptr<juce::XmlElement> root (juce::XmlDocument::parse (file));

    if (root != nullptr && root->hasTagName ("SmolFM"))
    {
        const juce::String name = root->getStringAttribute ("name");
        if (name.isNotEmpty())
            return name;
    }

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
