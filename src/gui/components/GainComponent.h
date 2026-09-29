/*
    GainComponent is the UI for one gain node: a single rotary bound to the
    gain factor APVTS parameter (0-10, 1.0 = unity) with a bipolar mapping:
    the knob centre shows unity, turning left attenuates (0..1), turning
    right boosts (1..10); the raw value stored in .smolfm stays linear.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class GainComponent final : public juce::Component
{
public:
    GainComponent (juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& gainParameterID);

    ~GainComponent() override = default;

    void resized() override;

private:
    juce::Slider gainSlider;
    juce::Label gainLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainComponent)
};

} // namespace gui