/*
    FMModulationComponent owns the FM Amount label and slider.

    The component handles its own internal layout and exposes no parameter
    logic outside of the constructor, where the APVTS attachment is created.
    A ghost-needle overlay on top of the knob shows the effective (modulated)
    index while a voice renders; the thumb keeps its attachment semantics.
*/

#pragma once

#include "../../PluginProcessor.h"
#include "ModIndicatorOverlay.h"

namespace gui
{

class FMModulationComponent final : public juce::Component
{
public:
    FMModulationComponent (juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& parameterID,
                       std::function<juce::Range<float>()> effectiveAmountRangeProvider);

    ~FMModulationComponent() override;

    void resized() override;

private:
    juce::Label titleLabel;
    juce::Slider fmSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fmAttachment;

    // Ghost needle over the amount knob (null when no provider is wired).
    std::unique_ptr<ModIndicatorOverlay> amountOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FMModulationComponent)
};

} // namespace gui
