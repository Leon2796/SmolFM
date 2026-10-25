/*
    Minimal self-check for the .smolfm parsers (ponytail: one runnable check,
    no framework).  Verifies that the XML and the YAML flavour of the same
    instrument parse into identical SmolFmData and that format detection
    picks the right parser.

    Build & run:
        cmake --build jumake_build --config Debug --target SmolFM_ParserTests
        jumake_build\src\Debug\SmolFM_ParserTests.exe
*/

#include <juce_core/juce_core.h>

#include "SmolFmXmlParser.h"
#include "SmolFmYamlParser.h"

#include <iostream>

using namespace smolfm;

namespace
{
    int failures = 0;

    #define CHECK(cond)                                                      \
        do {                                                                 \
            if (! (cond))                                                    \
            {                                                                \
                ++failures;                                                  \
                std::cerr << "FAIL (line " << __LINE__ << "): " #cond "\n";  \
            }                                                                \
        } while (false)

    juce::File writeTemp (const juce::String& name, const juce::String& content)
    {
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("smolfm_parser_tests");
        dir.createDirectory();
        auto file = dir.getChildFile (name);
        file.replaceWithText (content);
        return file;
    }

    const char* xmlText =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<SmolFM version=\"3\" name=\"Demo\">\n"
        "  <Graph>\n"
        "    <Nodes>\n"
        "      <Node id=\"note\">\n"
        "        <Pin id=\"out\" direction=\"out\" type=\"frequency\"/>\n"
        "      </Node>\n"
        "      <Node id=\"osc0\" waveform=\"saw\" mode=\"pitch\" staticfreq=\"440\" lforate=\"2.0\">\n"
        "        <Pin id=\"note_in\" direction=\"in\" type=\"frequency\"/>\n"
        "        <Pin id=\"out\" direction=\"out\" type=\"signal\"/>\n"
        "      </Node>\n"
        "    </Nodes>\n"
        "    <Connections>\n"
        "      <Wire from=\"note\" fromPort=\"out\" to=\"osc0\" toPort=\"note_in\"/>\n"
        "    </Connections>\n"
        "  </Graph>\n"
        "  <Layout>\n"
        "    <Boxes>\n"
        "      <Box id=\"note\" x=\"60\" y=\"100\"/>\n"
        "      <Box id=\"osc0\" x=\"400\" y=\"100\"/>\n"
        "    </Boxes>\n"
        "  </Layout>\n"
        "</SmolFM>\n";

    const char* yamlText =
        "version: \"3\"\n"
        "name: \"Demo\"\n"
        "graph:\n"
        "  nodes:\n"
        "    - id: \"note\"\n"
        "      pins:\n"
        "        - id: \"out\"\n"
        "          direction: out\n"
        "          type: frequency\n"
        "    - id: \"osc0\"\n"
        "      waveform: saw\n"
        "      mode: pitch\n"
        "      staticfreq: 440\n"
        "      lforate: 2.0\n"
        "      pins:\n"
        "        - id: \"note_in\"\n"
        "          direction: in\n"
        "          type: frequency\n"
        "        - id: \"out\"\n"
        "          direction: out\n"
        "          type: signal\n"
        "  connections:\n"
        "    - from: \"note\"\n"
        "      fromPort: \"out\"\n"
        "      to: \"osc0\"\n"
        "      toPort: \"note_in\"\n"
        "layout:\n"
        "  boxes:\n"
        "    - id: \"note\"\n"
        "      x: 60\n"
        "      y: 100\n"
        "    - id: \"osc0\"\n"
        "      x: 400\n"
        "      y: 100\n";

