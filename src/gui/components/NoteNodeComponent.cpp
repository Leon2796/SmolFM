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

    // Compact content size: an info stone on 1x1, same recipe as the gain node.
    setSize (100, 88);
}

void NoteNodeComponent::resized()
{
    infoLabel.setBounds (getLocalBounds().reduced (8));
}

} // namespace gui
