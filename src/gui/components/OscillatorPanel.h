/*
    OscillatorPanel is a reusable JUCE component for one FM oscillator.

    It contains a title label, a frequency rotary slider and a ComboBox for
    waveform selection.  The layout is built with juce::Grid.

    The same component class is used for both the Carrier and Modulator.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class OscillatorPanel final : public juce::Component
{
public:
    OscillatorPanel (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& title,
                     const juce::String& frequencyParameterID,
                     const juce::String& waveformParameterID);

    ~OscillatorPanel() override;

    void resized() override;

private:
    juce::Label titleLabel;
    juce::Slider frequencySlider;
    juce::ComboBox waveformBox;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> frequencyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorPanel)
};

} // namespace gui
