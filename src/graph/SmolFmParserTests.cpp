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
        "      <Node id=\"osc0\" waveform=\"saw\" mode=\"pitch\" staticfreq=\"440\">\n"
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
        CHECK (osc.parameters.size() == 3);

        auto paramOf = [&osc] (const char* key)
        {
            const auto it = osc.parameters.find (key);
            return it != osc.parameters.end() ? it->second : -1.0f;
        };

        CHECK (paramOf ("waveform") == 1.0f
            && paramOf ("mode") == 0.0f
            && paramOf ("staticfreq") == 440.0f);
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
        "      <Node id=\"osc0\" waveform=\"saw\" mode=\"pitch\" staticfreq=\"440\">\n"
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

    if (failures == 0)
        std::cout << "SmolFM_ParserTests: all checks passed\n";

    return failures == 0 ? 0 : 1;
}