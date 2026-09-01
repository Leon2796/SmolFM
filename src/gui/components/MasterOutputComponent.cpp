/*
    MasterOutputComponent implementation.
*/

#include "MasterOutputComponent.h"
#include "SliderUtils.h"

namespace gui
{

MasterOutputComponent::MasterOutputComponent (juce::AudioProcessorValueTreeState& apvts,
                                              const juce::String& parameterID,
                                              std::function<float()> peakProviderIn)
    : peakProvider (std::move (peakProviderIn))
{
    titleLabel.setText ("Output", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    configureRotarySlider (levelSlider, "");
    levelSlider.setRange (0.0, 1.0, 0.001);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (levelSlider);

    levelAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts,
                                                                                     parameterID,
                                                                                     levelSlider));

    startTimerHz (30);
}

MasterOutputComponent::~MasterOutputComponent()
{
    stopTimer();
}

void MasterOutputComponent::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    titleLabel.setBounds (bounds.removeFromTop (20));
    bounds.removeFromTop (4);

    // Meter strip on the right, slider takes the rest.
    const int meterWidth = 14;
    meterBounds = bounds.removeFromRight (meterWidth);
    levelSlider.setBounds (bounds);
}

void MasterOutputComponent::paint (juce::Graphics& g)
{
    // Meter background.
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRect (meterBounds);

    // Peak bar, bottom-up, clamped to [0, 1].
    const float level = juce::jlimit (0.0f, 1.0f, displayedPeak);
    const int barHeight = static_cast<int> (meterBounds.getHeight() * level);

    g.setColour (level > 0.9f ? juce::Colours::red
                 : level > 0.6f ? juce::Colours::yellow
                                : juce::Colours::lightblue);
    g.fillRect (meterBounds.getX(),
                meterBounds.getBottom() - barHeight,
                meterBounds.getWidth(),
                barHeight);
}

void MasterOutputComponent::timerCallback()
{
    if (peakProvider != nullptr)
    {
        const float p = peakProvider();
        // Decay in the UI too, so the bar falls smoothly between polls.
        displayedPeak = p > displayedPeak ? p : displayedPeak * 0.85f;
        repaint();
    }
}

} // namespace gui
