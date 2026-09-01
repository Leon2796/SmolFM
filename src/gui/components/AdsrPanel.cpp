/*
    AdsrPanel implementation.
*/

#include "AdsrPanel.h"
#include "SliderUtils.h"

namespace gui
{

AdsrPanel::AdsrPanel (juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& attackParameterID,
                      const juce::String& decayParameterID,
                      const juce::String& sustainParameterID,
                      const juce::String& releaseParameterID)
{
    titleLabel.setText ("ADSR", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    configureRotarySlider (attackSlider,  " s");
    configureRotarySlider (decaySlider,   " s");
    configureRotarySlider (sustainSlider, "");
    configureRotarySlider (releaseSlider, " s");

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (attackSlider);
    addAndMakeVisible (decaySlider);
    addAndMakeVisible (sustainSlider);
    addAndMakeVisible (releaseSlider);

    attackAttachment.reset  (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, attackParameterID,  attackSlider));
    decayAttachment.reset   (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, decayParameterID,   decaySlider));
    sustainAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, sustainParameterID, sustainSlider));
    releaseAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, releaseParameterID, releaseSlider));
}

AdsrPanel::~AdsrPanel()
{
}

void AdsrPanel::resized()
{
    juce::Grid grid;
    grid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                          juce::Grid::TrackInfo (juce::Grid::Fr (4)) };
    grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                             juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                             juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                             juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
    grid.rowGap = juce::Grid::Px (8);
    grid.columnGap = juce::Grid::Px (8);

    // The title spans all four columns.
    juce::GridItem titleItem (titleLabel);
    titleItem.column = { juce::GridItem::Span (4) };
    grid.items.add (titleItem);

    grid.items.add (juce::GridItem (attackSlider));
    grid.items.add (juce::GridItem (decaySlider));
    grid.items.add (juce::GridItem (sustainSlider));
    grid.items.add (juce::GridItem (releaseSlider));

    grid.performLayout (getLocalBounds());
}

} // namespace gui
