/*
    DraggableComponent implementation.
*/

#include "DraggableComponent.h"
#include "PinComponent.h"
#include "DraggablePanel.h"

namespace gui
{

DraggableComponent::DraggableComponent (const juce::String& id,
                                        const juce::String& titleIn,
                                        std::unique_ptr<juce::Component> contentIn)
    : boxId (id),
      title (titleIn),
      content (std::move (contentIn))
{
    jassert (content != nullptr);

    addAndMakeVisible (*content);

    closeButton.setButtonText (juce::String::charToString (0x2715)); // ✕
    closeButton.setTooltip ("Remove this node");
    closeButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.8f));
    closeButton.onClick = [this]
    {
        if (onCloseRequested)
            onCloseRequested (*this);
    };
    addAndMakeVisible (closeButton);

    const juce::Rectangle<int> preferred = getPreferredSize();
    setSize (preferred.getWidth(), preferred.getHeight());
}

void DraggableComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).darker (0.6f));

    g.setColour (juce::Colours::lightblue.withAlpha (0.7f));
    g.fillRect (0, 0, getWidth(), titleBarHeight);

    // The JUCE colour lookup for Slider textbox outline is what makes the box
    // blend with the rest of the UI without pulling in a custom theme.
    g.setColour (getLookAndFeel().findColour (juce::Slider::textBoxOutlineColourId));
    g.drawRect (getLocalBounds(), 1);

    g.setColour (juce::Colours::black);
    g.setFont (titleBarHeight - 6.0f);
    g.drawText (title, 6, 0, getWidth() - 12, titleBarHeight, juce::Justification::centredLeft, true);
}

void DraggableComponent::resized()
{
    content->setBounds (contentPadding,
                        titleBarHeight + contentPadding,
                        getWidth() - 2 * contentPadding,
                        getHeight() - titleBarHeight - 2 * contentPadding);

    // Keep the little close × parked in the top-right corner of the title bar.
    closeButton.setBounds (getWidth() - titleBarHeight, 0, titleBarHeight, titleBarHeight);

    layoutPins();
}

void DraggableComponent::layoutPins()
{
    const int w = getWidth();
    const int h = getHeight();

    const int pinD = PinComponent::diameter;

    auto distribute = [pinD] (juce::OwnedArray<PinComponent>& pins, int x, int top, int bottom)
    {
        const int num = pins.size();
        for (int i = 0; i < num; ++i)
        {
            const float t = num == 1 ? 0.5f : static_cast<float> (i) / static_cast<float> (num - 1);
            const int cy = static_cast<int> (top + (bottom - top) * t);
            pins[i]->setBounds (x, cy - pinD / 2, pinD, pinD);
        }
    };

    const int pinTop = titleBarHeight + pinMargin + pinD / 2;
    const int pinBottom = h - pinMargin - pinD / 2;

    distribute (inputPins,  -pinD / 2,     pinTop, pinBottom);
    distribute (outputPins, w - pinD / 2,  pinTop, pinBottom);
}

void DraggableComponent::addInputPin (const juce::String& pinId, smolfm::PortType type)
{
    auto* parentPanel = dynamic_cast<DraggablePanel*> (getParentComponent());
    jassert (parentPanel != nullptr);
    if (parentPanel == nullptr)
        return;

    auto* pin = new PinComponent (*parentPanel, boxId, pinId, false, type);
    inputPins.add (pin);
    addAndMakeVisible (*pin);
    layoutPins();
}

void DraggableComponent::addOutputPin (const juce::String& pinId, smolfm::PortType type)
{
    auto* parentPanel = dynamic_cast<DraggablePanel*> (getParentComponent());
    jassert (parentPanel != nullptr);
    if (parentPanel == nullptr)
        return;

    auto* pin = new PinComponent (*parentPanel, boxId, pinId, true, type);
    outputPins.add (pin);
    addAndMakeVisible (*pin);
    layoutPins();
}

juce::Point<float> DraggableComponent::getInputPinCentre (int index) const noexcept
{
    if (index < 0 || index >= getNumInputPins())
        return {};

    const auto b = inputPins[index]->getBounds();
    return b.getCentre().toFloat();
}

juce::Point<float> DraggableComponent::getOutputPinCentre (int index) const noexcept
{
    if (index < 0 || index >= getNumOutputPins())
        return {};

    const auto b = outputPins[index]->getBounds();
    return b.getCentre().toFloat();
}

void DraggableComponent::mouseDown (const juce::MouseEvent& e)
{
    draggingFromTitleBar = (e.y < titleBarHeight);

    if (draggingFromTitleBar)
    {
        dragStartMouse = e.getScreenPosition();
        dragStartPosition = getPosition();
        toFront (true);
    }
}

void DraggableComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (! draggingFromTitleBar)
        return;

    const juce::Point<int> delta = e.getScreenPosition() - dragStartMouse;
    juce::Point<int> newPos = dragStartPosition + delta;

    // Keep the box inside its parent so users cannot lose it off-screen.
    if (auto* parent = getParentComponent())
    {
        newPos.x = juce::jlimit (0, juce::jmax (0, parent->getWidth()  - getWidth()),  newPos.x);
        newPos.y = juce::jlimit (0, juce::jmax (0, parent->getHeight() - getHeight()), newPos.y);
    }

    setTopLeftPosition (newPos);

    if (auto* parent = getParentComponent())
        parent->repaint();
}

void DraggableComponent::mouseUp (const juce::MouseEvent& /*e*/)
{
    draggingFromTitleBar = false;
}

juce::Rectangle<int> DraggableComponent::getPreferredSize() const noexcept
{
    // We keep boxes small on purpose: enough room for the content with a
    // comfortable padding, regardless of the panel's size.  Adjust here if
    // a content control changes its default size.
    int contentWidth  = 220;
    int contentHeight = 260;

    if (content != nullptr)
    {
        // Reuse the child's current size hint if it has been set already.
        if (content->getWidth()  > 0) contentWidth  = content->getWidth();
        if (content->getHeight() > 0) contentHeight = content->getHeight();
    }

    return { 0, 0,
             contentWidth  + 2 * contentPadding,
             contentHeight + titleBarHeight + 2 * contentPadding };
}

} // namespace gui
