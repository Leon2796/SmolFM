/*
    FrequencyScaleComponent implementation.
*/

#include "FrequencyScaleComponent.h"
#include "SliderUtils.h"

namespace gui
{

FrequencyScaleComponent::FrequencyScaleComponent (juce::AudioProcessorValueTreeState& apvts,
                                                  const juce::String& factorParameterID,
                                                  const juce::String& intervalParameterID)
{
    titleLabel.setText ("Freq Scale", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    // Item order must match the choices declared in
    // PluginProcessor::createParameterLayout() ("None", "Octave up", ...);
    // ComboBoxAttachment maps them onto the choice index (item id = order+1).
    // Display texts are compact on purpose: the node box stays small while
    // the popup still spells every interval out.
    intervalBox.addItemList ({ "None", "Octave up", "Octave dn", "Fifth up",
                               "Fifth dn", "4th up", "4th dn",
                               "Minor 3rd up", "Major 3rd up",
                               "Semi up", "Semi dn" }, 1);
    intervalBox.setTextWhenNothingSelected ("None");

    configureRotarySlider (factorSlider, " x");

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (intervalBox);
    addAndMakeVisible (factorSlider);

    intervalAttachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (apvts,
                                                                                          intervalParameterID,
                                                                                          intervalBox));
    factorAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts,
                                                                                      factorParameterID,
                                                                                      factorSlider));

    // Compact content size: DraggableComponent reads this to size the node
    // box (title bar + padding), so every fscale node stays small instead of
    // falling back to the wide 220x260 default box.
    setSize (124, 156);
}

FrequencyScaleComponent::~FrequencyScaleComponent()
{
}

void FrequencyScaleComponent::resized()
{
    juce::Grid grid;
    grid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Px (18)),
                          juce::Grid::TrackInfo (juce::Grid::Px (20)),
                          juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
    grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
    grid.rowGap = juce::Grid::Px (6);

    grid.items.add (juce::GridItem (titleLabel));
    grid.items.add (juce::GridItem (intervalBox));
    grid.items.add (juce::GridItem (factorSlider));

    grid.performLayout (getLocalBounds());
}

} // namespace gui
