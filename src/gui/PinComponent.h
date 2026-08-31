/*
    PinComponent renders a single input or output port on the edge of a
    DraggableComponent.

    The pin is the smallest possible unit in the graph editor: a 12-px circle
    at the side of the box.  Clicking an OUTPUT pin starts a drag-connection
    in the parent panel.  Clicking an INPUT pin that is already connected
    removes that connection.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../processors/Processor.h"

namespace gui
{

class DraggablePanel;   // fwd

class PinComponent final : public juce::Component
{
public:
    PinComponent (DraggablePanel& ownerPanel,
                  const juce::String& boxId,
                  const juce::String& pinId,
                  bool isOutput,
                  smolfm::PortType type);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;

    /** Accessors used by the parent panel for hit-testing and drawing. */
    const juce::String& getBoxId()   const noexcept { return boxId; }
    const juce::String& getPinId()   const noexcept { return pinId; }
    bool                isOutput()   const noexcept { return output; }
    smolfm::PortType    getType()    const noexcept { return type; }

    /** Position of this pin's centre in PANEL coordinates. */
    juce::Point<float> getPinCentreInPanel() const;

    static constexpr int diameter = 12;

private:
    DraggablePanel& panel;
    juce::String    boxId;
    juce::String    pinId;
    bool            output;
    smolfm::PortType type;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PinComponent)
};

} // namespace gui
