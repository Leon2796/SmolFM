/*
    DraggableComponent implementation.
*/

#include "DraggableComponent.h"

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
