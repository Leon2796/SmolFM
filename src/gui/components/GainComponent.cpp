/*
    GainComponent implementation.
*/

#include "GainComponent.h"

namespace gui
{

GainComponent::GainComponent (juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& gainParameterID)
{
    gainSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    gainSlider.setTextValueSuffix ("x");
    addAndMakeVisible (gainSlider);

    gainLabel.setText ("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gainLabel);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, gainParameterID, gainSlider);
}

void GainComponent::resized()
{
    auto r = getLocalBounds().reduced (8);
    gainLabel.setBounds (r.removeFromTop (20));
    r.removeFromTop (8);
    gainSlider.setBounds (r.withSizeKeepingCentre (90, r.getHeight() - 20).translated (r.getCentreX() - 45 - r.getX(), 0));
}

} // namespace gui