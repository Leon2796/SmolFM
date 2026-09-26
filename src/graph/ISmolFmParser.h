/*
    ISmolFmParser - common data model + parser interface for .smolfm files.

    The format exists in two flavours (XML and YAML, see doc/formats/).
    A parser turns a file into a SmolFmData tree that knows nothing about
    the editor; SmolFmFile applies it to the panel + APVTS, so parsing and
    parameter loading stay decoupled.
*/

#pragma once

#include <juce_core/juce_core.h>

#include <map>
#include <vector>

namespace smolfm
{

struct PinDefinition
{
    juce::String id;
    juce::String direction;  // "in" or "out"
    juce::String type;       // "frequency" or "signal"
};

struct NodeDefinition
{
    juce::String id;
    int x = -1;             // -1 = no position in the file
    int y = -1;
    std::vector<PinDefinition> pins;
    std::map<juce::String, float> parameters;   // attribute name -> value
};

struct ConnectionDefinition
{
    juce::String from;
    juce::String fromPort;
    juce::String to;
    juce::String toPort;
};

// Presentation part of the file: one box per node on the canvas.
struct BoxDefinition
{
    juce::String id;
    int x = -1;             // -1 = not placed in the file
    int y = -1;
};

struct SmolFmData
{
    juce::String version;
    juce::String name;
    std::vector<NodeDefinition> nodes;
    std::vector<ConnectionDefinition> connections;
    std::vector<BoxDefinition> boxes;   // presentation: canvas positions

    // A Connections section was present (possibly empty).  An empty section
    // still means "replace the wiring with nothing"; a missing section means
    // "keep the current wiring".
    bool hasConnections = false;

    bool isValid() const { return version.isNotEmpty() && ! nodes.empty(); }
};

class ISmolFmParser
{
public:
    virtual ~ISmolFmParser() = default;

    /**
        Parse a .smolfm file (XML or YAML) and fill the data structure.

        @param file the file to parse
        @param data output data structure
        @return true on success, false on parse error
    */
    virtual bool parse (const juce::File& file, SmolFmData& data) = 0;

    /**
        Check if this parser can handle the file format.

        @param file the file to check
        @return true if this parser supports the format
    */
    virtual bool canParse (const juce::File& file) = 0;
};

//==============================================================================
// Enumerations of the semantic part (version 3 files carry the names; the
// readers also accept the numeric legacy spelling).  The names mirror the
// APVTS choice order declared in PluginProcessor::createParameterLayout().
inline juce::StringArray waveformNames ()   { return { "sine", "saw", "square", "triangle", "noise", "perlin", "simplex" }; }
inline juce::StringArray oscModeNames ()    { return { "pitch", "static", "lfo" }; }
inline juce::StringArray filterModeNames () { return { "lp", "bp", "hp", "notch" }; }
inline juce::StringArray shapeNames ()      { return { "soft", "hard", "fold" }; }
inline juce::StringArray syncNames ()       { return { "off", "on" }; }
inline juce::StringArray divisionNames ()   { return { "half", "quarter", "eighth", "sixteenth" }; }

/** Base id ("osc3" -> "osc"): everything before the first digit. */
inline juce::String baseIdPrefix (const juce::String& nodeId)
{
    for (int i = 0; i < nodeId.length(); ++i)
        if (juce::CharacterFunctions::isDigit (nodeId[i]))
            return nodeId.substring (0, i);

    return nodeId;
}

/** Parses an attribute value: enum names first, then the numeric spelling. */
inline float enumOrNumber (const juce::String& key, const juce::String& baseId, const juce::String& text)
{
    const juce::String k = key.toLowerCase();
    const juce::String name = text.trim();

    if (k == "waveform" && baseId == "osc")    { const int i = waveformNames().indexOf (name);    if (i >= 0) return (float) i; }
    if (k == "mode"     && baseId == "osc")    { const int i = oscModeNames().indexOf (name);     if (i >= 0) return (float) i; }
    if (k == "mode"     && baseId == "filter") { const int i = filterModeNames().indexOf (name);  if (i >= 0) return (float) i; }
    if (k == "shape"    && baseId == "shape")  { const int i = shapeNames().indexOf (name);       if (i >= 0) return (float) i; }
    if (k == "sync"     && baseId == "delay")  { const int i = syncNames().indexOf (name);        if (i >= 0) return (float) i; }
    if (k == "division" && baseId == "delay")  { const int i = divisionNames().indexOf (name);    if (i >= 0) return (float) i; }

    return name.getFloatValue();
}

/** Serialised text for a value: the enum name when the key has one, else the number. */
inline juce::String enumValueText (const juce::String& key, const juce::String& baseId, float value)
{
    const int index = (int) value;
    const juce::String k = key.toLowerCase();

    if (k == "waveform" && baseId == "osc")    { const auto& n = waveformNames();   if (index >= 0 && index < n.size()) return n[index]; }
    if (k == "mode"     && baseId == "osc")    { const auto& n = oscModeNames();    if (index >= 0 && index < n.size()) return n[index]; }
    if (k == "mode"     && baseId == "filter") { const auto& n = filterModeNames(); if (index >= 0 && index < n.size()) return n[index]; }
    if (k == "shape"    && baseId == "shape")  { const auto& n = shapeNames();      if (index >= 0 && index < n.size()) return n[index]; }
    if (k == "sync"     && baseId == "delay")  { const auto& n = syncNames();       if (index >= 0 && index < n.size()) return n[index]; }
    if (k == "division" && baseId == "delay")  { const auto& n = divisionNames();   if (index >= 0 && index < n.size()) return n[index]; }

    return juce::String (value);
}

} // namespace smolfm
