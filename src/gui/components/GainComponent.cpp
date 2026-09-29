/*
    GainComponent implementation.
*/

#include "GainComponent.h"
#include "SliderUtils.h"

namespace gui
{

GainComponent::GainComponent (juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& gainParameterID)
{
    configureRotarySlider (gainSlider, "x");
    addAndMakeVisible (gainSlider);

    gainLabel.setText ("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gainLabel);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, gainParameterID, gainSlider);

    // Compact content size: DraggableComponent reads this to size the node
    // box, so a single gain knob does not fall back to the 220x260 default.
    setSize (100, 114);
}

void GainComponent::resized()
{
    auto r = getLocalBounds().reduced (4);
    gainLabel.setBounds (r.removeFromTop (18));
    r.removeFromTop (4);
    gainSlider.setBounds (r.withSizeKeepingCentre (juce::jmin (92, r.getWidth()), r.getHeight()));
}

} // namespace gui