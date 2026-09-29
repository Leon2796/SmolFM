/*
    GainComponent is the UI for one gain node: a single rotary bound to the
    gain factor APVTS parameter (0-10, 1.0 = transparent; the range is skewed
    so unity sits at 12 o'clock and fine attenuation values stay reachable).
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