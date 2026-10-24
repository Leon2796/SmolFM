/*
    VelocityScaleComponent implementation.
*/

#include "VelocityScaleComponent.h"
#include "SliderUtils.h"

namespace gui
{

VelocityScaleComponent::VelocityScaleComponent (juce::AudioProcessorValueTreeState& apvts,
                                                const juce::String& curveParameterID,
                                                const juce::String& scaleParameterID)
{
    // Item order must match the choices declared in
    // PluginProcessor::createParameterLayout() ("Linear", "Sqrt", "Square");
    // ComboBoxAttachment maps them onto the choice index (item id = order+1).
    curveBox.addItemList ({ "Linear", "Sqrt", "Square" }, 1);
    curveBox.setTextWhenNothingSelected ("Linear");

    configureRotarySlider (scaleSlider, " x");

    addAndMakeVisible (curveBox);
    addAndMakeVisible (scaleSlider);

    curveAttachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (apvts,
                                                                                       curveParameterID,
                                                                                       curveBox));
    scaleAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts,
                                                                                     scaleParameterID,
                                                                                     scaleSlider));

    // Compact content size: DraggableComponent reads this to size the node
    // box (title bar + padding), same convention as the fscale node.
    setSize (124, 132);
}

VelocityScaleComponent::~VelocityScaleComponent()
{
}

void VelocityScaleComponent::resized()
{
    juce::Grid grid;
    grid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Px (20)),
                          juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
    grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
    grid.rowGap = juce::Grid::Px (6);

    grid.items.add (juce::GridItem (curveBox));
    grid.items.add (juce::GridItem (scaleSlider));

    grid.performLayout (getLocalBounds());
}

} // namespace gui