/*
    FrequencyScaleComponent owns the Frequency Scale label, the interval
    dropdown and the factor slider.

    The component handles its own internal layout and exposes no parameter
    logic outside of the constructor, where the APVTS attachments are created.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class FrequencyScaleComponent final : public juce::Component
{
public:
    FrequencyScaleComponent (juce::AudioProcessorValueTreeState& apvts,
                             const juce::String& factorParameterID,
                             const juce::String& intervalParameterID);

    ~FrequencyScaleComponent() override;

    void resized() override;

private:
    juce::Label titleLabel;
    juce::ComboBox intervalBox;
    juce::Slider factorSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> intervalAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> factorAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyScaleComponent)
};

} // namespace gui
