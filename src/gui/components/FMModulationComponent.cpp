/*
    FMModulationComponent implementation.
*/

#include "FMModulationComponent.h"
#include "SliderUtils.h"

namespace gui
{

FMModulationComponent::FMModulationComponent (juce::AudioProcessorValueTreeState& apvts,
                                      const juce::String& parameterID)
{
    titleLabel.setText ("FM Amount", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    configureRotarySlider (fmSlider, " rad");

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (fmSlider);

    fmAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts,
                                                                                parameterID,
                                                                                fmSlider));
}

FMModulationComponent::~FMModulationComponent()
{
}

void FMModulationComponent::resized()
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

} // namespace gui
