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
    titleLabel.setText ("FM Amount", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    configureRotarySlider (fmSlider, " rad");

    addAndMakeVisible (titleLabel);
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

    // Compact content size: DraggableComponent sizes the node box from this,
    // so a single-knob FM node no longer falls back to the 220x260 default.
    setSize (100, 140);
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
