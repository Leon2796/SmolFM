/*
    AmComponent implementation.
*/

#include "AmComponent.h"

namespace gui
{

AmComponent::AmComponent (juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& amountParamId)
{
    amountSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    amountSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (amountSlider);

    amountLabel.setText ("Depth", juce::dontSendNotification);
    amountLabel.setJustificationType (juce::Justification::centred);
    amountLabel.attachToComponent (&amountSlider, false);
    addAndMakeVisible (amountLabel);

    if (amountParamId.isNotEmpty())
        amountAttachment = std::make_unique<SliderAttachment> (apvts, amountParamId, amountSlider);

    setSize (200, 110);
}

void AmComponent::resized()
{
    amountSlider.setBounds (getLocalBounds().reduced (8).withTrimmedTop (14));
}

} // namespace gui
