/*
    MergerComponent implementation.
*/

#include "MergerComponent.h"

namespace gui
{

MergerComponent::MergerComponent()
{
    const auto arrow = juce::String::charToString (0x2192); // →
    infoLabel.setText ("L | R " + arrow + " out", juce::dontSendNotification);
    infoLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (infoLabel);

    // Compact content size: an info stone on 1x1, same recipe as the gain node.
    setSize (100, 88);
}

void MergerComponent::resized()
{
    infoLabel.setBounds (getLocalBounds().reduced (8));
}

} // namespace gui