    void compareParsed (const SmolFmData& d)
    {
        CHECK (d.version == "3");
        CHECK (d.name == "Demo");
        CHECK (d.nodes.size() == 2);
        CHECK (d.hasConnections);

        if (d.nodes.size() != 2)
            return;

        const auto& note = d.nodes[0];
        const auto& osc  = d.nodes[1];

        CHECK (note.id == "note" && note.x == 60 && note.y == 100);
        CHECK (note.parameters.empty());
        CHECK (note.pins.size() == 1
            && note.pins[0].id == "out"
            && note.pins[0].direction == "out"
            && note.pins[0].type == "frequency");

        CHECK (osc.id == "osc0" && osc.x == 400 && osc.y == 100);
        CHECK (osc.parameters.size() == 4);

        auto paramOf = [&osc] (const char* key)
        {
            const auto it = osc.parameters.find (key);
            return it != osc.parameters.end() ? it->second : -1.0f;
        };

        CHECK (paramOf ("waveform") == 1.0f
            && paramOf ("mode") == 0.0f
            && paramOf ("staticfreq") == 440.0f
            && paramOf ("lforate") == 2.0f);
        CHECK (osc.pins.size() == 2
            && osc.pins[0].id == "note_in"
            && osc.pins[1].id == "out");

        CHECK (d.connections.size() == 1);
        if (! d.connections.empty())
        {
            const auto& w = d.connections[0];
            CHECK (w.from == "note" && w.fromPort == "out"
                && w.to == "osc0" && w.toPort == "note_in");
        }

        // Presentation part: box positions attach to the nodes.
        CHECK (d.boxes.size() == 2);
        if (d.boxes.size() == 2)
        {
            CHECK (d.boxes[0].id == "note" && d.boxes[0].x == 60 && d.boxes[0].y == 100);
            CHECK (d.boxes[1].id == "osc0" && d.boxes[1].x == 400 && d.boxes[1].y == 100);
        }
    }
}

