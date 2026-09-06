/*
    OscillatorPanel implementation.
*/

#include "OscillatorPanel.h"
#include "SliderUtils.h"

namespace gui
{

OscillatorPanel::OscillatorPanel (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& title,
                                  const juce::String& waveformParameterID,
                                  const juce::String& lfoModeParameterID,
                                  const juce::String& lfoRateParameterID)
{
    titleLabel.setText (title, juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    // Item order must match the choices declared in
    // PluginProcessor::createParameterLayout() ("Sine", "Saw", "Square",
    // "Triangle", "Noise").  ComboBox item IDs start at 1;
    // ComboBoxAttachment maps them to the choice index.
    waveformBox.addItemList ({ "Sine", "Saw", "Square", "Triangle", "Noise" }, 1);

    // LFO controls.  Mode combo order must match the APVTS choice
    // ("Pitch", "LFO"); the rate slider is only meaningful in LFO mode.
    lfoModeBox.addItemList ({ "Pitch", "LFO" }, 1);
    lfoModeBox.addListener (this);

    lfoRateSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    lfoRateSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
    lfoRateSlider.setTextValueSuffix (" Hz");

    lfoModeLabel.setText ("Mode", juce::dontSendNotification);
    lfoModeLabel.setJustificationType (juce::Justification::centred);
    lfoRateLabel.setText ("Rate", juce::dontSendNotification);
    lfoRateLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (waveformBox);
    addAndMakeVisible (lfoModeLabel);
    addAndMakeVisible (lfoModeBox);
    addAndMakeVisible (lfoRateLabel);
    addAndMakeVisible (lfoRateSlider);

    waveformAttachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (apvts,
                                                                                          waveformParameterID,
                                                                                          waveformBox));
    lfoModeAttachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (apvts,
                                                                                         lfoModeParameterID,
                                                                                         lfoModeBox));
    lfoRateAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts,
                                                                                       lfoRateParameterID,
                                                                                       lfoRateSlider));

    updateLfoVisibility();
}

OscillatorPanel::~OscillatorPanel()
{
}

void OscillatorPanel::comboBoxChanged (juce::ComboBox*)
{
    updateLfoVisibility();
}

void OscillatorPanel::updateLfoVisibility()
{
    // LFO mode is item id 2 in the mode combo; hide the rate slider in pitch
    // mode so the small box does not grow controls it cannot use.
    const bool lfoActive = lfoModeBox.getSelectedId() == 2;
    lfoRateLabel.setVisible (lfoActive);
    lfoRateSlider.setVisible (lfoActive);
}

void OscillatorPanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    titleLabel.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (4);

    waveformBox.setBounds (bounds.removeFromTop (26).withSizeKeepingCentre (200, 26));
    bounds.removeFromTop (4);

    // Mode row always visible; rate row only shown in LFO mode.
    auto modeRow = bounds.removeFromTop (22);
    lfoModeLabel.setBounds (modeRow.removeFromLeft (44));
    lfoModeBox.setBounds (modeRow);

    if (lfoRateSlider.isVisible())
    {
        bounds.removeFromTop (2);
        auto rateRow = bounds.removeFromTop (24);
        lfoRateLabel.setBounds (rateRow.removeFromLeft (44));
        lfoRateSlider.setBounds (rateRow);
    }
}

} // namespace gui
