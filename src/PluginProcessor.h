/*
    PluginProcessor is the heart of the audio plugin.

    It receives MIDI from the host, routes it to a JUCE Synthesiser, and lets
    the Synthesiser render all active voices into the output buffer.  It also
    owns the AudioProcessorValueTreeState, which stores every parameter and
    handles state save/restore.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "SynthSound.h"
#include "SynthVoice.h"
#include "graph/GraphNodes.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    /**
        Give the editor access to the parameter state so it can attach UI controls.
    */
    juce::AudioProcessorValueTreeState& getParameters();

    /**
        Rewire every voice's internal processor graph according to a connection
        patch received from the UI.

        This is called from the message thread when the user drags a wire or
        clicks an input pin.  The change is picked up on the audio thread on
        the next sample because port connections are atomic.
    */
    void applyConnectionPatch (const smolfm::ConnectionPatch& patch);

    /**
        Working directory for the patch browser (UI state, persisted with the
        plugin state so the host restores it per session/preset).
    */
    juce::File getPatchDirectory() const;
    void setPatchDirectory (const juce::File& dir);

    /** Highest peak level across all voices, for the UI meter. */
    float getMasterPeakLevel() const;

private:
    //==============================================================================
    /**
        Build the parameter layout used by AudioProcessorValueTreeState.
    */
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState parameters;
    juce::Synthesiser synth;

    // Working directory of the patch browser.  Not an APVTS parameter —
    // stored as an attribute on the state value tree instead.
    juce::File patchDirectory;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
