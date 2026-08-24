/*
    DraggableComponent wraps a content juce::Component in a small titled box
    that the user can move with the mouse.

    Additionally the box exposes pins on its left / right edge so the parent
    DraggablePanel can draw connections between them.  Pin ids and types come
    from GraphNodeRegistry; the component itself only renders and forwards
    mouse events.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../processors/Processor.h"

namespace gui
{

class PinComponent;

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

    //-- Pin management ------------------------------------------------------

    /**
        Add an input pin at the left edge.

        Index controls vertical stacking; the component repositions pins in
        resized().
    */
    void addInputPin  (const juce::String& pinId, smolfm::PortType type);
    void addOutputPin (const juce::String& pinId, smolfm::PortType type);

    /** Get the pin centre in this component's own coordinates (for drawing). */
    juce::Point<float> getInputPinCentre  (int index) const noexcept;
    juce::Point<float> getOutputPinCentre (int index) const noexcept;

    /** Number of input / output pins. */
    int getNumInputPins()  const noexcept { return inputPins.size(); }
    int getNumOutputPins() const noexcept { return outputPins.size(); }

    /** Access the underlying pin components (used by the panel for hit tests). */
    PinComponent* getInputPin  (int index) noexcept { return index >= 0 && index < getNumInputPins()  ? inputPins[index]  : nullptr; }
    PinComponent* getOutputPin (int index) noexcept { return index >= 0 && index < getNumOutputPins() ? outputPins[index] : nullptr; }

private:
    static constexpr int titleBarHeight = 24;
    static constexpr int contentPadding = 4;
    static constexpr int pinMargin      = 8;

    void layoutPins();

    juce::String boxId;
    juce::String title;
    std::unique_ptr<juce::Component> content;

    juce::OwnedArray<PinComponent> inputPins;
    juce::OwnedArray<PinComponent> outputPins;

    // Drag state recorded at mouseDown.  draggingFromTitleBar is false until
    // the user presses the title bar; the mouseDrag handler bails out early
    // otherwise so child controls keep their normal drag behaviour.
    bool draggingFromTitleBar = false;
    juce::Point<int> dragStartMouse;
    juce::Point<int> dragStartPosition;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggableComponent)
};

} // namespace gui
