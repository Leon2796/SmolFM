/*
    WaveshaperComponent implementation.
*/

#include "WaveshaperComponent.h"

namespace gui
{

WaveshaperComponent::WaveshaperComponent (juce::AudioProcessorValueTreeState& apvts,
                                          const juce::String& driveParameterID,
                                          const juce::String& shapeParameterID)
{
    // Item order must match the choices declared in
    // PluginProcessor::createParameterLayout() ("Soft", "Hard", "Fold").
    shapeBox.addItemList ({ "Soft", "Hard", "Fold" }, 1);
    addAndMakeVisible (shapeBox);

    shapeLabel.setText ("Shape", juce::dontSendNotification);
    shapeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (shapeLabel);

    driveSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    driveSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (driveSlider);

    driveLabel.setText ("Drive", juce::dontSendNotification);
    driveLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (driveLabel);

    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, driveParameterID, driveSlider);
    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, shapeParameterID, shapeBox);
}

void WaveshaperComponent::resized()
{
    auto r = getLocalBounds().reduced (8);
    auto left  = r.removeFromLeft (r.getWidth() / 2);
    auto right = r;

    shapeLabel.setBounds (right.removeFromTop (20));
    shapeBox.setBounds (right.removeFromBottom (26).withSizeKeepingCentre (100, 26));

    driveLabel.setBounds (left.removeFromTop (20));
    driveSlider.setBounds (left.withSizeKeepingCentre (90, left.getHeight() - 20));
}

} // namespace gui