/*
    FmAmountComponent implementation.
*/

#include "FmAmountComponent.h"

namespace gui
{

FmAmountComponent::FmAmountComponent (juce::AudioProcessorValueTreeState& apvts,
                                      const juce::String& parameterID)
{
    titleLabel.setText ("FM Amount", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    configureSlider (fmSlider);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (fmSlider);

    fmAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts,
                                                                                parameterID,
                                                                                fmSlider));
}

FmAmountComponent::~FmAmountComponent()
{
}

void FmAmountComponent::resized()
{
    juce::Grid grid;
    grid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                          juce::Grid::TrackInfo (juce::Grid::Fr (4)) };
    grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
    grid.rowGap = juce::Grid::Px (8);

    grid.items.add (juce::GridItem (titleLabel));
    grid.items.add (juce::GridItem (fmSlider));

    grid.performLayout (getLocalBounds());
}

void FmAmountComponent::configureSlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 24);
    slider.setTextValueSuffix (" rad");
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::lightblue);
}

} // namespace gui
