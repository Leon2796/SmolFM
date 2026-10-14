/*
    SampleHoldComponent is the UI for one sample-hold node.

    Three controls bound to APVTS:
      - Rate (rotary, 0.5..20000 Hz)
      - Sync (toggle: Free / Host)
      - Division (combo: 1/2, 1/4, 1/8, 1/16, only used when Sync is on)

    The rate control is disabled while Sync is active because the host BPM
    plus division takes over.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace gui
{

class SampleHoldComponent final : public juce::Component
{
public:
    SampleHoldComponent (juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& rateId,
                         const juce::String& syncId,
                         const juce::String& divisionId);
    ~SampleHoldComponent() override = default;

    void resized() override;

private:
    void updateRateEnabled();

    juce::Slider rateSlider;
    juce::Label rateLabel;
    juce::ToggleButton syncButton { "Host Sync" };
    juce::ComboBox divisionBox;
    juce::Label divisionLabel;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment>   rateAttachment;
    std::unique_ptr<ButtonAttachment>   syncAttachment;
    std::unique_ptr<ComboBoxAttachment> divisionAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleHoldComponent)
};

} // namespace gui