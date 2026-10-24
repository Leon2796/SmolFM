/*
    VelocityScaleComponent owns the curve dropdown
    and the scale slider.

    The component handles its own internal layout and exposes no parameter
    logic outside of the constructor, where the APVTS attachments are created.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class VelocityScaleComponent final : public juce::Component
{
public:
    VelocityScaleComponent (juce::AudioProcessorValueTreeState& apvts,
                            const juce::String& curveParameterID,
                            const juce::String& scaleParameterID);

    ~VelocityScaleComponent() override;

    void resized() override;

private:
    juce::ComboBox curveBox;
    juce::Slider scaleSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> curveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> scaleAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelocityScaleComponent)
};

} // namespace gui