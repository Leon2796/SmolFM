/*
    DelayComponent is the UI for one delay node.

    Five controls bound to APVTS:
      - TimeMs (rotary, 1..2000 ms, log)
      - Sync (toggle: Free / Host)
      - Division (combo: 1/2, 1/4, 1/8, 1/16, only used when Sync is on)
      - Feedback (rotary 0..0.95)
      - Mix (rotary 0..1)

    The time control is disabled when Sync is active because the host BPM
    plus division takes over.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace gui
{

class DelayComponent final : public juce::Component
{
public:
    DelayComponent (juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& timeId,
                    const juce::String& feedbackId,
                    const juce::String& mixId,
                    const juce::String& syncId,
                    const juce::String& divisionId);
    ~DelayComponent() override = default;

    void resized() override;

private:
    void updateTimeEnabled();

    juce::Slider timeSlider;
    juce::Label timeLabel;
    juce::ToggleButton syncButton { "Host Sync" };
    juce::ComboBox divisionBox;
    juce::Label divisionLabel;
    juce::Slider feedbackSlider;
    juce::Label feedbackLabel;
    juce::Slider mixSlider;
    juce::Label mixLabel;

    using SliderAttachment  = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment  = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment= juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment>  timeAttachment;
    std::unique_ptr<SliderAttachment>  feedbackAttachment;
    std::unique_ptr<SliderAttachment>  mixAttachment;
    std::unique_ptr<ButtonAttachment>  syncAttachment;
    std::unique_ptr<ComboBoxAttachment> divisionAttachment;

    juce::String syncParamId;
    juce::AudioProcessorValueTreeState* apvtsRef = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayComponent)
};

} // namespace gui
