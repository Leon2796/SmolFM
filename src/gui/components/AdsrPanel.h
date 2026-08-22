/*
    AdsrPanel owns the four ADSR controls.

    It contains a title label and Attack, Decay, Sustain and Release rotary
    sliders.  The sliders are arranged with juce::Grid.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class AdsrPanel final : public juce::Component
{
public:
    AdsrPanel (juce::AudioProcessorValueTreeState& apvts);

    ~AdsrPanel() override;

    void resized() override;

private:
    juce::Label titleLabel;

    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdsrPanel)
};

} // namespace gui
