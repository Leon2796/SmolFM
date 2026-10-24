/*
    ResonatorComponent is the UI for one resonator node: six rotary controls
    bound to the APVTS parameters (tune, Q, decay, vel spread and the
    internal strike pair — strike level and strike time; both defaults keep
    the strike off).

    The static tune is the fallback value used when the optional tune port
    carries no connection; dynamic sweeps replace it.  A ghost-needle overlay
    on the Q knob shows the effective (modulated) Q.
*/

#pragma once

#include "../../PluginProcessor.h"
#include "ModIndicatorOverlay.h"

namespace gui
{

class ResonatorComponent final : public juce::Component
{
public:
    ResonatorComponent (juce::AudioProcessorValueTreeState& apvts,
                        const juce::String& tuneParameterID,
                        const juce::String& qParameterID,
                        const juce::String& decayParameterID,
                        const juce::String& velSpreadParameterID,
                        const juce::String& strikeLevelParameterID,
                        const juce::String& strikeTimeParameterID,
                        std::function<juce::Range<float>()> effectiveQRangeProvider);

    ~ResonatorComponent() override = default;

    void resized() override;

private:
    juce::Slider tuneSlider;
    juce::Slider qSlider;
    juce::Slider decaySlider;
    juce::Slider spreadSlider;
    juce::Slider strikeLevelSlider;
    juce::Slider strikeTimeSlider;
    juce::Label tuneLabel;
    juce::Label qLabel;
    juce::Label decayLabel;
    juce::Label spreadLabel;
    juce::Label strikeLevelLabel;
    juce::Label strikeTimeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> spreadAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> strikeLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> strikeTimeAttachment;

    // Ghost needle over the Q knob (null when no provider is wired).
    std::unique_ptr<ModIndicatorOverlay> qOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorComponent)
};

} // namespace gui