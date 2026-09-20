/*
    FilterComponent implementation.
*/

#include "FilterComponent.h"

namespace gui
{

FilterComponent::FilterComponent (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& cutoffParameterID,
                                  const juce::String& resonanceParameterID,
                                  const juce::String& modeParameterID)
{
    // Item order must match the choices declared in
    // PluginProcessor::createParameterLayout() ("LP", "BP", "HP", "Notch").
    modeBox.addItemList ({ "LP", "BP", "HP", "Notch" }, 1);
    addAndMakeVisible (modeBox);

    modeLabel.setText ("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (modeLabel);

    cutoffSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    cutoffSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (cutoffSlider);

    cutoffLabel.setText ("Cutoff", juce::dontSendNotification);
    cutoffLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (cutoffLabel);

    resonanceSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    resonanceSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (resonanceSlider);

    resonanceLabel.setText ("Res", juce::dontSendNotification);
    resonanceLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (resonanceLabel);

    cutoffAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, cutoffParameterID, cutoffSlider);
    resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, resonanceParameterID, resonanceSlider);
    modeAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, modeParameterID, modeBox);
}

void FilterComponent::resized()
{
    auto r = getLocalBounds().reduced (8);
    auto knobs = r.removeFromTop (r.getHeight() - 30);

    auto left   = knobs.removeFromLeft (knobs.getWidth() / 2);
    auto right  = knobs;

    cutoffLabel.setBounds (left.removeFromTop (18));
    cutoffSlider.setBounds (left.withSizeKeepingCentre (80, left.getHeight() - 18));

    resonanceLabel.setBounds (right.removeFromTop (18));
    resonanceSlider.setBounds (right.withSizeKeepingCentre (80, right.getHeight() - 18));

    modeLabel.setBounds (r.removeFromTop (18));
    modeBox.setBounds (r.withSizeKeepingCentre (110, 24));
}

} // namespace gui