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
#include <juce_audio_processors/juce_audio_processors.h>

#include "../processors/Processor.h"

namespace gui
{

class PinComponent;

class DraggableComponent final : public juce::Component,
                                 private juce::Button::Listener
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
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    /** Fired when the little × in the title bar is clicked.  The panel
        removes the box via this callback. */
    std::function<void (DraggableComponent&)> onCloseRequested;

    /** Stable identifier used for persistence. */
    const juce::String& getBoxId() const noexcept { return boxId; }

    /**
        Wire an optional bypass parameter.

        Shows a small bypass icon at the LEFT edge of the title bar (mirroring
        the close × at the right) and, while bypassed, dims the whole node
        with a translucent overlay.  An empty parameter id leaves the node
        untouched — node types without a bypass option look exactly as before.
    */
    void setBypassParameter (juce::AudioProcessorValueTreeState& apvts,
                             const juce::String& bypassParameterId);

    // -- Multi-select ------------------------------------------------------
    // Marked via double-click on the title bar or the panel's rubber-band
    // rectangle; marked boxes move as a group when one of them is dragged.

    /** True when the box carries the multi-select marker. */
    bool isSelected() const noexcept { return selected; }

    /** Set the marker; repaints only on change. */
    void setSelected (bool shouldBeSelected);

    /** Preferred size of the box (content + title bar). */
    juce::Rectangle<int> getPreferredSize() const noexcept;

    //-- Pin management ------------------------------------------------------

    /**
        Add an input pin at the left edge.

        Index controls vertical stacking; the component repositions pins in
        resized().
    */
    void addInputPin  (const juce::String& pinId, smolfm::PortType type,
                       smolfm::ChannelSide sideHint = smolfm::ChannelSide::none);
    void addOutputPin (const juce::String& pinId, smolfm::PortType type,
                       smolfm::ChannelSide sideHint = smolfm::ChannelSide::none);

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

    void paintOverChildren (juce::Graphics& g) override;
    void buttonStateChanged (juce::Button* button) override;
    void buttonClicked (juce::Button*) override {} // clicks sync via ButtonAttachment

    juce::String boxId;
    juce::String title;
    std::unique_ptr<juce::Component> content;
    juce::TextButton closeButton { "close" };

    // Optional bypass wiring (see setBypassParameter): title-bar icon plus
    // dimmed overlay.  A null button means "node type without bypass option".
    std::unique_ptr<juce::Button> bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::OwnedArray<PinComponent> inputPins;
    juce::OwnedArray<PinComponent> outputPins;

    // Drag state recorded at mouseDown.  draggingFromTitleBar is false until
    // the user presses the title bar; the mouseDrag handler bails out early
    // otherwise so child controls keep their normal drag behaviour.
    bool draggingFromTitleBar = false;
    juce::Point<int> dragStartMouse;
    juce::Point<int> dragStartPosition;

    bool selected = false;   ///< multi-select marker (see isSelected/setSelected)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggableComponent)
};

} // namespace gui
