/*
    FeedbackComponent implementation.
*/

#include "FeedbackComponent.h"

namespace gui
{

FeedbackComponent::FeedbackComponent (juce::AudioProcessorValueTreeState& apvts,
                                      const juce::String& amountParameterID)
{
    amountSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    amountSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible (amountSlider);

    amountLabel.setText ("Amount", juce::dontSendNotification);
    amountLabel.setJustificationType (juce::Justification::centred);
    amountLabel.attachToComponent (&amountSlider, false);
    addAndMakeVisible (amountLabel);

    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, amountParameterID, amountSlider);

    // Compact content size: keeps the feedback node box small (one knob).
    setSize (130, 150);
}

void FeedbackComponent::resized()
{
    auto r = getLocalBounds().reduced (8);
    amountSlider.setBounds (r.withTrimmedTop (14).withSizeKeepingCentre (80, r.getHeight() - 14));
}

} // namespace gui