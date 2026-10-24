/*
    SplitterComponent implementation.
*/

#include "SplitterComponent.h"

namespace gui
{

SplitterComponent::SplitterComponent()
{
    const auto arrow = juce::String::charToString (0x2192); // →
    infoLabel.setText ("in " + arrow + " L | R", juce::dontSendNotification);
    infoLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (infoLabel);

    // Compact content size: an info stone on 1x1, same recipe as the gain node.
    setSize (100, 88);
}

void SplitterComponent::resized()
{
    infoLabel.setBounds (getLocalBounds().reduced (8));
}

} // namespace gui