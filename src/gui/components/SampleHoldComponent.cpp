/*
    SampleHoldComponent implementation.
*/

#include "SampleHoldComponent.h"

namespace gui
{

SampleHoldComponent::SampleHoldComponent (juce::AudioProcessorValueTreeState& apvts,
                                          const juce::String& rateId,
                                          const juce::String& syncId,
                                          const juce::String& divisionId)
{
    // Rate ---------------------------------------------------------------
    rateSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    rateSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    addAndMakeVisible (rateSlider);
    rateLabel.setText ("Rate", juce::dontSendNotification);
    rateLabel.setJustificationType (juce::Justification::centred);
    rateLabel.attachToComponent (&rateSlider, false);
    addAndMakeVisible (rateLabel);

    // Sync toggle --------------------------------------------------------
    addAndMakeVisible (syncButton);

    // Division combo ------------------------------------------------------
    divisionBox.addItemList ({ "1/2", "1/4", "1/8", "1/16" }, 1);
    addAndMakeVisible (divisionBox);
    divisionLabel.setText ("Note", juce::dontSendNotification);
    divisionLabel.setJustificationType (juce::Justification::centred);
    divisionLabel.attachToComponent (&divisionBox, false);
    addAndMakeVisible (divisionLabel);

    // Attachments ---------------------------------------------------------
    if (rateId.isNotEmpty())     rateAttachment     = std::make_unique<SliderAttachment>   (apvts, rateId, rateSlider);
    if (syncId.isNotEmpty())     syncAttachment     = std::make_unique<ButtonAttachment>   (apvts, syncId, syncButton);
    if (divisionId.isNotEmpty()) divisionAttachment = std::make_unique<ComboBoxAttachment> (apvts, divisionId, divisionBox);

    // Listen for sync changes to disable the rate slider.
    syncButton.onClick = [this] { updateRateEnabled(); };
    updateRateEnabled();

    // Compact content size: one rotary + the sync/division strip; the narrow
    // width keeps the node box one cell wide (two rows).
    setSize (112, 160);
}

void SampleHoldComponent::resized()
{
    auto r = getLocalBounds().reduced (8);

    // Bottom strip: sync toggle + division combo.
    auto bottom = r.removeFromBottom (28);
    syncButton.setBounds (bottom.removeFromLeft (bottom.getWidth() / 2).reduced (2));
    divisionBox.setBounds (bottom.reduced (2));

    // The single rotary control fills the remaining space; full width now,
    // because the narrow box stacks the knob above the strip.
    rateSlider.setBounds (r.reduced (4, 4).withTrimmedTop (14));
}

void SampleHoldComponent::updateRateEnabled()
{
    const bool syncActive = syncButton.getToggleState();
    rateSlider.setEnabled (! syncActive);
    rateSlider.setAlpha (syncActive ? 0.35f : 1.0f);
    divisionBox.setEnabled (syncActive);
    divisionBox.setAlpha (syncActive ? 1.0f : 0.35f);
}

} // namespace gui