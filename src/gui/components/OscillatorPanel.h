/*
    OscillatorPanel is a reusable JUCE component for one FM oscillator.

    It contains a title label, a frequency/ratio rotary slider and three
    radio-style waveform buttons.  The layout is built with juce::Grid.

    The same component class is used for both the Carrier and Modulator.
*/

#pragma once

#include "../../PluginProcessor.h"
#include "WaveformSelector.h"

namespace gui
{

class OscillatorPanel final : public juce::Component
{
public:
    OscillatorPanel (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& title,
                     const juce::String& ratioParameterID,
                     const juce::String& waveformParameterID,
                     int radioGroupId);

    ~OscillatorPanel() override;

    void resized() override;

private:
    void configureSlider (juce::Slider& slider, const juce::String& suffix);

    juce::Label titleLabel;
    juce::Slider ratioSlider;

    juce::ToggleButton sineButton;
    juce::ToggleButton sawButton;
    juce::ToggleButton squareButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttachment;
    std::unique_ptr<gui::WaveformSelector> waveformSelector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorPanel)
};

} // namespace gui
