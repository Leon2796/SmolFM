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
      boundsConstrainer (std::make_unique<juce::ComponentBoundsConstrainer>()),
      resizer (this, boundsConstrainer.get())
{
    titleLabel.setText ("SmolFM", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (graphPanel);

    boundsConstrainer->setMinimumSize (700, 500);
    boundsConstrainer->setMaximumSize (2400, 1600);

    addAndMakeVisible (resizer);

    // Tell hosts (especially the Standalone wrapper) that this editor has a
    // preferred size it can be resized from.
    setResizable (true, true);

    // -----------------------------------------------------------------
    // Build the five boxes we show in the graph view.  Content is the same
    // panels as before, so controls still work as usual.
    //
    // IMPORTANT: add the box to the panel FIRST, then add pins.  Pin
    // construction needs the panel as a parent so its component tree sits
    // under the right JUCE hierarchy.
    // -----------------------------------------------------------------

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
        "fm", "FM Amount",
        std::make_unique<gui::FmAmountComponent> (processorRef.getParameters(), "fmAmount"));
    auto note = std::make_unique<gui::DraggableComponent> (
        "note", "Note In",
        std::make_unique<gui::NoteNodeComponent>());
    auto adsr = std::make_unique<gui::DraggableComponent> (
        "adsr", "ADSR",
        std::make_unique<gui::AdsrPanel> (processorRef.getParameters()));

    // -----------------------------------------------------------------
    // Wire the UI into the processor graph, then add the boxes so they have
    // a parent before any pin is attached.
    // -----------------------------------------------------------------

    graphPanel.onConnectionPatchChanged = [this] (const smolfm::ConnectionPatch& patch)
    {
        processorRef.applyConnectionPatch (patch);
    };

    auto& noteRef      = graphPanel.addComponent (std::move (note));
    auto& carrierRef   = graphPanel.addComponent (std::move (carrier));
    auto& fmRef        = graphPanel.addComponent (std::move (fmAmount));
    auto& modulatorRef = graphPanel.addComponent (std::move (modulator));
    auto& adsrRef      = graphPanel.addComponent (std::move (adsr));

    // Now that the boxes live inside the panel, attach the pins.  The ids
    // match GraphNodes.h ("note.out", "carrier.note_in", ...) so the panel
    // can resolve them when the user drags a wire.
    //
    // Carrier and Modulator are the same node shape: one note_in (frequency)
    // and one signal out.  FM Amount takes two signals and puts one out.
    noteRef.addOutputPin     ("out",          smolfm::PortType::frequency);
    carrierRef.addInputPin   ("note_in",      smolfm::PortType::frequency);
    carrierRef.addOutputPin  ("out",          smolfm::PortType::signal);
    modulatorRef.addInputPin ("note_in",      smolfm::PortType::frequency);
    modulatorRef.addOutputPin("out",          smolfm::PortType::signal);
    fmRef.addInputPin        ("carrier_in",   smolfm::PortType::signal);
    fmRef.addInputPin        ("modulator_in", smolfm::PortType::signal);
    fmRef.addOutputPin       ("out",          smolfm::PortType::signal);
    adsrRef.addInputPin      ("in",           smolfm::PortType::signal);

    constexpr int margin = 16;
    constexpr int spacing = 32;

    noteRef.setTopLeftPosition     (margin, margin);
    carrierRef.setTopLeftPosition  (margin + spacing + carrierRef.getWidth(), margin);
    fmRef.setTopLeftPosition       (margin + 2 * (spacing + fmRef.getWidth()), margin);
    modulatorRef.setTopLeftPosition(margin + 3 * (spacing + modulatorRef.getWidth()), margin);
    adsrRef.setTopLeftPosition     (margin, margin + 200);

    // -----------------------------------------------------------------
    // Default wiring — the classic FM chain through the new node shapes:
    //   note.out   -> carrier.note_in     (MIDI note drives carrier hz)
    //   carrier.out -> fm.carrier_in      (carrier waveform to be modulated)
    //   modulator.out -> fm.modulator_in  (modulator waveform)
    //   fm.out     -> adsr.in             (envelope shapes the FM result)
    //
    // loadLayout() may replace this entirely from the saved patch.
    // -----------------------------------------------------------------
    smolfm::ConnectionPatch defaultPatch;
    defaultPatch.connections.push_back ({ { "note",      "out" }, { "carrier", "note_in" } });
    defaultPatch.connections.push_back ({ { "carrier",   "out" }, { "fm", "carrier_in" } });
    defaultPatch.connections.push_back ({ { "modulator", "out" }, { "fm", "modulator_in" } });
    defaultPatch.connections.push_back ({ { "fm",        "out" }, { "adsr", "in" } });
    graphPanel.applyPatch (defaultPatch);

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

    graphPanel.setBounds (bounds);

    // Park the resize handle in the bottom-right corner.
    resizer.setBounds (getWidth() - 24, getHeight() - 24, 24, 24);

    // Once the graph panel has its real bounds we can restore any saved layout
    // without clamping boxes off-screen.
    graphPanel.loadLayout();
}

