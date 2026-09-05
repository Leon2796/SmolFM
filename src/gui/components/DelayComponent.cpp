/*
    DelayComponent implementation.
*/

#include "DelayComponent.h"

namespace gui
{

DelayComponent::DelayComponent (juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& timeId,
                                const juce::String& feedbackId,
                                const juce::String& mixId,
                                const juce::String& syncId,
                                const juce::String& divisionId)
    : syncParamId (syncId), apvtsRef (&apvts)
{
    // Time ---------------------------------------------------------------
    timeSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    timeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible (timeSlider);
    timeLabel.setText ("Time", juce::dontSendNotification);
    timeLabel.setJustificationType (juce::Justification::centred);
    timeLabel.attachToComponent (&timeSlider, false);
    addAndMakeVisible (timeLabel);

    // Sync toggle --------------------------------------------------------
    addAndMakeVisible (syncButton);

    // Division combo ------------------------------------------------------
    divisionBox.addItemList ({ "1/2", "1/4", "1/8", "1/16" }, 1);
    addAndMakeVisible (divisionBox);
    divisionLabel.setText ("Note", juce::dontSendNotification);
    divisionLabel.setJustificationType (juce::Justification::centred);
    divisionLabel.attachToComponent (&divisionBox, false);
    addAndMakeVisible (divisionLabel);

    // Feedback ------------------------------------------------------------
    feedbackSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    feedbackSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible (feedbackSlider);
    feedbackLabel.setText ("Fdbk", juce::dontSendNotification);
    feedbackLabel.setJustificationType (juce::Justification::centred);
    feedbackLabel.attachToComponent (&feedbackSlider, false);
    addAndMakeVisible (feedbackLabel);

    // Mix -------------------------------------------------------------
    mixSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible (mixSlider);
    mixLabel.setText ("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType (juce::Justification::centred);
    mixLabel.attachToComponent (&mixSlider, false);
    addAndMakeVisible (mixLabel);

    // Attachments -----------------------------------------------------------
    if (timeId.isNotEmpty())       timeAttachment     = std::make_unique<SliderAttachment>  (apvts, timeId, timeSlider);
    if (feedbackId.isNotEmpty())   feedbackAttachment = std::make_unique<SliderAttachment>  (apvts, feedbackId, feedbackSlider);
    if (mixId.isNotEmpty())        mixAttachment      = std::make_unique<SliderAttachment>  (apvts, mixId, mixSlider);
    if (syncId.isNotEmpty())       syncAttachment     = std::make_unique<ButtonAttachment>  (apvts, syncId, syncButton);
    if (divisionId.isNotEmpty())   divisionAttachment = std::make_unique<ComboBoxAttachment>(apvts, divisionId, divisionBox);

    // Listen for sync changes to disable the time slider.
    syncButton.onClick = [this] { updateTimeEnabled(); };
    updateTimeEnabled();

    setSize (260, 130);
}

void DelayComponent::resized()
{
    auto r = getLocalBounds().reduced (6);
    auto topRow = r.removeFromTop (r.getHeight() / 2);
    auto bottomRow = r;

    auto half1 = topRow.removeFromLeft (topRow.getWidth() / 2);
    auto half2 = topRow;

    timeSlider.setBounds (half1.reduced (4).withTrimmedTop (10));
    feedbackSlider.setBounds (half2.reduced (4).withTrimmedTop (10));

    syncButton.setBounds (bottomRow.removeFromLeft (90).reduced (4).withTrimmedTop (10));
    divisionBox.setBounds (bottomRow.removeFromLeft (70).reduced (4).withTrimmedTop (10));
    mixSlider.setBounds (bottomRow.reduced (4).withTrimmedTop (10));
}

void DelayComponent::updateTimeEnabled()
{
    const bool syncActive = syncButton.getToggleState();
    timeSlider.setEnabled (! syncActive);
    timeSlider.setAlpha (syncActive ? 0.35f : 1.0f);
    divisionBox.setEnabled (syncActive);
    divisionBox.setAlpha (syncActive ? 1.0f : 0.35f);
}

} // namespace gui
