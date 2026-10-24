/*
    FMModulationComponent implementation.
*/

#include "FMModulationComponent.h"
#include "SliderUtils.h"

namespace gui
{

FMModulationComponent::FMModulationComponent (juce::AudioProcessorValueTreeState& apvts,
                                      const juce::String& parameterID,
                                      std::function<juce::Range<float>()> effectiveAmountRangeProviderIn)
{
    configureRotarySlider (fmSlider, " rad");

    addAndMakeVisible (fmSlider);

    fmAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts,
                                                                                parameterID,
                                                                                fmSlider));

    // Ghost needle over the amount knob: shows the effective (modulated)
    // index from the engine's telemetry while a voice renders — the thumb
    // itself is never moved.
    if (effectiveAmountRangeProviderIn != nullptr)
        amountOverlay.reset (new ModIndicatorOverlay (fmSlider,
                                                      std::move (effectiveAmountRangeProviderIn)));

    // Compact content size: one knob on 1x1, same recipe as the gain node.
    setSize (100, 88);
}

FMModulationComponent::~FMModulationComponent()
{
}

void FMModulationComponent::resized()
{
    juce::Grid grid;
    grid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
    grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)) };

    grid.items.add (juce::GridItem (fmSlider));

    grid.performLayout (getLocalBounds());
}

} // namespace gui
