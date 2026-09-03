/*
    AmComponent is the UI for one AM modulator node.

    It exposes a single depth slider (0..1) bound to the APVTS parameter
    "am<N>Amount".  The carrier/modulator pins live on the box frame, so the
    inside of the box only needs this one control.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace gui
{

class AmComponent final : public juce::Component
{
public:
    AmComponent (juce::AudioProcessorValueTreeState& apvts,
                 const juce::String& amountParamId);
    ~AmComponent() override = default;

    void resized() override;

private:
    juce::Slider amountSlider;
    juce::Label amountLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> amountAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmComponent)
};

} // namespace gui
