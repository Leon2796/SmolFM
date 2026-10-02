/*
    ResonatorComponent implementation.
*/

#include "ResonatorComponent.h"

namespace gui
{

ResonatorComponent::ResonatorComponent (juce::AudioProcessorValueTreeState& apvts,
                                        const juce::String& tuneParameterID,
                                        const juce::String& qParameterID,
                                        const juce::String& decayParameterID,
                                        const juce::String& velSpreadParameterID)
{
    // One rotary per parameter, label above the knob (DelayComponent style).
    auto setupKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.attachToComponent (&s, false);
        addAndMakeVisible (l);
    };

    setupKnob (tuneSlider, tuneLabel, "Tune");
    setupKnob (qSlider, qLabel, "Q");
    setupKnob (decaySlider, decayLabel, "Decay");
    setupKnob (spreadSlider, spreadLabel, "VelSpr");

    tuneAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, tuneParameterID, tuneSlider);
    qAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, qParameterID, qSlider);
    decayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, decayParameterID, decaySlider);
    spreadAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, velSpreadParameterID, spreadSlider);

    // Compact content size: keeps the resonator node box small (four knobs
    // in one row).
    setSize (260, 160);
}

void ResonatorComponent::resized()
{
    auto r = getLocalBounds().reduced (8);

    const int w = r.getWidth() / 4;
    tuneSlider   .setBounds (r.removeFromLeft (w).reduced (4).withTrimmedTop (14));
    qSlider      .setBounds (r.removeFromLeft (w).reduced (4).withTrimmedTop (14));
    decaySlider  .setBounds (r.removeFromLeft (w).reduced (4).withTrimmedTop (14));
    spreadSlider .setBounds (r.reduced (4).withTrimmedTop (14));
}

} // namespace gui