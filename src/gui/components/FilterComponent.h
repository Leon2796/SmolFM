/*
    FilterComponent is the UI for one filter node: a static cutoff rotary,
    a resonance rotary and a mode combo bound to the three APVTS parameters.

    The static cutoff is the fallback value used when the optional
    cutoff_mod_in port carries no connection; dynamic sweeps replace it —
    ghost-needle overlays on both knobs show the effective values.
*/

#pragma once

#include "../../PluginProcessor.h"
#include "ModIndicatorOverlay.h"

namespace gui
{

class FilterComponent final : public juce::Component
{
public:
    FilterComponent (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& cutoffParameterID,
                     const juce::String& resonanceParameterID,
                     const juce::String& modeParameterID,
                     std::function<juce::Range<float>()> effectiveCutoffRangeProvider,
                     std::function<juce::Range<float>()> effectiveResonanceRangeProvider);

    ~FilterComponent() override = default;

    void resized() override;

private:
    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;
    juce::ComboBox modeBox;
    juce::Label cutoffLabel;
    juce::Label resonanceLabel;
    juce::Label modeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  cutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  resonanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;

    // Ghost needles on both knobs (null when no provider is wired).  On the
    // cutoff the needle is the star of the show: with cutoff_mod_in wired the
    // knob's own value is not even in the loop.
    std::unique_ptr<ModIndicatorOverlay> cutoffOverlay;
    std::unique_ptr<ModIndicatorOverlay> resonanceOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterComponent)
};

} // namespace gui