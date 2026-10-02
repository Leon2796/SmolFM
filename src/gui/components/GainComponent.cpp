/*
    GainComponent implementation.
*/

#include "GainComponent.h"
#include "SliderUtils.h"

namespace gui
{

GainComponent::GainComponent (juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& gainParameterID)
{
    configureRotarySlider (gainSlider, "x");
    addAndMakeVisible (gainSlider);

    gainLabel.setText ("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gainLabel);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, gainParameterID, gainSlider);

    // The param's NormalisableRange is lambda-based (interval == 0), so the
    // default text conversion prints 7 decimals ("0.4500000x").  Show the raw
    // factor with 2 decimals instead.  Typing and the bipolar drag mapping
    // (centre = 1.0x unity, left attenuates, right boosts) stay untouched:
    // valueFromTextFunction and the piecewise range are the attachment's.
    gainSlider.textFromValueFunction = [] (double value)
    {
        return juce::String (value, 2);
    };
    gainSlider.updateText();

    // Compact content size: DraggableComponent reads this to size the node
    // box, so a single gain knob does not fall back to the 220x260 default.
    setSize (100, 114);
}

void GainComponent::resized()
{
    auto r = getLocalBounds().reduced (4);
    gainLabel.setBounds (r.removeFromTop (18));
    r.removeFromTop (4);
    gainSlider.setBounds (r.withSizeKeepingCentre (juce::jmin (92, r.getWidth()), r.getHeight()));
}

} // namespace gui