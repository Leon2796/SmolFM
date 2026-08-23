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
      adsrPanel (processorRef.getParameters()),
      boundsConstrainer (std::make_unique<juce::ComponentBoundsConstrainer>()),
      resizer (this, boundsConstrainer.get())
{
    titleLabel.setText ("SmolFM", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (graphPanel);
    addAndMakeVisible (adsrPanel);

    // Constrain the window so the UI never collapses below a usable minimum
    // (enough room for two oscillator boxes plus the ADSR panel) and never
    // exceeds the host screen (JUCE clamps anyway, but keeping a sane ceiling
    // avoids silly saved states on multi-monitor setups).
    boundsConstrainer->setMinimumSize (700, 500);
    boundsConstrainer->setMaximumSize (2400, 1600);

    addAndMakeVisible (resizer);

    // Tell hosts (especially the Standalone wrapper) that this editor has a
    // preferred size it can be resized from.  Without this the corner handle
    // would show but the host window would stay glued to 1200x800.
    setResizable (true, true);

    // Signal-graph nodes.  The order here only decides the default placement
    // below; the user can rearrange freely afterwards.
    auto carrier = std::make_unique<gui::DraggableComponent> (
        "carrier", "Carrier",
        std::make_unique<gui::OscillatorPanel> (processorRef.getParameters(),
                                                "Carrier",
                                                "carrierFrequency",
                                                "carrierWaveform"));

    auto modulator = std::make_unique<gui::DraggableComponent> (
        "modulator", "Modulator",
        std::make_unique<gui::OscillatorPanel> (processorRef.getParameters(),
                                                "Modulator",
                                                "modulatorFrequency",
                                                "modulatorWaveform"));

    auto fmAmount = std::make_unique<gui::DraggableComponent> (
        "fmAmount", "FM Amount",
        std::make_unique<gui::FmAmountComponent> (processorRef.getParameters(), "fmAmount"));

    auto note = std::make_unique<gui::DraggableComponent> (
        "note", "Note In",
        std::make_unique<gui::NoteNodeComponent>());

    // Sensible default placement in a horizontal flow.  loadLayout() below
    // overrides these positions when a saved layout exists.
    auto& carrierRef = graphPanel.addComponent (std::move (carrier));
    auto& fmAmountRef = graphPanel.addComponent (std::move (fmAmount));
    auto& modulatorRef = graphPanel.addComponent (std::move (modulator));
    auto& noteRef = graphPanel.addComponent (std::move (note));

    constexpr int margin = 16;
    constexpr int spacing = 24;

    noteRef.setTopLeftPosition (margin, margin);
    carrierRef.setTopLeftPosition (margin + spacing + carrierRef.getWidth(), margin);
    fmAmountRef.setTopLeftPosition (margin + 2 * (spacing + fmAmountRef.getWidth()), margin);
    modulatorRef.setTopLeftPosition (margin + 3 * (spacing + modulatorRef.getWidth()), margin);

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

    titleLabel.setBounds (bounds.removeFromTop (32));
    bounds.removeFromTop (8);

    // Top half: draggable signal-graph (grows with the window).
    // Bottom half: ADSR (also grows, capped at 60% of the window height so it
    // never eats the graph panel).
    const int graphHeight = juce::jmax (150, bounds.getHeight() / 2 - 4);
    adsrPanel.setBounds (bounds.removeFromBottom (bounds.getHeight() - graphHeight));
    bounds.removeFromBottom (8);
    graphPanel.setBounds (bounds);

    // Park the resize handle in the bottom-right corner of the editor itself
    // (not of the content area) so it overlays the panel edges slightly.
    resizer.setBounds (getWidth() - 24, getHeight() - 24, 24, 24);

    // Once the graph panel has its real bounds we can restore any saved layout
    // without clamping boxes off-screen.  Calling loadLayout here (instead of
    // in the constructor) is also what makes standalone window resizing work.
    graphPanel.loadLayout();
}