int main()
{
    const auto xmlFile  = writeTemp ("sample_xml.smolfm",  xmlText);
    const auto yamlFile = writeTemp ("sample_yaml.smolfm", yamlText);

    SmolFmXmlParser  xmlParser;
    SmolFmYamlParser yamlParser;

    // Format detection cross-check.
    CHECK (xmlParser.canParse (xmlFile));
    CHECK (! xmlParser.canParse (yamlFile));
    CHECK (yamlParser.canParse (yamlFile));
    CHECK (! yamlParser.canParse (xmlFile));

    // Both flavours parse into the same data.
    SmolFmData fromXml;
    CHECK (xmlParser.parse (xmlFile, fromXml));
    compareParsed (fromXml);

    SmolFmData fromYaml;
    CHECK (yamlParser.parse (yamlFile, fromYaml));
    compareParsed (fromYaml);

    // A pins block must not swallow the node that follows it.
    const auto pinsThenNode = writeTemp ("pins_then_node.smolfm",
        "version: \"2\"\n"
        "nodes:\n"
        "  - id: \"a\"\n"
        "    pins:\n"
        "      - id: \"in\"\n"
        "        direction: in\n"
        "        type: signal\n"
        "  - id: \"b\"\n"
        "    x: 5\n"
        "    y: 5\n"
        "connections:\n");

    SmolFmData mixed;
    CHECK (yamlParser.parse (pinsThenNode, mixed));
    CHECK (mixed.nodes.size() == 2);
    if (mixed.nodes.size() == 2)
    {
        CHECK (mixed.nodes[0].id == "a" && mixed.nodes[0].pins.size() == 1);
        CHECK (mixed.nodes[1].id == "b" && mixed.nodes[1].x == 5 && mixed.nodes[1].y == 5);
    }
    CHECK (mixed.hasConnections);

    // An empty connections section is recognised (it clears the wiring on load).
    const auto noWires = writeTemp ("no_wires.smolfm",
        "version: \"2\"\nnodes:\n  - id: \"a\"\n    x: 1\n    y: 1\nconnections:\n");
    SmolFmData emptyWires;
    CHECK (yamlParser.parse (noWires, emptyWires));
    CHECK (emptyWires.hasConnections && emptyWires.connections.empty());

    // Garbage is accepted by neither parser.
    const auto garbage = writeTemp ("garbage.smolfm", "hello world");
    CHECK (! xmlParser.canParse (garbage));
    CHECK (! yamlParser.canParse (garbage));

    // Legacy v2: flat structure with numeric enum values stays readable.
    const auto legacyXml = writeTemp ("legacy_xml.smolfm",
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<SmolFM version=\"2\" name=\"Legacy\">\n"
        "  <Nodes>\n"
        "    <Node id=\"osc0\" x=\"7\" y=\"9\" waveform=\"1\" mode=\"0\" staticfreq=\"220\">\n"
        "      <Pin id=\"out\" direction=\"out\" type=\"signal\"/>\n"
        "    </Node>\n"
        "  </Nodes>\n"
        "  <Connections/>\n"
        "</SmolFM>\n");

    SmolFmData legacy;
    CHECK (xmlParser.parse (legacyXml, legacy));
    CHECK (legacy.version == "2" && legacy.nodes.size() == 1);
    if (! legacy.nodes.empty())
    {
        auto& n = legacy.nodes[0];
        CHECK (n.id == "osc0" && n.x == 7 && n.y == 9);
        CHECK (n.parameters["waveform"] == 1.0f && n.parameters["staticfreq"] == 220.0f);
    }
    CHECK (legacy.hasConnections && legacy.connections.empty());

    // Enum names also parse case-insensitively from the YAML flavour.
    const auto legacyYaml = writeTemp ("legacy_yaml.smolfm",
        "version: \"2\"\n"
        "nodes:\n"
        "  - id: \"osc0\"\n"
        "    x: 3\n"
        "    y: 4\n"
        "    waveform: \"Saw\"\n"
        "    mode: \"static\"\n"
        "connections:\n");
    SmolFmData legacyY;
    CHECK (yamlParser.parse (legacyYaml, legacyY));
    CHECK (legacyY.nodes.size() == 1);
    if (! legacyY.nodes.empty())
    {
        auto& n = legacyY.nodes[0];
        CHECK (n.x == 3 && n.y == 4);
        CHECK (n.parameters["waveform"] == 1.0f
            && n.parameters["mode"] == 1.0f);
    }

    // Regression: hand-written instruments carry no "<?xml" declaration (see
    // fm-bell.smolfm).  Format detection must still pick the XML parser via
    // the <SmolFM root marker and must not fall through to YAML, or the app
    // silently refuses to load the whole instruments corpus.
    const auto bareXml = writeTemp ("bare_xml.smolfm",
        "<SmolFM version=\"3\" name=\"Demo\">\n"
        "  <Graph>\n"
        "    <Nodes>\n"
        "      <Node id=\"note\">\n"
        "        <Pin id=\"out\" direction=\"out\" type=\"frequency\"/>\n"
        "      </Node>\n"
        "      <Node id=\"osc0\" waveform=\"saw\" mode=\"pitch\" staticfreq=\"440\" lforate=\"2.0\">\n"
        "        <Pin id=\"note_in\" direction=\"in\" type=\"frequency\"/>\n"
        "        <Pin id=\"out\" direction=\"out\" type=\"signal\"/>\n"
        "      </Node>\n"
        "    </Nodes>\n"
        "    <Connections>\n"
        "      <Wire from=\"note\" fromPort=\"out\" to=\"osc0\" toPort=\"note_in\"/>\n"
        "    </Connections>\n"
        "  </Graph>\n"
        "  <Layout>\n"
        "    <Boxes>\n"
        "      <Box id=\"note\" x=\"60\" y=\"100\"/>\n"
        "      <Box id=\"osc0\" x=\"400\" y=\"100\"/>\n"
        "    </Boxes>\n"
        "  </Layout>\n"
        "</SmolFM>\n");
    CHECK (xmlParser.canParse (bareXml));
    CHECK (! yamlParser.canParse (bareXml));
    SmolFmData fromBareXml;
    CHECK (xmlParser.parse (bareXml, fromBareXml));
    compareParsed (fromBareXml);

    // The migrated instruments/*.smolfm files are version 3 documents: the
    // same parser stack the synthesizer uses must load them all.  The folder
    // is located via the executable path (repo checkout layout) and the
    // check skips silently when it is absent.
    auto instrumentsDir = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                              .getParentDirectory()
                              .getParentDirectory()
                              .getParentDirectory()
                              .getChildFile ("instruments");

    if (instrumentsDir.isDirectory())
    {
        int loaded = 0;
        for (const auto& instrument : instrumentsDir.findChildFiles (juce::File::findFiles, true, "*.smolfm"))
        {
            SmolFmData instrumentData;
            CHECK (xmlParser.canParse (instrument));
            CHECK (xmlParser.parse (instrument, instrumentData));
            CHECK (instrumentData.isValid());
            ++loaded;
        }
        CHECK (loaded > 0);
    }

    // fscale interval enum: the writer emits names, the reader accepts names
    // (case-insensitive) and the numeric legacy spelling.  Index order must
    // match the APVTS choices in PluginProcessor::createParameterLayout().
    CHECK (enumOrNumber ("interval", "fscale", "octave_up") == 1.0f);
    CHECK (enumOrNumber ("interval", "fscale", "Fifth_up") == 3.0f);
    CHECK (enumOrNumber ("interval", "fscale", "3") == 3.0f);
    CHECK (enumValueText ("interval", "fscale", 3.0f) == "fifth_up");
    CHECK (enumValueText ("interval", "fscale", 0.0f) == "none");

    // The XML flavour parses the v3 attribute spelling into the enum index;
    // the fscale factor still loads alongside it.
    const auto fscaleXml = writeTemp ("fscale_interval.smolfm",
        "<SmolFM version=\"3\" name=\"Demo\">\n"
        "  <Graph>\n"
        "    <Nodes>\n"
        "      <Node id=\"fscale0\" factor=\"1.5\" interval=\"octave_up\"/>\n"
        "    </Nodes>\n"
        "  </Graph>\n"
        "</SmolFM>\n");
    SmolFmData fscaleData;
    CHECK (xmlParser.parse (fscaleXml, fscaleData));
    CHECK (fscaleData.nodes.size() == 1);
    if (fscaleData.nodes.size() == 1)
    {
        auto& n = fscaleData.nodes[0];
        CHECK (n.parameters["interval"] == 1.0f);
        CHECK (n.parameters["factor"] == 1.5f);
    }

    // resonator/feedback nodes: the new node types and their attributes
    // round-trip through both flavours' whitelists.
    const auto resXml = writeTemp ("resonator_feedback.smolfm",
        "<SmolFM version=\"3\" name=\"ResDemo\">\n"
        "  <Graph>\n"
        "    <Nodes>\n"
        "      <Node id=\"resonator0\" tune=\"49.5\" q=\"0.8\" decay=\"0.35\" spread=\"2\" strikelevel=\"0.5\" striketime=\"0.01\"/>\n"
        "      <Node id=\"feedback0\" amount=\"0.42\"/>\n"
        "    </Nodes>\n"
        "  </Graph>\n"
        "</SmolFM>\n");
    const auto resYaml = writeTemp ("resonator_feedback.yaml.smolfm",
        "version: \"3\"\n"
        "name: \"ResDemo\"\n"
        "graph:\n"
        "  nodes:\n"
        "    - id: \"resonator0\"\n"
        "      tune: 49.5\n"
        "      q: 0.8\n"
        "      decay: 0.35\n"
        "      spread: 2\n"
        "      strikelevel: 0.5\n"
        "      striketime: 0.01\n"
        "    - id: \"feedback0\"\n"
        "      amount: 0.42\n");
    SmolFmData resData;
    CHECK (xmlParser.parse (resXml, resData) && resData.nodes.size() == 2);
    SmolFmData resYData;
    CHECK (yamlParser.parse (resYaml, resYData) && resYData.nodes.size() == 2);
    if (resData.nodes.size() == 2)
    {
        auto& r = resData.nodes[0];
        CHECK (r.parameters["tune"] == 49.5f);
        CHECK (r.parameters["q"] == 0.8f);
        CHECK (r.parameters["decay"] == 0.35f);
        CHECK (r.parameters["spread"] == 2.0f);
        CHECK (r.parameters["strikelevel"] == 0.5f);
        CHECK (r.parameters["striketime"] == 0.01f);
    }
    if (resYData.nodes.size() == 2)
    {
        auto& f = resYData.nodes[1];
        CHECK (f.id == "feedback0" && f.parameters["amount"] == 0.42f);
    }

    // vsc: the curve enum parses by name (both flavours), the scale float
    // loads alongside it — mirrors the resonator/feedback round-trip checks.
    CHECK (enumOrNumber ("curve", "vsc", "sqrt") == 1.0f);
    CHECK (enumOrNumber ("curve", "vsc", "linear") == 0.0f);
    CHECK (enumOrNumber ("curve", "vsc", "Square") == 2.0f);
    CHECK (enumValueText ("curve", "vsc", 2.0f) == "square");

    const auto vscXml = writeTemp ("vsc_node.smolfm",
        "<SmolFM version=\"3\" name=\"VscDemo\">\n"
        "  <Graph>\n"
        "    <Nodes>\n"
        "      <Node id=\"vsc0\" scale=\"1.5\" curve=\"sqrt\"/>\n"
        "    </Nodes>\n"
        "  </Graph>\n"
        "</SmolFM>\n");
    const auto vscYaml = writeTemp ("vsc_node.yaml.smolfm",
        "version: \"3\"\n"
        "name: \"VscDemo\"\n"
        "graph:\n"
        "  nodes:\n"
        "    - id: \"vsc0\"\n"
        "      curve: sqrt\n"
        "      scale: 1.5\n");
    SmolFmData vscData;
    CHECK (xmlParser.parse (vscXml, vscData) && vscData.nodes.size() == 1);
    SmolFmData vscYData;
    CHECK (yamlParser.parse (vscYaml, vscYData) && vscYData.nodes.size() == 1);
    if (vscData.nodes.size() == 1)
    {
        auto& n = vscData.nodes[0];
        CHECK (n.parameters["curve"] == 1.0f);
        CHECK (n.parameters["scale"] == 1.5f);
    }
    if (vscYData.nodes.size() == 1)
    {
        auto& y = vscYData.nodes[0];
        CHECK (y.parameters["curve"] == 1.0f);
        CHECK (y.parameters["scale"] == 1.5f);
    }

    // fadsr: the up/down frequency multipliers parse through both flavours'
    // whitelists alongside the env times (mirrors the resonator strike keys).
    const auto fadsrXml = writeTemp ("fadsr_node.smolfm",
        "<SmolFM version=\"3\" name=\"FadsrDemo\">\n"
        "  <Graph>\n"
        "    <Nodes>\n"
        "      <Node id=\"fadsr0\" attack=\"0.01\" decay=\"0.2\" sustain=\"0.7\" release=\"0.4\" up=\"3\" down=\"0.5\"/>\n"
        "    </Nodes>\n"
        "  </Graph>\n"
        "</SmolFM>\n");
    const auto fadsrYaml = writeTemp ("fadsr_node.yaml.smolfm",
        "version: \"3\"\n"
        "name: \"FadsrDemo\"\n"
        "graph:\n"
        "  nodes:\n"
        "    - id: \"fadsr0\"\n"
        "      attack: 0.01\n"
        "      decay: 0.2\n"
        "      sustain: 0.7\n"
        "      release: 0.4\n"
        "      up: 3\n"
        "      down: 0.5\n");
    SmolFmData fadsrData;
    CHECK (xmlParser.parse (fadsrXml, fadsrData) && fadsrData.nodes.size() == 1);
    SmolFmData fadsrYData;
    CHECK (yamlParser.parse (fadsrYaml, fadsrYData) && fadsrYData.nodes.size() == 1);
    if (fadsrData.nodes.size() == 1 && fadsrYData.nodes.size() == 1)
    {
        auto& x = fadsrData.nodes[0];
        auto& y = fadsrYData.nodes[0];
        CHECK (x.parameters["up"] == 3.0f && y.parameters["up"] == 3.0f);
        CHECK (x.parameters["down"] == 0.5f && y.parameters["down"] == 0.5f);
        CHECK (x.parameters["release"] == 0.4f && y.parameters["release"] == 0.4f);
    }

    // fadsr up/down accept the interval NAME as the factor: the name maps to
    // its exact 12-TET ratio (never the choice index), e.g. octave_up = 2 and
    // fifth_down = 0.667420 (2^(-7/12)); unknown names fall back to the
    // numeric parse.  The writer stays numeric (enumValueText has no fadsr
    // branch), so saving a name-picked value yields "2", not "octave_up".
    const auto fadsrNamesXml = writeTemp ("fadsr_names.smolfm",
        "<SmolFM version=\"3\" name=\"FadsrNames\">\n"
        "  <Graph>\n"
        "    <Nodes>\n"
        "      <Node id=\"fadsr0\" up=\"octave_up\" down=\"fifth_down\"/>\n"
        "    </Nodes>\n"
        "  </Graph>\n"
        "</SmolFM>\n");
    const auto fadsrNamesYaml = writeTemp ("fadsr_names.yaml.smolfm",
        "version: \"3\"\n"
        "name: \"FadsrNames\"\n"
        "graph:\n"
        "  nodes:\n"
        "    - id: \"fadsr0\"\n"
        "      up: octave_up\n"
        "      down: fifth_down\n");
    SmolFmData fadsrNamesData;
    CHECK (xmlParser.parse (fadsrNamesXml, fadsrNamesData) && fadsrNamesData.nodes.size() == 1);
    SmolFmData fadsrNamesYData;
    CHECK (yamlParser.parse (fadsrNamesYaml, fadsrNamesYData) && fadsrNamesYData.nodes.size() == 1);
    if (fadsrNamesData.nodes.size() == 1 && fadsrNamesYData.nodes.size() == 1)
    {
        auto& fx = fadsrNamesData.nodes[0];
        auto& fy = fadsrNamesYData.nodes[0];
        CHECK (fx.parameters["up"]   == 2.0f      && fy.parameters["up"]   == 2.0f);
        CHECK (fx.parameters["down"] == 0.667420f && fy.parameters["down"] == 0.667420f);
    }
    CHECK (enumOrNumber ("up",   "fadsr", "none")   == 1.0f);  // no shift
    CHECK (enumOrNumber ("down", "fadsr", "fourth_down") == fscaleIntervalRatioFromIndex (6));
    CHECK (enumOrNumber ("up",   "fadsr", "bogus") == 0.0f);  // falls back to numeric parse
    CHECK (enumValueText ("up",   "fadsr", 2.0f) == "2");   // writer stays numeric

    // hold: rate float + sync/division enums round-trip through both
    // flavours' whitelists (mirrors the resonator/feedback checks).
    CHECK (enumOrNumber ("sync", "hold", "on") == 1.0f);
    CHECK (enumOrNumber ("sync", "hold", "off") == 0.0f);
    CHECK (enumValueText ("division", "hold", 2.0f) == "eighth");

    const auto holdXml = writeTemp ("hold_node.smolfm",
        "<SmolFM version=\"3\" name=\"HoldDemo\">\n"
        "  <Graph>\n"
        "    <Nodes>\n"
        "      <Node id=\"hold0\" rate=\"8000\" sync=\"off\" division=\"quarter\"/>\n"
        "    </Nodes>\n"
        "  </Graph>\n"
        "</SmolFM>\n");
    const auto holdYaml = writeTemp ("hold_node.yaml.smolfm",
        "version: \"3\"\n"
        "name: \"HoldDemo\"\n"
        "graph:\n"
        "  nodes:\n"
        "    - id: \"hold0\"\n"
        "      rate: 8000\n"
        "      sync: off\n"
        "      division: quarter\n");
    SmolFmData holdData;
    CHECK (xmlParser.parse (holdXml, holdData) && holdData.nodes.size() == 1);
    SmolFmData holdYData;
    CHECK (yamlParser.parse (holdYaml, holdYData) && holdYData.nodes.size() == 1);
    if (holdData.nodes.size() == 1)
    {
        auto& n = holdData.nodes[0];
        CHECK (n.parameters["rate"] == 8000.0f);
        CHECK (n.parameters["sync"] == 0.0f);
        CHECK (n.parameters["division"] == 1.0f);
    }
    if (holdYData.nodes.size() == 1)
    {
        auto& h = holdYData.nodes[0];
        CHECK (h.parameters["sync"] == 0.0f);
        CHECK (h.parameters["division"] == 1.0f);
    }

    // bypass (gain/shape/filter/adsr/fadsr/delay): plain 0/1 number, written
    // by SmolFmFile::save as a number (enumValueText has no bypass branch);
    // both flavours' whitelists accept it.
    CHECK (enumValueText ("bypass", "gain", 1.0f) == "1");   // writer stays numeric
    CHECK (enumValueText ("bypass", "delay", 0.0f) == "0");
    const auto bypassXml = writeTemp ("bypass_node.smolfm",
        "<SmolFM version=\"3\" name=\"BypassDemo\">\n"
        "  <Graph>\n"
        "    <Nodes>\n"
        "      <Node id=\"gain0\" factor=\"2\" bypass=\"1\"/>\n"
        "      <Node id=\"delay0\" time=\"250\" bypass=\"0\"/>\n"
        "    </Nodes>\n"
        "  </Graph>\n"
        "</SmolFM>\n");
    const auto bypassYaml = writeTemp ("bypass_node.yaml.smolfm",
        "version: \"3\"\n"
        "name: \"BypassDemo\"\n"
        "graph:\n"
        "  nodes:\n"
        "    - id: \"gain0\"\n"
        "      factor: 2\n"
        "      bypass: 1\n"
        "    - id: \"delay0\"\n"
        "      time: 250\n"
        "      bypass: 1\n");
    SmolFmData bypassXData;
    CHECK (xmlParser.parse (bypassXml, bypassXData) && bypassXData.nodes.size() == 2);
    SmolFmData bypassYData;
    CHECK (yamlParser.parse (bypassYaml, bypassYData) && bypassYData.nodes.size() == 2);
    if (bypassXData.nodes.size() == 2)
    {
        CHECK (bypassXData.nodes[0].parameters["bypass"] == 1.0f);
        CHECK (bypassXData.nodes[1].parameters["bypass"] == 0.0f);
    }
    if (bypassYData.nodes.size() == 2)
        CHECK (bypassYData.nodes[0].parameters["bypass"] == 1.0f);

    if (failures == 0)
        std::cout << "SmolFM_ParserTests: all checks passed\n";

    return failures == 0 ? 0 : 1;
}
