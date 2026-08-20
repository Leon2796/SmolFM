/*
    PluginEditor is the user interface of the synthesizer.

    It composes the high-level UI from dedicated child components.  Each
    component owns its own controls and layout; the editor only arranges them
    with a juce::Grid.

    Parameter flow:

        UI control  ←── attachment ──→  APVTS parameter  ←── voice reads ──→  audio engine
*/

#pragma once

#include "PluginProcessor.h"
#include "gui/components/OscillatorPanel.h"
#include "gui/components/FmAmountComponent.h"
#include "gui/components/AdsrPanel.h"

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

    // Top-level title label.
    juce::Label titleLabel;

    // Logical UI sections.  Each component owns its own controls and layout.
    gui::OscillatorPanel carrierPanel;
    gui::FmAmountComponent fmAmountComponent;
    gui::OscillatorPanel modulatorPanel;
    gui::AdsrPanel adsrPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

