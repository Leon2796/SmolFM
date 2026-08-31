/*
    FrequencyScaleComponent implementation.
*/

#include "FrequencyScaleComponent.h"
#include "SliderUtils.h"

namespace gui
{

FrequencyScaleComponent::FrequencyScaleComponent (juce::AudioProcessorValueTreeState& apvts,
                                                  const juce::String& parameterID)
{
    titleLabel.setText ("Freq Scale", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    configureRotarySlider (factorSlider, " x");

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (factorSlider);

    factorAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts,
                                                                                      parameterID,
                                                                                      factorSlider));
}

FrequencyScaleComponent::~FrequencyScaleComponent()
{
}

void FrequencyScaleComponent::resized()
{
    juce::Grid grid;
    grid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                          juce::Grid::TrackInfo (juce::Grid::Fr (4)) };
    grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
    grid.rowGap = juce::Grid::Px (8);

    grid.items.add (juce::GridItem (titleLabel));
    grid.items.add (juce::GridItem (factorSlider));

    grid.performLayout (getLocalBounds());
}

} // namespace gui
