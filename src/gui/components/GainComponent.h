/*
    GainComponent is the UI for one gain node: a single rotary bound to the
    gain factor APVTS parameter (0-10, 1.0 = unity) with a bipolar mapping:
    the knob centre shows unity, turning left attenuates (0..1), turning
    right boosts (1..10); the raw value stored in .smolfm stays linear.

    A ghost needle overlay can draw the modulated factor (gain * (1+m) over
    factor_mod_in) on top of the knob while a voice renders; velocity stays
    sound-only and never moves the needle, and the thumb is never moved.
*/

#pragma once

#include "../../PluginProcessor.h"
#include "ModIndicatorOverlay.h"

namespace gui
{

class GainComponent final : public juce::Component
{
public:
    GainComponent (juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& gainParameterID,
                   std::function<juce::Range<float>()> effectiveGainRangeProvider);

    ~GainComponent() override = default;

    void resized() override;

private:
    juce::Slider gainSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    // Ghost needle over the gain knob (null when no provider is wired).
    std::unique_ptr<ModIndicatorOverlay> gainOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainComponent)
};

} // namespace gui