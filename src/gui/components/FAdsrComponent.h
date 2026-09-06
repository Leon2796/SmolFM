/*
    FAdsrComponent is the UI for one F-ADSR pitch-envelope node.

    Six rotary sliders bound to APVTS: Attack, Decay, Sustain, Release plus
    the Up and Down scaling factors applied to the input frequency at
    envelope value E = 1 and E = 0.

    Slightly wider than the other processors so the six sliders keep usable
    thumb sizes; the box height matches the standard 260px content height.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class FAdsrComponent final : public juce::Component,
                             private juce::Slider::Listener
{
public:
    FAdsrComponent (juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& attackParameterID,
                    const juce::String& decayParameterID,
                    const juce::String& sustainParameterID,
                    const juce::String& releaseParameterID,
                    const juce::String& upParameterID,
                    const juce::String& downParameterID);

    ~FAdsrComponent() override;

    void resized() override;

private:
    /** Draws the current ADSR shape from the four timing sliders. */
    class CurveDisplay final : public juce::Component
    {
    public:
        void paint (juce::Graphics& g) override;
    };

    void sliderValueChanged (juce::Slider*) override;

    juce::Label titleLabel;

    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    juce::Slider upSlider;
    juce::Slider downSlider;

    CurveDisplay curveDisplay;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> upAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> downAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FAdsrComponent)
};

} // namespace gui