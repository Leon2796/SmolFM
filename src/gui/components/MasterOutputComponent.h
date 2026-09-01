/*
    MasterOutputComponent is the UI for the final master stage.

    It shows a vertical level meter next to a rotary master volume slider.
    The meter polls the per-voice peak atomics via a timer on the message
    thread and displays the maximum across voices.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class MasterOutputComponent final : public juce::Component,
                                    private juce::Timer
{
public:
    MasterOutputComponent (juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& parameterID,
                           std::function<float()> peakProvider);

    ~MasterOutputComponent() override;

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    juce::Label titleLabel;
    juce::Slider levelSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment;

    std::function<float()> peakProvider;
    float displayedPeak = 0.0f;
    juce::Rectangle<int> meterBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterOutputComponent)
};

} // namespace gui
