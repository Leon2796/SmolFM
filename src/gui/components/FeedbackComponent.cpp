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
    setSize (100, 88);
}

void FeedbackComponent::resized()
{
    amountSlider.setBounds (getLocalBounds().reduced (4).withTrimmedTop (14));
}

} // namespace gui