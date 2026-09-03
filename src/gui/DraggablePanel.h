/*
    DraggablePanel hosts DraggableComponent boxes in a free-form canvas and
    visualises the wiring between them.

    Connections:
        - Click an output pin and drag to an input pin to create a wire.
        - Click an input pin that already has a wire to remove it.
        - Wires are drawn as cubic curves, colour-coded by port type.
        - Layout and wiring live exclusively in the loaded .smolfm file.

    The panel never touches the audio thread itself.  Wire changes are
    broadcast via `onConnectionPatchChanged` (std::function) so the
    processor can pick them up.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>

#include "DraggableComponent.h"
#include "PinComponent.h"
#include "../graph/GraphNodes.h"

namespace gui
{

class DraggablePanel final : public juce::Component
{
public:
    DraggablePanel();
    ~DraggablePanel() override;

    /** Add a box and adopt its position.  Ownership passes to the panel. */
    DraggableComponent& addComponent (std::unique_ptr<DraggableComponent> box);

    // -- Dynamic add/remove --------------------------------------------------

    /**
        Add a box for one node TYPE ("osc", "fm", ...) and return its instance
        id (e.g. "osc2").  The content factory receives the instance id and
        the APVTS so it can bind its own sliders.

        Returns nullptr when the budget for this type is exhausted.
    */
        DraggableComponent* addNodeOfType (const juce::String& baseId,
                                       juce::AudioProcessorValueTreeState& apvts,
                                       std::function<std::unique_ptr<juce::Component> (const juce::String& instanceId,
                                                                                       juce::AudioProcessorValueTreeState&)> makeContent,
                                       bool makeVisible = true);

    /** Remove a box (and all its wires) from the canvas. */
    void removeNode (DraggableComponent& box);

    // -- Introspection for .smolfm save/load --------------------------------

    /** All box ids currently on the canvas, in insertion order. */
    juce::StringArray getBoxIds() const;

    /** Current bounds of one box (empty rect when the id is unknown). */
    juce::Rectangle<int> getBoxBounds (const juce::String& boxId) const;

    /** Move a box to a new top-left position (only clamped to >= 0).  The
        editor sizes the window to fit right after a patch load. */
    void setBoxPosition (const juce::String& boxId, juce::Point<int> pos);

    /** Toolbar helpers: how many boxes of this base type ("osc"/"fm") exist. */
    int countBoxesOfType (const juce::String& baseId) const;
    bool hasBox (const juce::String& baseId) const;   // for single-instance types

    /** Persist positions AND wiring to disk. */
    void saveLayout();

    /** Restore positions AND wiring if a saved layout exists. */
    void loadLayout();

    // -- Pin mouse events (forwarded by PinComponent) -----------------------

    void pinMouseDown (PinComponent& pin, const juce::MouseEvent& e);
    void pinMouseDrag (PinComponent& pin, const juce::MouseEvent& e);
    void pinMouseUp   (PinComponent& pin, const juce::MouseEvent& e);

    // -- Wiring --------------------------------------------------------------

    /** Find the box component by id, or nullptr. */
    DraggableComponent* findBox (const juce::String& boxId) noexcept;

    /** Find the input pin for (nodeId, portId) on a box. */
    PinComponent* findInputPin  (const juce::String& nodeId, const juce::String& portId) noexcept;
    PinComponent* findOutputPin (const juce::String& nodeId, const juce::String& portId) noexcept;

    /** Read the current wiring. */
    const smolfm::ConnectionPatch& getCurrentPatch() const noexcept { return currentPatch; }

        /** Replace the wiring entirely (e.g. from a saved patch). */
    void applyPatch (const smolfm::ConnectionPatch& patch);

    /** Hide every box that is not part of the current wiring; show the ones
        that are connected.  Call this after a patch load. */
    void updateVisibilityFromConnections();

    /**
        Bounding box of all boxes plus padding — the size the panel needs to
        show the current layout without clipping.
    */
    juce::Rectangle<int> getContentBounds() const noexcept;

    /** Insert a single wire if types are compatible. */
    bool tryConnect (const smolfm::ConnectionPatch::Endpoint& from,
                     const smolfm::ConnectionPatch::Endpoint& to);

    /** Remove the wire that ends at (nodeId, portId). */
    bool removeConnectionTo (const smolfm::ConnectionPatch::Endpoint& to);

    /** Remove every wire that starts or ends at the given box. */
    void removeAllConnectionsForBox (const juce::String& boxId);

    /** True when an input pin already has a wire. */
    bool isConnected (const smolfm::ConnectionPatch::Endpoint& to) const noexcept;

    /** External notification hook (PluginEditor wires this to the processor). */
    std::function<void (const smolfm::ConnectionPatch&)> onConnectionPatchChanged;

    /** Fired when a box is added or removed (for toolbar badges). */
    std::function<void()> onNodeSetChanged;

    /**
        Used by SmolFmFile::load to re-create a box that exists in the XML
        but isn't on the canvas.  Returns the new box or nullptr.  Set by
        PluginEditor at construction.
    */
    std::function<DraggableComponent* (const juce::String& instanceId)> onCreateMissingNode;

    void paintOverChildren (juce::Graphics& g) override;
    void resized() override;

private:
    void layoutBoxes();

    /** Returns true when the two endpoint types match. */
    static bool typesMatch (const smolfm::ConnectionPatch::Endpoint& from,
                            const smolfm::ConnectionPatch::Endpoint& to);

    juce::OwnedArray<DraggableComponent> boxes;

    smolfm::ConnectionPatch currentPatch;

    // Drag state for an in-flight wire.
    bool draggingWire = false;
    smolfm::ConnectionPatch::Endpoint draggedFrom;
    juce::Point<float> draggedToPosition;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggablePanel)
};

} // namespace gui
