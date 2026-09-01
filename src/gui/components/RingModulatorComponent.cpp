/*
    RingModulatorComponent implementation.
*/

#include "RingModulatorComponent.h"

namespace gui
{

RingModulatorComponent::RingModulatorComponent()
{
    const auto multiplySign = juce::String::charToString (0x00D7); // ×
    infoLabel.setText ("in1 " + multiplySign + " in2", juce::dontSendNotification);
    infoLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (infoLabel);

    setSize (200, 60);
}

void RingModulatorComponent::resized()
{
    infoLabel.setBounds (getLocalBounds().reduced (8));
}

} // namespace gui
