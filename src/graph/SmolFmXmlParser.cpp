/*
    SmolFmXmlParser - XML flavour parser for .smolfm files.

    Reads the vocabulary that SmolFmFile::save writes (see parametersForNode
    there) in both spellings: version 3 splits the document into a semantic
    <Graph> (processors + wiring, enum attributes as names) and a
    presentation <Layout> (canvas boxes), while the flat legacy structure
    with numeric values stays readable.  The result is a format-independent
    SmolFmData tree; applying it to the editor is SmolFmFile's job.
*/

#include "SmolFmXmlParser.h"

#include <map>
#include <memory>

namespace smolfm
{

namespace
{
    // Attribute names written by SmolFmFile::save.  Int-valued keys arrive
    // as "1" etc.; getDoubleAttribute reads both spellings.
    const char* knownAttributes[] =
    {
        "waveform", "mode", "shape", "sync", "division",
        "staticfreq", "lforate", "attack", "decay", "sustain", "release",
        "up", "down", "factor", "amount", "drive", "cutoff", "resonance",
        "time", "feedback", "mix", "level"
    };

    void readAttributes (const juce::XmlElement& element, const juce::String& nodeId,
                         std::map<juce::String, float>& out)
    {
        const juce::String prefix = baseIdPrefix (nodeId);

        for (int i = 0; i < element.getNumAttributes(); ++i)
        {
            const juce::String name = element.getAttributeName (i);

            for (auto* known : knownAttributes)
                if (name == known)
                {
                    out[name] = enumOrNumber (name, prefix, element.getStringAttribute (name));
                    break;
                }
        }
    }
}

bool SmolFmXmlParser::canParse (const juce::File& file)
{
    // Content sniffing on the canonical root marker.  The XML declaration is
    // what SmolFmFile::save writes, but XML itself makes it optional and the
    // hand-written instruments (fm-bell.smolfm and the whole instruments/
    // corpus) omit it.  Demanding "<?xml" here made createParser() fall
    // through to the YAML sniff (which needs "version:"/"nodes:"), so
    // SmolFmFile::load silently refused every hand-written file before any
    // parsing happened.
    auto content = file.loadFileAsString();
    return content.contains ("<SmolFM");
}

bool SmolFmXmlParser::parse (const juce::File& file, SmolFmData& data)
{
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));

    if (xml == nullptr || xml->getTagName() != "SmolFM")
        return false;

    data.version = xml->getStringAttribute ("version", "2");
    data.name    = xml->getStringAttribute ("name", "");

    // Version 3 wraps the semantic part in <Graph> and puts positions into
    // <Layout>; version 2 keeps the flat legacy structure.  Both read.
    const auto* graphElement = xml->getChildByName ("Graph");
    auto* nodesElement = graphElement != nullptr
        ? graphElement->getChildByName ("Nodes")
        : xml->getChildByName ("Nodes");

    if (nodesElement != nullptr)
    {
        for (auto* nodeElement = nodesElement->getFirstChildElement();
             nodeElement != nullptr;
             nodeElement = nodeElement->getNextElement())
        {
            if (! nodeElement->hasTagName ("Node"))
                continue;

            NodeDefinition node;
            node.id = nodeElement->getStringAttribute ("id");
            node.x  = nodeElement->getIntAttribute ("x", -1);
            node.y  = nodeElement->getIntAttribute ("y", -1);
            readAttributes (*nodeElement, node.id, node.parameters);

            for (auto* pinElement = nodeElement->getFirstChildElement();
                 pinElement != nullptr;
                 pinElement = pinElement->getNextElement())
            {
                if (! pinElement->hasTagName ("Pin"))
                    continue;

                PinDefinition pin;
                pin.id        = pinElement->getStringAttribute ("id");
                pin.direction = pinElement->getStringAttribute ("direction", "in");
                pin.type      = pinElement->getStringAttribute ("type", "signal");
                node.pins.push_back (pin);
            }

            if (node.id.isNotEmpty())
                data.nodes.push_back (node);
        }
    }

    auto* connectionsElement = graphElement != nullptr
        ? graphElement->getChildByName ("Connections")
        : xml->getChildByName ("Connections");

    // An empty <Connections/> still means "replace the wiring with nothing".
    data.hasConnections = connectionsElement != nullptr;

    if (connectionsElement != nullptr)
    {
        for (auto* wireElement = connectionsElement->getFirstChildElement();
             wireElement != nullptr;
             wireElement = wireElement->getNextElement())
        {
            if (! wireElement->hasTagName ("Wire"))
                continue;

            ConnectionDefinition conn;
            conn.from     = wireElement->getStringAttribute ("from");
            conn.fromPort = wireElement->getStringAttribute ("fromPort");
            conn.to       = wireElement->getStringAttribute ("to");
            conn.toPort   = wireElement->getStringAttribute ("toPort");

            if (conn.from.isNotEmpty() && conn.fromPort.isNotEmpty()
             && conn.to.isNotEmpty()   && conn.toPort.isNotEmpty())
                data.connections.push_back (conn);
        }
    }

    // Presentation part (version 3): canvas positions.  Layout entries
    // override any node-local x/y so the writer's split always wins.
    if (auto* layoutElement = xml->getChildByName ("Layout"))
        if (auto* boxesElement = layoutElement->getChildByName ("Boxes"))
            for (auto* boxElement = boxesElement->getFirstChildElement();
                 boxElement != nullptr;
                 boxElement = boxElement->getNextElement())
            {
                if (! boxElement->hasTagName ("Box"))
                    continue;

                BoxDefinition box;
                box.id = boxElement->getStringAttribute ("id");
                box.x  = boxElement->getIntAttribute ("x", -1);
                box.y  = boxElement->getIntAttribute ("y", -1);

                if (box.id.isNotEmpty())
                    data.boxes.push_back (box);
            }

    for (auto& node : data.nodes)
        for (const auto& box : data.boxes)
            if (box.id == node.id)
            {
                node.x = box.x;
                node.y = box.y;
                break;
            }

    return true;
}

} // namespace smolfm
