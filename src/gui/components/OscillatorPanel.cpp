/*
    OscillatorPanel implementation.
*/

#include "OscillatorPanel.h"
#include "SliderUtils.h"

namespace gui
{

OscillatorPanel::OscillatorPanel (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& title,
                                  const juce::String& waveformParameterID)
{
    titleLabel.setText (title, juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    // Item order must match the choices declared in
    // PluginProcessor::createParameterLayout() ("Sine", "Saw", "Square", "Triangle").
    // ComboBox item IDs start at 1; ComboBoxAttachment maps them to the choice index.
    waveformBox.addItemList ({ "Sine", "Saw", "Square", "Triangle" }, 1);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (waveformBox);

    waveformAttachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (apvts,
                                                                                          waveformParameterID,
                                                                                          waveformBox));
}

OscillatorPanel::~OscillatorPanel()
{
}

void OscillatorPanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    titleLabel.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (8);

    // The ComboBox gets a fixed height anchored to the bottom; the rest stays
    // empty so the pin area above remains visible.
    waveformBox.setBounds (bounds.removeFromBottom (26).withSizeKeepingCentre (200, 26));
}

} // namespace gui
