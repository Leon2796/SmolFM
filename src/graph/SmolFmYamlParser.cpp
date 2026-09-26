/*
    SmolFmYamlParser - YAML flavour parser for .smolfm files.

    Line-based parser for the subset of YAML described in
    doc/formats/smolfm-yaml-grammar.md: nested mappings plus "- " sequences
    with 2-space indentation.  Block structure comes from the indent, not
    from key names, so a node's `pins:` block cannot swallow the next node.

    Version 3 nests nodes/connections under `graph:` and adds `layout:`
    (boxes: positions) plus enum attribute values as names; legacy flat
    files with numeric values stay readable.
*/

#include "SmolFmYamlParser.h"

namespace smolfm
{

namespace
{
    // Value after the first ':' on a "key: value" line, quotes stripped.
    juce::String valueOf (const juce::String& line)
    {
        auto value = line.fromFirstOccurrenceOf (":", false, false).trim();

        if (value.length() >= 2
            && ((value.startsWithChar ('"')  && value.endsWithChar ('"'))
             || (value.startsWithChar ('\'') && value.endsWithChar ('\''))))
            return value.substring (1, value.length() - 1);

        return value;
    }

    // One "key: value" line onto a node.  Keys mirror the XML attribute
    // names written by SmolFmFile::save.
    void readNodeAttribute (const juce::String& line, NodeDefinition& node)
    {
        const auto value = valueOf (line);
        const auto prefix = baseIdPrefix (node.id);

        if      (line.startsWith ("x:"))          node.x = value.getIntValue();
        else if (line.startsWith ("y:"))          node.y = value.getIntValue();
        else if (line.startsWith ("waveform:"))   node.parameters["waveform"] = enumOrNumber ("waveform", prefix, value);
        else if (line.startsWith ("mode:"))       node.parameters["mode"]     = enumOrNumber ("mode", prefix, value);
        else if (line.startsWith ("shape:"))      node.parameters["shape"]    = enumOrNumber ("shape", prefix, value);
        else if (line.startsWith ("sync:"))       node.parameters["sync"]     = enumOrNumber ("sync", prefix, value);
        else if (line.startsWith ("division:"))   node.parameters["division"] = enumOrNumber ("division", prefix, value);
        else if (line.startsWith ("staticfreq:")) node.parameters["staticfreq"] = value.getFloatValue();
        else if (line.startsWith ("lforate:"))    node.parameters["lforate"]    = value.getFloatValue();
        else if (line.startsWith ("attack:"))     node.parameters["attack"]     = value.getFloatValue();
        else if (line.startsWith ("decay:"))      node.parameters["decay"]      = value.getFloatValue();
        else if (line.startsWith ("sustain:"))    node.parameters["sustain"]    = value.getFloatValue();
        else if (line.startsWith ("release:"))    node.parameters["release"]    = value.getFloatValue();
        else if (line.startsWith ("up:"))         node.parameters["up"]         = value.getFloatValue();
        else if (line.startsWith ("down:"))       node.parameters["down"]       = value.getFloatValue();
        else if (line.startsWith ("factor:"))     node.parameters["factor"]     = value.getFloatValue();
        else if (line.startsWith ("amount:"))     node.parameters["amount"]     = value.getFloatValue();
        else if (line.startsWith ("drive:"))      node.parameters["drive"]      = value.getFloatValue();
        else if (line.startsWith ("cutoff:"))     node.parameters["cutoff"]     = value.getFloatValue();
        else if (line.startsWith ("resonance:"))  node.parameters["resonance"]  = value.getFloatValue();
        else if (line.startsWith ("time:"))       node.parameters["time"]       = value.getFloatValue();
        else if (line.startsWith ("feedback:"))   node.parameters["feedback"]   = value.getFloatValue();
        else if (line.startsWith ("mix:"))        node.parameters["mix"]        = value.getFloatValue();
        else if (line.startsWith ("level:"))      node.parameters["level"]      = value.getFloatValue();
    }
}

bool SmolFmYamlParser::canParse (const juce::File& file)
{
    const auto content = file.loadFileAsString();
    return ! content.contains ("<?xml")
        && (content.contains ("version:") || content.contains ("nodes:"));
}

bool SmolFmYamlParser::parse (const juce::File& file, SmolFmData& data)
{
    const auto content = file.loadFileAsString();
    if (content.trim().isEmpty())
        return false;

    const juce::StringArray lines = juce::StringArray::fromLines (content);

    enum class Section { root, nodes, connections, layout, boxes };
    Section section = Section::root;

    NodeDefinition node;
    PinDefinition pin;
    ConnectionDefinition wire;
    BoxDefinition box;
    bool inNode = false;
    bool inPins = false;
    bool inBoxes = false;
    int pinIndent = -1;             // indent of the "- id:" pin lines

    auto flushPin = [&]
    {
        if (inPins && pin.id.isNotEmpty())
            node.pins.push_back (pin);
        pin = PinDefinition();
        inPins = false;
    };
    auto flushNode = [&]
    {
        flushPin();
        if (inNode && node.id.isNotEmpty())
            data.nodes.push_back (node);
        node = NodeDefinition();
        inNode = false;
    };
    auto flushWire = [&]
    {
        if (wire.from.isNotEmpty() && wire.fromPort.isNotEmpty()
         && wire.to.isNotEmpty()   && wire.toPort.isNotEmpty())
            data.connections.push_back (wire);
        wire = ConnectionDefinition();
    };
    auto flushBox = [&]
    {
        if (inBoxes && box.id.isNotEmpty())
            data.boxes.push_back (box);
        box = BoxDefinition();
        inBoxes = false;
    };

    for (const auto& raw : lines)
    {
        const juce::String trimmed = raw.trimStart();
        const int indent = raw.length() - trimmed.length();
        const juce::String line = trimmed.trimEnd();

        if (line.isEmpty() || line.startsWith ("#"))
            continue;

        // Section headers.  Version 3 nests nodes/connections under
        // `graph:` and adds `layout:` with box positions; legacy files keep
        // nodes/connections at the top level.  `graph:` and `layout:` are
        // just markers for this line state machine.
        if (line == "graph:")
        {
            flushNode();
            flushWire();
            flushBox();
            continue;
        }
        if (line == "layout:")
        {
            flushNode();
            flushWire();
            flushBox();
            section = Section::layout;
            continue;
        }
        if (line == "boxes:" && section == Section::layout)
        {
            section = Section::boxes;
            inBoxes = true;
            continue;
        }
        if (line == "nodes:")
        {
            flushNode();
            flushBox();
            section = Section::nodes;
            continue;
        }
        if (line == "connections:")
        {
            flushNode();
            flushBox();
            flushWire();
            data.hasConnections = true;
            section = Section::connections;
            continue;
        }

        if (section == Section::root)
        {
            if (line.startsWith ("version:"))
                data.version = valueOf (line);
            else if (line.startsWith ("name:"))
                data.name = valueOf (line);

            continue;
        }

        if (section == Section::nodes)
        {
            if (line.startsWith ("- id:"))
            {
                if (inPins && (pinIndent == -1 || indent >= pinIndent))
                {
                    // Another (or the first) pin of the current node.
                    flushPin();
                    pin.id = valueOf (line);
                    pinIndent = indent;
                    inPins = true;
                    continue;
                }

                // A new node; a pin list never reaches this indent.
                flushNode();
                node.id = valueOf (line);
                inNode = true;
                continue;
            }

            if (inNode && line == "pins:")
            {
                flushPin();
                inPins = true;
                pinIndent = -1;
                continue;
            }

            if (inPins)
            {
                if      (line.startsWith ("direction:")) pin.direction = valueOf (line);
                else if (line.startsWith ("type:"))      pin.type      = valueOf (line);

                continue;
            }

            if (inNode)
                readNodeAttribute (line, node);

            continue;
        }

        if (section == Section::boxes)
        {
            if (line.startsWith ("- id:"))
            {
                flushBox();
                box.id = valueOf (line);
                inBoxes = true;
                continue;
            }

            if (inBoxes)
            {
                if      (line.startsWith ("x:")) box.x = valueOf (line).getIntValue();
                else if (line.startsWith ("y:")) box.y = valueOf (line).getIntValue();

                continue;
            }

            continue;
        }

        // section == connections
        if (line.startsWith ("- from:"))
        {
            flushWire();
            wire.from = valueOf (line);
            continue;
        }
        if (wire.from.isNotEmpty())
        {
            if      (line.startsWith ("fromPort:")) wire.fromPort = valueOf (line);
            else if (line.startsWith ("toPort:")) { wire.toPort = valueOf (line); flushWire(); }
            else if (line.startsWith ("to:"))       wire.to       = valueOf (line);
        }
    }

    flushNode();
    flushWire();
    flushBox();

    // Presentation positions attach to their nodes; a layout entry wins
    // over any inline x/y (relevant for files carrying both spellings).
    for (auto& n : data.nodes)
        for (const auto& b : data.boxes)
            if (b.id == n.id)
            {
                n.x = b.x;
                n.y = b.y;
                break;
            }

    return true;
}

} // namespace smolfm



