/*
    AdsrPanel implementation.
*/

#include "AdsrPanel.h"

namespace gui
{

AdsrPanel::AdsrPanel (juce::AudioProcessorValueTreeState& apvts)
{
    titleLabel.setText ("ADSR", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    configureSlider (attackSlider,  " s");
    configureSlider (decaySlider,   " s");
    configureSlider (sustainSlider, "");
    configureSlider (releaseSlider, " s");

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (attackSlider);
    addAndMakeVisible (decaySlider);
    addAndMakeVisible (sustainSlider);
    addAndMakeVisible (releaseSlider);

    attackAttachment.reset  (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, "attack",  attackSlider));
    decayAttachment.reset   (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, "decay",   decaySlider));
    sustainAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, "sustain", sustainSlider));
    releaseAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, "release", releaseSlider));
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

void AdsrPanel::configureSlider (juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 24);
    slider.setTextValueSuffix (suffix);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::lightblue);
}

} // namespace gui
