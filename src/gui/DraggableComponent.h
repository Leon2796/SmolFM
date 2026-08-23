/*
    DraggableComponent wraps a content juce::Component in a small titled box
    that the user can move with the mouse.

    The box draws a label bar at the top and a frame around the content.  The
    content is laid out below the bar.  Dragging anywhere in the title bar
    moves the whole box.

    Each box carries a stable String ID so the DraggablePanel can save and
    restore its position in a PropertiesFile.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{

class DraggableComponent final : public juce::Component
{
public:
    /**
        Wrap a content component in a draggable box.

        The wrapper takes ownership of `content` (via std::unique_ptr) and
        shows it inside the box, below the title bar.
    */
    DraggableComponent (const juce::String& boxId,
                        const juce::String& title,
                        std::unique_ptr<juce::Component> content);

    ~DraggableComponent() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

    /** Stable identifier used for persistence. */
    const juce::String& getBoxId() const noexcept { return boxId; }

    /** Preferred size of the box (content + title bar). */
    juce::Rectangle<int> getPreferredSize() const noexcept;

private:
    static constexpr int titleBarHeight = 24;
    static constexpr int contentPadding = 4;

    juce::String boxId;
    juce::String title;
    std::unique_ptr<juce::Component> content;

    // Drag state recorded at mouseDown.  draggingFromTitleBar is false until
    // the user presses the title bar; the mouseDrag handler bails out early
    // otherwise so child controls keep their normal drag behaviour.
    bool draggingFromTitleBar = false;
    juce::Point<int> dragStartMouse;
    juce::Point<int> dragStartPosition;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggableComponent)
};

} // namespace gui
