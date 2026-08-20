/*
    WaveformSelector implementation.
*/

#include "WaveformSelector.h"

namespace gui
{

namespace
{
    void configureWaveformButton (juce::ToggleButton& button,
                                  const juce::String& text,
                                  int radioGroupId)
    {
        button.setButtonText (text);
        button.setRadioGroupId (radioGroupId);
        button.setClickingTogglesState (true);
    }
}

//==============================================================================
WaveformSelector::WaveformSelector (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& parameterID,
                                  juce::ToggleButton& sineButtonIn,
                                  juce::ToggleButton& sawButtonIn,
                                  juce::ToggleButton& squareButtonIn)
    : parameter (*apvts.getParameter (parameterID)),
      sineButton (sineButtonIn),
      sawButton (sawButtonIn),
      squareButton (squareButtonIn)
{
    parameter.addListener (this);

    sineButton.addListener (this);
    sawButton.addListener (this);
    squareButton.addListener (this);
}

WaveformSelector::~WaveformSelector()
{
    parameter.removeListener (this);

    sineButton.removeListener (this);
    sawButton.removeListener (this);
    squareButton.removeListener (this);
}

void WaveformSelector::sendInitialUpdate()
{
    updateButtonsFromParameter();
}

void WaveformSelector::buttonClicked (juce::Button* button)
{
    if (ignoreCallbacks)
        return;

    // A radio group turns the previously selected button OFF when a new one
    // is selected, and that also generates a buttonClicked message.  Only act
    // on the button that is being turned ON.
    if (! button->getToggleState())
        return;

    if (button == &sineButton)
        setParameterFromButton (0);
    else if (button == &sawButton)
        setParameterFromButton (1);
    else if (button == &squareButton)
        setParameterFromButton (2);
}

void WaveformSelector::parameterValueChanged (int /*parameterIndex*/, float newValue)
{
    // newValue is normalised [0, 1].  Convert back to a waveform index.
    int waveformIndex = static_cast<int> (std::round (parameter.convertFrom0to1 (newValue)));

    // Ignore if the index already matches the selected button to avoid loops.
    int currentIndex = -1;
    if (sineButton.getToggleState())   currentIndex = 0;
    if (sawButton.getToggleState())    currentIndex = 1;
    if (squareButton.getToggleState()) currentIndex = 2;

    if (waveformIndex == currentIndex)
        return;

    juce::MessageManager::callAsync ([this, waveformIndex]
    {
        setSelectedButton (waveformIndex);
    });
}

void WaveformSelector::parameterGestureChanged (int /*parameterIndex*/, bool /*gestureIsStarting*/)
{
}

void WaveformSelector::updateButtonsFromParameter()
{
    // parameter.getValue() returns the normalised [0, 1] value.  Convert it
    // back to an integer waveform index.
    int waveformIndex = static_cast<int> (std::round (parameter.convertFrom0to1 (parameter.getValue())));
    setSelectedButton (waveformIndex);
}

void WaveformSelector::setSelectedButton (int waveformIndex)
{
    const juce::ScopedValueSetter<bool> svs (ignoreCallbacks, true);

    // Update the visual toggle state without sending notifications: a parameter
    // change already drove this update, so we must not fire buttonClicked again.
    sineButton.setToggleState   (waveformIndex == 0, juce::dontSendNotification);
    sawButton.setToggleState    (waveformIndex == 1, juce::dontSendNotification);
    squareButton.setToggleState (waveformIndex == 2, juce::dontSendNotification);
}

void WaveformSelector::setParameterFromButton (int waveformIndex)
{
    const juce::ScopedValueSetter<bool> svs (ignoreCallbacks, true);

    float normalisedValue = parameter.convertTo0to1 (static_cast<float> (waveformIndex));
    parameter.setValueNotifyingHost (normalisedValue);

    setSelectedButton (waveformIndex);
}

} // namespace gui
