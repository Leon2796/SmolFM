/*
    DraggablePanel hosts DraggableComponent boxes in a free-form canvas.

    Each box can be placed anywhere inside the panel.  The panel saves and
    restores box positions via a juce::PropertiesFile so the user's layout
    persists across sessions.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "DraggableComponent.h"

namespace gui
{

class DraggablePanel final : public juce::Component
{
public:
    DraggablePanel();
    ~DraggablePanel() override;

    /** Add a box and adopt its position.  Ownership passes to the panel. */
    DraggableComponent& addComponent (std::unique_ptr<DraggableComponent> box);

    /** Persist all current box positions to disk. */
    void saveLayout();

    /** Restore box positions if a saved layout exists. */
    void loadLayout();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::PropertiesFile& getPropertiesFile();

    juce::OwnedArray<DraggableComponent> boxes;
    std::unique_ptr<juce::PropertiesFile> propertiesFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggablePanel)
};

} // namespace gui
