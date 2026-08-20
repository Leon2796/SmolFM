/*
    WaveformSelector keeps three radio-style toggle buttons in sync with a
    single juce::AudioParameterChoice.

    A Choice parameter stores one integer index (0 = Sine, 1 = Saw, 2 = Square).
    JUCE's ButtonParameterAttachment only supports boolean parameters, so this
    small helper listens to button clicks and writes the matching index, and
    listens to parameter changes to update the selected button.
*/

#pragma once

#include "../../PluginProcessor.h"

namespace gui
{

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

} // namespace gui
