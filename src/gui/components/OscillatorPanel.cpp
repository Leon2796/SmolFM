/*
    OscillatorPanel implementation.
*/

#include "OscillatorPanel.h"
#include "SliderUtils.h"

namespace gui
{

namespace
{
    void configureWaveformButton (juce::ToggleButton& button,
                                  const juce::String& text,
                                  int radioGroupId)
    {
        button.setButtonText (text);
        button.setRadioGroupId (radioGroupId);
        button.setClickingTogglesState (true);
    }
}

//==============================================================================
OscillatorPanel::OscillatorPanel (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& title,
                                  const juce::String& frequencyParameterID,
                                  const juce::String& waveformParameterID,
                                  int radioGroupId)
{
    titleLabel.setText (title, juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    configureRotarySlider (frequencySlider, " Hz");

    configureWaveformButton (sineButton,   "Sine",   radioGroupId);
    configureWaveformButton (sawButton,    "Saw",    radioGroupId);
    configureWaveformButton (squareButton, "Square", radioGroupId);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (frequencySlider);
    addAndMakeVisible (sineButton);
    addAndMakeVisible (sawButton);
    addAndMakeVisible (squareButton);

    frequencyAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts,
                                                                                         frequencyParameterID,
                                                                                         frequencySlider));
    waveformSelector.reset (new WaveformSelector (apvts,
                                                  waveformParameterID,
                                                  sineButton,
                                                  sawButton,
                                                  squareButton));
    waveformSelector->sendInitialUpdate();
}

OscillatorPanel::~OscillatorPanel()
{
}

void OscillatorPanel::resized()
{
    // Title, ratio slider and waveform buttons laid out with a grid.
    // The bottom row has three columns for the waveform buttons.
    juce::Grid grid;
    grid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                          juce::Grid::TrackInfo (juce::Grid::Fr (4)),
                          juce::Grid::TrackInfo (juce::Grid::Fr (2)) };
    grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                               juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                               juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
    grid.rowGap = juce::Grid::Px (8);
    grid.columnGap = juce::Grid::Px (4);

    // Title spans all three columns.
    grid.items.add (juce::GridItem (titleLabel).withArea (1, juce::GridItem::Span (3)));

    // Frequency slider spans all three columns.
    grid.items.add (juce::GridItem (frequencySlider).withArea (2, juce::GridItem::Span (3)));

    // Waveform buttons each occupy one column of the bottom row.
    grid.items.add (juce::GridItem (sineButton).withArea (3, 1));
    grid.items.add (juce::GridItem (sawButton).withArea (3, 2));
    grid.items.add (juce::GridItem (squareButton).withArea (3, 3));

    grid.performLayout (getLocalBounds());
}

} // namespace gui
