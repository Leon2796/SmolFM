/*
    WaveshaperComponent is the UI for one waveshaper node: a drive rotary and
    a shape combo bound to the two APVTS parameters.  A ghost-needle overlay
    on the drive knob shows the effective (modulated) drive.
*/

#pragma once

#include "../../PluginProcessor.h"
#include "ModIndicatorOverlay.h"

namespace gui
{

class WaveshaperComponent final : public juce::Component
{
public:
    WaveshaperComponent (juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& driveParameterID,
                         const juce::String& shapeParameterID,
                         std::function<juce::Range<float>()> effectiveDriveRangeProvider);

    ~WaveshaperComponent() override = default;

    void resized() override;

private:
    juce::Slider driveSlider;
    juce::ComboBox shapeBox;
    juce::Label driveLabel;
    juce::Label shapeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;

    // Ghost needle over the drive knob (null when no provider is wired).
    std::unique_ptr<ModIndicatorOverlay> driveOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveshaperComponent)
};

} // namespace gui