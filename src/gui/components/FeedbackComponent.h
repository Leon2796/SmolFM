/*
    FeedbackComponent is the UI for one feedback (cycle breaker) node: a
    single rotary bound to the APVTS amount parameter (per-pass damping of
    the recirculated loop signal).
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class FeedbackComponent final : public juce::Component
{
public:
    FeedbackComponent (juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& amountParameterID);

    ~FeedbackComponent() override = default;

    void resized() override;

private:
    juce::Slider amountSlider;
    juce::Label amountLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeedbackComponent)
};

} // namespace gui