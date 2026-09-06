/*
    OscillatorPanel is a reusable JUCE component for one FM oscillator.

    It contains a title label, a ComboBox for waveform selection, and the
    LFO controls: a mode combo (Pitch / LFO) and a rate slider whose range
    (0.01-50 Hz) is only meaningful in LFO mode.

    In pitch mode the frequency is driven by the note_in port only.  In LFO
    mode the oscillator runs at the fixed rate parameter and re-syncs its
    phase on every note.  The same component class is used for every
    oscillator instance.
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
                     const juce::String& lfoModeParameterID,
                     const juce::String& lfoRateParameterID);

    ~OscillatorPanel() override;

    void resized() override;

private:
    void comboBoxChanged (juce::ComboBox*) override;
    void updateLfoVisibility();

    juce::Label titleLabel;
    juce::ComboBox waveformBox;

    juce::Label lfoModeLabel;
    juce::ComboBox lfoModeBox;
    juce::Label lfoRateLabel;
    juce::Slider lfoRateSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  lfoRateAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorPanel)
};

} // namespace gui
