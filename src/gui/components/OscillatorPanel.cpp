/*
    OscillatorPanel implementation.
*/

#include "OscillatorPanel.h"
#include "SliderUtils.h"

#include <cmath>

namespace gui
{

OscillatorPanel::OscillatorPanel (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& title,
                                  const juce::String& waveformParameterID,
                                  const juce::String& modeParameterID,
                                  const juce::String& staticFreqParameterID,
                                  const juce::String& lfoRateParameterID)
{
    // Follow mode changes that come from outside this combo box (patch load
    // writes the APVTS and the ComboBoxAttachment updates the combo without
    // notification, so a pure combo-listener misses the reload).  The rows
    // are derived from the PARAMETER value, not the combo selection — the
    // parameter is the single source of truth and arrives first when several
    // attachments fire for the same change.
    if (auto* modeParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (modeParameterID)))
    {
        modeParamValue = modeParam;
        modeSyncAttachment.reset (new juce::ParameterAttachment (*modeParam,
                                                                 [this] (float) { updateFrequencyRows(); },
                                                                 nullptr));
        modeSyncAttachment->sendInitialUpdate();
    }
    titleLabel.setText (title, juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

        // Item order must match the choices declared in
    // PluginProcessor::createParameterLayout() ("Sine", "Saw", "Square",
    // "Triangle", "Noise", "Perlin Noise", "Simplex Noise").
    // ComboBox item IDs start at 1; ComboBoxAttachment maps them to the choice index.
    waveformBox.addItemList ({ "Sine", "Saw", "Square", "Triangle", "Noise", "Perlin Noise", "Simplex Noise" }, 1);

    // Frequency-mode controls.  Mode combo order must match the APVTS choice
    // ("Pitch", "Static", "LFO"); each frequency row is only visible in its
    // own mode.  Both frequency controls are small rotary knobs.
    modeBox.addItemList ({ "Pitch", "Static", "LFO" }, 1);
    modeBox.addListener (this);

    staticFreqSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    staticFreqSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    staticFreqSlider.setTextValueSuffix (" Hz");
    staticFreqSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::lightblue);

    lfoRateSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    lfoRateSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    lfoRateSlider.setTextValueSuffix (" Hz");
    lfoRateSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::lightgreen);

    modeLabel.setText ("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType (juce::Justification::centred);
    staticFreqLabel.setText ("Freq", juce::dontSendNotification);
    staticFreqLabel.setJustificationType (juce::Justification::centred);
    lfoRateLabel.setText ("Rate", juce::dontSendNotification);
    lfoRateLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (waveformBox);
    addAndMakeVisible (modeLabel);
    addAndMakeVisible (modeBox);
    addAndMakeVisible (staticFreqLabel);
    addAndMakeVisible (staticFreqSlider);
    addAndMakeVisible (lfoRateLabel);
    addAndMakeVisible (lfoRateSlider);

    waveformAttachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (apvts,
                                                                                          waveformParameterID,
                                                                                          waveformBox));
    modeAttachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (apvts,
                                                                                      modeParameterID,
                                                                                      modeBox));
    staticFreqAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts,
                                                                                         staticFreqParameterID,
                                                                                         staticFreqSlider));
    lfoRateAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts,
                                                                                       lfoRateParameterID,
                                                                                       lfoRateSlider));

    updateFrequencyRows();
}

OscillatorPanel::~OscillatorPanel()
{
}

void OscillatorPanel::comboBoxChanged (juce::ComboBox*)
{
    updateFrequencyRows();
}

void OscillatorPanel::updateFrequencyRows()
{
    // Mode ids: 1 = Pitch (no extra row), 2 = Static (freq row), 3 = LFO
    // (rate row).  Read from the parameter (not the combo) so a patch load
    // that arrives through the attachments always shows the correct row.
    int mode = 0;
    if (modeParamValue != nullptr)
        mode = juce::jlimit (0, 2, (int) std::round (modeParamValue->convertFrom0to1 (modeParamValue->getValue())));
    else
        mode = modeBox.getSelectedId() - 1;

    staticFreqLabel.setVisible (mode == 1);
    staticFreqSlider.setVisible (mode == 1);
    lfoRateLabel.setVisible (mode == 2);
    lfoRateSlider.setVisible (mode == 2);
    resized();   // re-layout: the knob row only takes space when visible
}

void OscillatorPanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    titleLabel.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (4);

    waveformBox.setBounds (bounds.removeFromTop (26).withSizeKeepingCentre (200, 26));
    bounds.removeFromTop (4);

    // Mode row always visible; the frequency row matching the selected mode
    // shows a small rotary knob below it.  The knob needs its height PLUS the
    // textbox height (TextBoxBelow renders the value strip below the circle).
    auto modeRow = bounds.removeFromTop (22);
    modeLabel.setBounds (modeRow.removeFromLeft (44));
    modeBox.setBounds (modeRow);

    const bool showStatic = staticFreqSlider.isVisible();
    if (showStatic || lfoRateSlider.isVisible())
    {
        bounds.removeFromTop (2);
        auto& label = showStatic ? staticFreqLabel : lfoRateLabel;
        auto& slider = showStatic ? staticFreqSlider : lfoRateSlider;

        label.setBounds (bounds.removeFromTop (16));
        slider.setBounds (bounds.removeFromTop (72).withSizeKeepingCentre (76, 72));
    }
}

} // namespace gui
