/*
    FrequencyScaleComponent owns the Frequency Scale label and factor slider.

    The component handles its own internal layout and exposes no parameter
    logic outside of the constructor, where the APVTS attachment is created.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class FrequencyScaleComponent final : public juce::Component
{
public:
    FrequencyScaleComponent (juce::AudioProcessorValueTreeState& apvts,
                             const juce::String& parameterID);

    ~FrequencyScaleComponent() override;

    void resized() override;

private:
    juce::Label titleLabel;
    juce::Slider factorSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> factorAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyScaleComponent)
};

} // namespace gui
