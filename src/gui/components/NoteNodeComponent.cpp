/*
    NoteNodeComponent implementation.
*/

#include "NoteNodeComponent.h"

namespace gui
{

NoteNodeComponent::NoteNodeComponent()
{
    infoLabel.setText ("MIDI note -> Hz", juce::dontSendNotification);
    infoLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (infoLabel);

    setSize (200, 60);
}

void NoteNodeComponent::resized()
{
    infoLabel.setBounds (getLocalBounds().reduced (8));
}

} // namespace gui
