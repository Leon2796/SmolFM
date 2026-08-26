/*
    DraggablePanel hosts DraggableComponent boxes in a free-form canvas and
    visualises the wiring between them.

    Connections:
        - Click an output pin and drag to an input pin to create a wire.
        - Click an input pin that already has a wire to remove it.
        - Wires are drawn as cubic curves, colour-coded by port type.
        - Layout AND wiring are persisted via juce::PropertiesFile.

    The panel never touches the audio thread itself.  Wire changes are
    broadcast via `onConnectionPatchChanged` (std::function) so the
    processor can pick them up.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>

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

    // -- Introspection for .smolfm save/load --------------------------------

    /** All box ids currently on the canvas, in insertion order. */
    juce::StringArray getBoxIds() const;

    /** Current bounds of one box (empty rect when the id is unknown). */
    juce::Rectangle<int> getBoxBounds (const juce::String& boxId) const;

    /** Move a box to a new top-left position (clamped into the panel). */
    void setBoxPosition (const juce::String& boxId, juce::Point<int> pos);

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

    /** Insert a single wire if types are compatible. */
    bool tryConnect (const smolfm::ConnectionPatch::Endpoint& from,
                     const smolfm::ConnectionPatch::Endpoint& to);

    /** Remove the wire that ends at (nodeId, portId). */
    bool removeConnectionTo (const smolfm::ConnectionPatch::Endpoint& to);

    /** True when an input pin already has a wire. */
    bool isConnected (const smolfm::ConnectionPatch::Endpoint& to) const noexcept;

    /** External notification hook (PluginEditor wires this to the processor). */
    std::function<void (const smolfm::ConnectionPatch&)> onConnectionPatchChanged;

    void paintOverChildren (juce::Graphics& g) override;
    void resized() override;

private:
    juce::PropertiesFile& getPropertiesFile();
    void layoutBoxes();

    /** Returns true when the two endpoint types match. */
    static bool typesMatch (const smolfm::ConnectionPatch::Endpoint& from,
                            const smolfm::ConnectionPatch::Endpoint& to);

    juce::OwnedArray<DraggableComponent> boxes;
    std::unique_ptr<juce::PropertiesFile> propertiesFile;

    smolfm::ConnectionPatch currentPatch;

    // Drag state for an in-flight wire.
    bool draggingWire = false;
    smolfm::ConnectionPatch::Endpoint draggedFrom;
    juce::Point<float> draggedToPosition;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggablePanel)
};

} // namespace gui
