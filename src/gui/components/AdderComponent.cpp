/*
    AdderComponent implementation.
*/

#include "AdderComponent.h"

namespace gui
{

AdderComponent::AdderComponent()
{
    const auto plus = juce::String::charToString (0x2b); // +
    infoLabel.setText ("in1 " + plus + " in2 " + juce::String::charToString (0x2192) + " out",
                       juce::dontSendNotification);
    infoLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (infoLabel);

    setSize (200, 60);
}

void AdderComponent::resized()
{
    infoLabel.setBounds (getLocalBounds().reduced (8));
}

} // namespace gui