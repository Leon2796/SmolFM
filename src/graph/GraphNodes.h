/*
    GraphNodes.h declares the per-voice processing graph whose node *set* is
    no longer fixed: the user may add several nodes of the same kind.  Node
    ids carry an instance index so every box on the canvas maps to its own
    processor in the voice pool and its own APVTS parameter pair.

        Node id scheme
            note0 .. note3  up to 4 MIDI note sources (all emit the played note)
            osc0 .. osc7  up to 8 oscillators (carrier or modulator â€” same thing)
            fm0 .. fm3    up to 4 FM amount stages
            adsr          exactly one (final envelope)

    A connection is expressed from the input side, because an InputPort can
    only ever have one source.  FM stages live in the frequency domain and
    chain in Hertz; the oscillator at the end of the chain integrates the
    instantaneous frequency (true FM) and renders the chosen waveform.

        note.out (frequency)   -> fm0.freq_in
        osc1.out (signal)      -> fm0.modulator_in
        fm0.out  (frequency)   -> fm1.freq_in   (chained FM stages)
        fm1.out  (frequency)   -> osc0.note_in
        osc0.out (signal)      -> adsr.in
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include <map>
#include <memory>
#include <vector>
#include <atomic>

#include "../processors/Processor.h"
#include "../processors/ProcessorPort.h"

namespace smolfm
{

//==============================================================================
/**
    Coarse node kind.  Oscillator nodes are identical whether they act as
    carrier or modulator â€” the wiring decides that role.
*/
enum class NodeType
{
    note,
    oscillator,
    fmAmount,
    frequencyScale,
    ringModulator,
    amModulator,
    adsr,
    masterOutput,
    unknown
};

//==============================================================================
/**
    Static description of one node *type* (not instance).

    Instance ids append the index to this base: for type `oscillator` with
    base id "osc", the concrete nodes are "osc0", "osc1", ...
*/
struct NodeSpec
{
    juce::String id;          // base id ("note", "osc", "fm", "adsr")
    juce::String title;       // shown in the box title bar
    juce::String outputPortId;
    juce::StringArray inputPortIds;
    PortType outputType = PortType::signal;
    std::vector<PortType> inputTypes;

    // APVTS parameter ids (unindexed templates: "%" gets the instance index,
    // e.g. "osc%Frequency" -> "osc3Frequency").  Empty for note.
    juce::String frequencyParameterTemplate;
    juce::String waveformParameterTemplate;
    juce::String amountParameterTemplate;
    juce::String adsrParameterTemplate;  // "adsr%Attack" style prefix, only on adsr
    juce::String levelParameterTemplate; // master volume, only on output
};

//==============================================================================
/** In-memory model of the current connection graph (unchanged shape). */
struct ConnectionPatch
{
    struct Endpoint
    {
        juce::String nodeId;
        juce::String portId;

        bool operator== (const Endpoint& other) const noexcept
        {
            return nodeId == other.nodeId && portId == other.portId;
        }
        bool operator!= (const Endpoint& other) const noexcept { return ! operator== (other); }
        bool operator< (const Endpoint& other) const noexcept
        {
            if (nodeId != other.nodeId) return nodeId < other.nodeId;
            return portId < other.portId;
        }
    };

    struct Connection
    {
        Endpoint from;
        Endpoint to;

        bool operator== (const Connection& other) const noexcept { return from == other.from && to == other.to; }
        bool operator!= (const Connection& other) const noexcept { return ! operator== (other); }
    };

    std::vector<Connection> connections;

    bool operator== (const ConnectionPatch& other) const noexcept
    {
        return connections.size() == other.connections.size()
            && std::equal (connections.begin(), connections.end(), other.connections.begin());
    }
    bool operator!= (const ConnectionPatch& other) const noexcept { return ! operator== (other); }
};

//==============================================================================
/**
    Registry: knows the node types, their budgets and how to turn an instance
    id ("osc3") back into (type, index) and the matching APVTS parameter ids.
*/
class GraphNodeRegistry
{
public:
    static constexpr int maxOscillators = 8;
    static constexpr int maxFmAmounts   = 4;
    static constexpr int maxFrequencyScales = 4;
    static constexpr int maxRingModulators  = 4;
    static constexpr int maxAmModulators    = 4;
    static constexpr int maxNotes       = 4;
    static constexpr int maxAdsr        = 4;
    static constexpr int maxMasterOutputs = 1;

    static const std::vector<NodeSpec>& getAllSpecs();

    /** Look up the type-level spec ("osc", "fm", ...). */
    static const NodeSpec* findSpec (const juce::String& baseId);

    // -- Instance id helpers -------------------------------------------------

    static NodeType typeOf (const juce::String& nodeId);
    static int      indexOf (const juce::String& nodeId);
    static juce::String makeInstanceId (const juce::String& baseId, int index);
    static juce::String baseIdOf (const juce::String& nodeId);

    /** How many of this type may exist at once. */
    static int maxInstancesOf (NodeType type);

    // -- Parameter helpers ---------------------------------------------------

    /** Concrete APVTS ids for one instance ("osc3Frequency", "fm1Amount", ...). */
    static juce::String frequencyParameterIdFor (const juce::String& nodeId);
    static juce::String waveformParameterIdFor  (const juce::String& nodeId);
    static juce::String amountParameterIdFor    (const juce::String& nodeId);
    static juce::String adsrParameterIdFor      (const juce::String& nodeId, const juce::String& which);
    static juce::String levelParameterIdFor     (const juce::String& nodeId);

    // -- Port info -----------------------------------------------------------

    static std::pair<bool, PortType> findPortInfo (const juce::String& nodeId,
                                                   const juce::String& portId,
                                                   bool isOutput);
};

//==============================================================================
/** JSON (de)serialisation used by the panel's auto-layout persistence. */
class ConnectionPatchIO
{
public:
    static juce::String serializeToJson (const ConnectionPatch& patch);
    static bool parseFromJson (const juce::String& json, ConnectionPatch& outPatch);
};

} // namespace smolfm
