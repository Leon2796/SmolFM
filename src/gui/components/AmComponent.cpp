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

    // Compact content size: one knob on 1x1, same recipe as the gain node.
    setSize (100, 88);
}

void AmComponent::resized()
{
    amountSlider.setBounds (getLocalBounds().reduced (4).withTrimmedTop (14));
}

} // namespace gui
