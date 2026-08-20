/*
    FmAmountComponent owns the FM Amount label and slider.

    The component handles its own internal layout and exposes no parameter
    logic outside of the constructor, where the APVTS attachment is created.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class FmAmountComponent final : public juce::Component
{
public:
    FmAmountComponent (juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& parameterID);

    ~FmAmountComponent() override;

    void resized() override;

private:
    void configureSlider (juce::Slider& slider);

    juce::Label titleLabel;
    juce::Slider fmSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fmAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FmAmountComponent)
};

} // namespace gui
