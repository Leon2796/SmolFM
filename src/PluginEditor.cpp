/*
    PluginEditor.cpp builds and lays out the synthesizer UI.

    All controls are attached to AudioProcessorValueTreeState parameters.  The
    attachments own the connection, so no manual value-transfer code is needed.

    Waveform selection uses ToggleButtons in radio groups.  Each oscillator has
    its own AudioParameterChoice (Sine = 0, Saw = 1, Square = 2).  A small
    WaveformSelector class keeps the three radio buttons and the Choice
    parameter synchronized, because JUCE's ButtonParameterAttachment only
    supports boolean parameters, not Choice parameters.
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    /**
        Helper to configure a toggle button as part of a radio group.

        Radio group IDs make JUCE automatically deselect the other buttons in
        the same group when one is selected.
    */
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

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    auto& apvts = processorRef.getParameters();

    // --- Sliders --------------------------------------------------------------
    configureSlider (carrierRatioSlider,  "x");
    configureSlider (modulatorRatioSlider, "x");
    configureSlider (fmAmountSlider,      " rad");
    configureSlider (attackSlider,        " s");
    configureSlider (decaySlider,         " s");
    configureSlider (sustainSlider,       "");
    configureSlider (releaseSlider,       " s");

    // --- Labels ---------------------------------------------------------------
    carrierLabel.setText ("Carrier",  juce::dontSendNotification);
    modulatorLabel.setText ("Modulator", juce::dontSendNotification);
    fmLabel.setText ("FM Amount", juce::dontSendNotification);
    adsrLabel.setText ("ADSR", juce::dontSendNotification);

    carrierLabel.setJustificationType  (juce::Justification::centred);
    modulatorLabel.setJustificationType (juce::Justification::centred);
    fmLabel.setJustificationType (juce::Justification::centred);
    adsrLabel.setJustificationType (juce::Justification::centred);

    // --- Waveform buttons -----------------------------------------------------
    configureWaveformButton (carrierSineButton,   "Sine",   1001);
    configureWaveformButton (carrierSawButton,    "Saw",    1001);
    configureWaveformButton (carrierSquareButton, "Square", 1001);

    configureWaveformButton (modulatorSineButton, "Sine",   1002);
    configureWaveformButton (modulatorSawButton,  "Saw",    1002);
    configureWaveformButton (modulatorSquareButton, "Square", 1002);

    // --- Add components to the editor -----------------------------------------
    addAndMakeVisible (carrierLabel);
    addAndMakeVisible (modulatorLabel);
    addAndMakeVisible (fmLabel);
    addAndMakeVisible (adsrLabel);

    addAndMakeVisible (carrierRatioSlider);
    addAndMakeVisible (modulatorRatioSlider);
    addAndMakeVisible (fmAmountSlider);

    addAndMakeVisible (attackSlider);
    addAndMakeVisible (decaySlider);
    addAndMakeVisible (sustainSlider);
    addAndMakeVisible (releaseSlider);

    addAndMakeVisible (carrierSineButton);
    addAndMakeVisible (carrierSawButton);
    addAndMakeVisible (carrierSquareButton);

    addAndMakeVisible (modulatorSineButton);
    addAndMakeVisible (modulatorSawButton);
    addAndMakeVisible (modulatorSquareButton);

    // --- Attachments ----------------------------------------------------------
    // Attachments must be created after the component is added and configured.
    // They bind each UI control to the corresponding APVTS parameter.
    carrierRatioAttachment.reset  (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, "carrierRatio",  carrierRatioSlider));
    modulatorRatioAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, "modulatorRatio", modulatorRatioSlider));
    fmAmountAttachment.reset    (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, "fmAmount",      fmAmountSlider));

    attackAttachment.reset  (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, "attack",  attackSlider));
    decayAttachment.reset   (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, "decay",   decaySlider));
    sustainAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, "sustain", sustainSlider));
    releaseAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, "release", releaseSlider));

    // Waveform selectors keep the radio buttons and Choice parameters in sync.
    carrierWaveformSelector.reset  (new WaveformSelector (apvts, "carrierWaveform",
                                                          carrierSineButton, carrierSawButton, carrierSquareButton));
    modulatorWaveformSelector.reset(new WaveformSelector (apvts, "modulatorWaveform",
                                                          modulatorSineButton, modulatorSawButton, modulatorSquareButton));

    carrierWaveformSelector->sendInitialUpdate();
    modulatorWaveformSelector->sendInitialUpdate();

    // Set the plugin window size.  Make it larger so sliders and labels are easy
    // to read and operate.
    setSize (1200, 800);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill the editor background with the default JUCE background colour.
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (24);

    // Top title area.
    auto titleArea = bounds.removeFromTop (40);
    juce::ignoreUnused (titleArea);

    // Carrier and modulator columns.
    auto oscArea = bounds.removeFromTop (250);
    auto carrierArea = oscArea.removeFromLeft (oscArea.getWidth() / 2);
    auto modulatorArea = oscArea;

    // Carrier column.
    carrierLabel.setBounds (carrierArea.removeFromTop (25));
    carrierRatioSlider.setBounds (carrierArea.removeFromTop (120));

    auto carrierButtonRow = carrierArea.removeFromTop (40);
    carrierSineButton.setBounds   (carrierButtonRow.removeFromLeft (carrierButtonRow.getWidth() / 3));
    carrierSawButton.setBounds    (carrierButtonRow.removeFromLeft (carrierButtonRow.getWidth() / 2));
    carrierSquareButton.setBounds (carrierButtonRow);

    // Modulator column.
    modulatorLabel.setBounds (modulatorArea.removeFromTop (25));
    modulatorRatioSlider.setBounds (modulatorArea.removeFromTop (120));

    auto modulatorButtonRow = modulatorArea.removeFromTop (40);
    modulatorSineButton.setBounds   (modulatorButtonRow.removeFromLeft (modulatorButtonRow.getWidth() / 3));
    modulatorSawButton.setBounds    (modulatorButtonRow.removeFromLeft (modulatorButtonRow.getWidth() / 2));
    modulatorSquareButton.setBounds (modulatorButtonRow);

    // FM Amount row.
    auto fmArea = bounds.removeFromTop (90);
    fmLabel.setBounds (fmArea.removeFromTop (25));
    fmAmountSlider.setBounds (fmArea.reduced (60, 0));

    // ADSR row.
    auto adsrArea = bounds;
    adsrLabel.setBounds (adsrArea.removeFromTop (25));

    auto adsrSliderArea = adsrArea.removeFromTop (120);
    int adsrWidth = adsrSliderArea.getWidth() / 4;
    attackSlider.setBounds  (adsrSliderArea.removeFromLeft (adsrWidth));
    decaySlider.setBounds   (adsrSliderArea.removeFromLeft (adsrWidth));
    sustainSlider.setBounds (adsrSliderArea.removeFromLeft (adsrWidth));
    releaseSlider.setBounds (adsrSliderArea);
}

void AudioPluginAudioProcessorEditor::configureSlider (juce::Slider& slider,
                                                         const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 24);
    slider.setTextValueSuffix (suffix);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::lightblue);
}
