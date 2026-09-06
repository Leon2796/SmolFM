/*
    OscillatorPanel is a reusable JUCE component for one FM oscillator.

    It contains a title label, a ComboBox for waveform selection, and the
    frequency-mode controls: a mode combo (Pitch / Static / LFO), a static
    frequency slider (20-20000 Hz, audio range) and an LFO rate slider
    (0.01-50 Hz).  Only the row matching the selected mode is visible.

    In pitch mode the frequency is driven by the note_in port only.  In
    Static/LFO mode the oscillator runs at the fixed parameter value and
    re-syncs its phase on every note.  The same component class is used for
    every oscillator instance.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

class OscillatorPanel final : public juce::Component,
                              private juce::ComboBox::Listener
{
public:
    OscillatorPanel (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& title,
                     const juce::String& waveformParameterID,
                     const juce::String& modeParameterID,
                     const juce::String& staticFreqParameterID,
                     const juce::String& lfoRateParameterID);

    ~OscillatorPanel() override;

    void resized() override;

private:
    void comboBoxChanged (juce::ComboBox*) override;
    void updateFrequencyRows();

    juce::Label titleLabel;
    juce::ComboBox waveformBox;

    juce::Label modeLabel;
    juce::ComboBox modeBox;
    juce::Label staticFreqLabel;
    juce::Slider staticFreqSlider;
    juce::Label lfoRateLabel;
    juce::Slider lfoRateSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  staticFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  lfoRateAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorPanel)
};

} // namespace gui
