/*
    PluginEditor.cpp composes the synthesizer UI from dedicated child
    components.  Each component owns its own controls, layout and parameter
    attachments.  The editor only arranges the top-level components with a
    juce::Grid.
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      carrierPanel (processorRef.getParameters(), "Carrier", "carrierRatio", "carrierWaveform", 1001),
      fmAmountComponent (processorRef.getParameters(), "fmAmount"),
      modulatorPanel (processorRef.getParameters(), "Modulator", "modulatorRatio", "modulatorWaveform", 1002),
      adsrPanel (processorRef.getParameters())
{
    titleLabel.setText ("SmolFM", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (carrierPanel);
    addAndMakeVisible (fmAmountComponent);
    addAndMakeVisible (modulatorPanel);
    addAndMakeVisible (adsrPanel);

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
    juce::Grid grid;
    grid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),    // title
                          juce::Grid::TrackInfo (juce::Grid::Fr (5)),    // oscillators + FM amount
                          juce::Grid::TrackInfo (juce::Grid::Fr (3)) };  // ADSR
    grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (3)),   // carrier
                               juce::Grid::TrackInfo (juce::Grid::Fr (2)),   // FM amount
                               juce::Grid::TrackInfo (juce::Grid::Fr (3)) }; // modulator
    grid.rowGap = juce::Grid::Px (16);
    grid.columnGap = juce::Grid::Px (16);

    // Title spans all three columns in the first row.
    grid.items.add (juce::GridItem (titleLabel).withArea (1, juce::GridItem::Span (3)));

    // Middle row: Carrier, FM Amount, Modulator.
    grid.items.add (juce::GridItem (carrierPanel).withArea (2, 1));
    grid.items.add (juce::GridItem (fmAmountComponent).withArea (2, 2));
    grid.items.add (juce::GridItem (modulatorPanel).withArea (2, 3));

    // ADSR spans all three columns in the third row.
    grid.items.add (juce::GridItem (adsrPanel).withArea (3, juce::GridItem::Span (3)));

    grid.performLayout (getLocalBounds().reduced (24));
}

