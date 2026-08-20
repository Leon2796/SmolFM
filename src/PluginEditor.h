/*
    PluginEditor is the user interface of the synthesizer.

    It only displays and controls parameters.  The actual source of truth is the
    AudioProcessorValueTreeState owned by the processor.  Each slider and button
    is attached to a parameter through a JUCE attachment object, so the UI stays
    synchronized with the audio engine and with the host's automation.

    Parameter flow:

        UI control  ←── attachment ──→  APVTS parameter  ←── voice reads ──→  audio engine
*/

#pragma once

#include "PluginProcessor.h"

//==============================================================================
/**
    Small helper that keeps three radio-style toggle buttons in sync with a
    single AudioParameterChoice.

    A Choice parameter stores one integer index (0 = Sine, 1 = Saw, 2 = Square).
    Three ButtonParameterAttachments cannot be used because they all try to
    write a boolean value to the same parameter.  Instead, this class listens
    to button clicks and writes the matching index, and listens to parameter
    changes to update the selected button.
*/
class WaveformSelector final : private juce::Button::Listener,
                               private juce::AudioProcessorParameter::Listener
{
public:
    WaveformSelector (juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& parameterID,
                      juce::ToggleButton& sineButton,
                      juce::ToggleButton& sawButton,
                      juce::ToggleButton& squareButton);

    ~WaveformSelector() override;

    /** Make the buttons reflect the current parameter value right now. */
    void sendInitialUpdate();

private:
    void buttonClicked (juce::Button* button) override;
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;

    void updateButtonsFromParameter();
    void setSelectedButton (int waveformIndex);
    void setParameterFromButton (int waveformIndex);

    juce::RangedAudioParameter& parameter;
    juce::ToggleButton& sineButton;
    juce::ToggleButton& sawButton;
    juce::ToggleButton& squareButton;

    bool ignoreCallbacks = false;
};

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AudioPluginAudioProcessor& processorRef;

    // Frequency ratio and FM amount sliders.
    juce::Slider carrierRatioSlider;
    juce::Slider modulatorRatioSlider;
    juce::Slider fmAmountSlider;

    // ADSR sliders.
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;

    // Waveform buttons.  Three buttons per oscillator, grouped as radio groups.
    juce::ToggleButton carrierSineButton;
    juce::ToggleButton carrierSawButton;
    juce::ToggleButton carrierSquareButton;

    juce::ToggleButton modulatorSineButton;
    juce::ToggleButton modulatorSawButton;
    juce::ToggleButton modulatorSquareButton;

    // Labels for clarity.
    juce::Label carrierLabel;
    juce::Label modulatorLabel;
    juce::Label fmLabel;
    juce::Label adsrLabel;

    // Attachments keep the UI synchronized with the APVTS parameters.
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> carrierRatioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modulatorRatioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fmAmountAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;

    // Waveform selection helpers keep the radio buttons and Choice parameters in sync.
    std::unique_ptr<WaveformSelector> carrierWaveformSelector;
    std::unique_ptr<WaveformSelector> modulatorWaveformSelector;

    // Helper to configure a slider so every slider in the UI looks consistent.
    void configureSlider (juce::Slider& slider, const juce::String& suffix);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

