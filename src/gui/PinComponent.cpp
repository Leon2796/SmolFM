/*
    PinComponent implementation.
*/

#include "PinComponent.h"
#include "DraggablePanel.h"

namespace gui
{

PinComponent::PinComponent (DraggablePanel& ownerPanel,
                            const juce::String& boxIdIn,
                            const juce::String& pinIdIn,
                            bool isOutputIn,
                            smolfm::PortType typeIn)
    : panel (ownerPanel),
      boxId (boxIdIn),
      pinId (pinIdIn),
      output (isOutputIn),
      type (typeIn)
{
    setSize (diameter, diameter);
}

void PinComponent::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (1.0f);

    // Visual cue for the "kind" of value: orange = frequency, cyan = signal.
    const auto colour = type == smolfm::PortType::frequency
                            ? juce::Colours::orange
                            : juce::Colours::cyan;

    g.setColour (colour);
    g.drawEllipse (r, 2.0f);

    // Filled dot inside outputs, hollow for inputs (visual "direction").
    if (output)
    {
        g.setColour (colour.withAlpha (0.8f));
        g.fillEllipse (r.reduced (2.5f));
    }
}

void PinComponent::mouseDown (const juce::MouseEvent& e)
{
    // The panel decides what to do — it owns the connection state.
    panel.pinMouseDown (*this, e);
}

void PinComponent::mouseDrag (const juce::MouseEvent& e)
{
    panel.pinMouseDrag (*this, e);
}

void PinComponent::mouseUp (const juce::MouseEvent& e)
{
    panel.pinMouseUp (*this, e);
}

juce::Point<float> PinComponent::getPinCentreInPanel() const
{
    // Convert this pin's local centre into panel coordinates.
    const auto bounds = getBounds();
    const juce::Point<int> centre = bounds.getCentre();

    // PinComponent is a child of the DraggableComponent; convert via parent.
    if (auto* box = dynamic_cast<juce::Component*> (getParentComponent()))
    {
        const juce::Point<int> parentLocal = box->getPosition() + centre;
        return parentLocal.toFloat();
    }

    return centre.toFloat();
}

} // namespace gui
