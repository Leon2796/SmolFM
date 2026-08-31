/*
    FMModulationComponent owns the FM Amount label and slider.

    The component handles its own internal layout and exposes no parameter
    logic outside of the constructor, where the APVTS attachment is created.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class FMModulationComponent final : public juce::Component
{
public:
    FMModulationComponent (juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& parameterID);

    ~FMModulationComponent() override;

    void resized() override;

private:
    juce::Label titleLabel;
    juce::Slider fmSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fmAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FMModulationComponent)
};

} // namespace gui
