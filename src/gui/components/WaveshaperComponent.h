/*
    WaveshaperComponent is the UI for one waveshaper node: a drive rotary and
    a shape combo bound to the two APVTS parameters.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class WaveshaperComponent final : public juce::Component
{
public:
    WaveshaperComponent (juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& driveParameterID,
                         const juce::String& shapeParameterID);

    ~WaveshaperComponent() override = default;

    void resized() override;

private:
    juce::Slider driveSlider;
    juce::ComboBox shapeBox;
    juce::Label driveLabel;
    juce::Label shapeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveshaperComponent)
};

} // namespace gui