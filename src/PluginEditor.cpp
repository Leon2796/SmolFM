/*
    PluginEditor.cpp composes the synthesizer UI from dedicated child
    components.  Each component owns its own controls, layout and parameter
    attachments.  The editor only arranges the top-level components with a
    juce::Grid.
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "graph/SmolFmFile.h"
#include "gui/components/OscillatorPanel.h"
#include "gui/components/FMModulationComponent.h"
#include "gui/components/FrequencyScaleComponent.h"
#include "gui/components/AdsrPanel.h"
#include "gui/components/NoteNodeComponent.h"

namespace
{
    // Node-type content factories.  Each produces the panel a box needs, bound
    // to the instance's own parameter ids (via GraphNodeRegistry).

    std::unique_ptr<juce::Component> makeOscillatorContent (const juce::String& instanceId,
                                                            juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::OscillatorPanel> (
            apvts, "Oscillator",
            smolfm::GraphNodeRegistry::waveformParameterIdFor  (instanceId));
    }

    std::unique_ptr<juce::Component> makeFmContent (const juce::String& instanceId,
                                                    juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::FMModulationComponent> (
            apvts, smolfm::GraphNodeRegistry::amountParameterIdFor (instanceId));
    }

    std::unique_ptr<juce::Component> makeFrequencyScaleContent (const juce::String& instanceId,
                                                                juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::FrequencyScaleComponent> (
            apvts, smolfm::GraphNodeRegistry::amountParameterIdFor (instanceId));
    }

    std::unique_ptr<juce::Component> makeAdsrContent (const juce::String&,
                                                      juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::AdsrPanel> (apvts);
    }

    std::unique_ptr<juce::Component> makeNoteContent (const juce::String&,
                                                      juce::AudioProcessorValueTreeState&)
    {
        return std::make_unique<gui::NoteNodeComponent>();
    }
}

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
    addAndMakeVisible (patchBrowser);
    addAndMakeVisible (exportButton);
    addAndMakeVisible (importButton);

    exportButton.onClick = [this] { exportPatch(); };
    importButton.onClick = [this] { importPatch(); };

    // Patch browser: directory lives in the processor state; selection loads
    // the chosen patch into the graph panel.
    patchBrowser.onDirectoryChosen = [this] (const juce::File& dir)
    {
        processorRef.setPatchDirectory (dir);
        patchBrowser.setDirectory (dir);
    };
    patchBrowser.onPatchSelected = [this] (const juce::File& file)
    {
        if (smolfm::SmolFmFile::load (graphPanel, processorRef.getParameters(), file))
            patchBrowser.setInstrumentName (smolfm::SmolFmFile::readInstrumentName (file));
    };
    patchBrowser.setDirectory (processorRef.getPatchDirectory());

    boundsConstrainer->setMinimumSize (700, 500);
    boundsConstrainer->setMaximumSize (2400, 1600);
    addAndMakeVisible (resizer);
    setResizable (true, true);

    // -----------------------------------------------------------------
    // Toolbar palette: one tile per node type, showing how many are left.
    // -----------------------------------------------------------------
    oscButton  .configure ("osc",  "Osc",   [] (const juce::Rectangle<float>& r)
                           {   juce::Path p; p.addEllipse (r.reduced (1.0f)); return p; },
                           smolfm::GraphNodeRegistry::maxOscillators);
    fmButton   .configure ("fm",   "FM",    [] (const juce::Rectangle<float>& r)
                           {   juce::Path p; p.startNewSubPath (r.getX(), r.getCentreY());
                               p.cubicTo (r.getX() + r.getWidth() * 0.3f, r.getY(),
                                          r.getRight() - r.getWidth() * 0.3f, r.getBottom(), r.getRight(), r.getCentreY());
                               return p; },
                           smolfm::GraphNodeRegistry::maxFmAmounts);
    scaleButton.configure ("fscale", "Scale", [] (const juce::Rectangle<float>& r)
                           {   juce::Path p; const float c = r.getCentreY();
                               p.startNewSubPath (r.getX(), c); p.lineTo (r.getRight(), r.getY());
                               p.startNewSubPath (r.getX(), c); p.lineTo (r.getRight(), c);
                               p.startNewSubPath (r.getX(), c); p.lineTo (r.getRight(), r.getBottom());
                               return p; },
                           smolfm::GraphNodeRegistry::maxFrequencyScales);
    adsrButton .configure ("adsr", "ADSR",  [] (const juce::Rectangle<float>& r)
                           {   juce::Path p; p.startNewSubPath (r.getX(), r.getBottom());
                               p.lineTo (r.getX() + r.getWidth() * 0.25f, r.getY());
                               p.lineTo (r.getX() + r.getWidth() * 0.5f, r.getCentreY());
                               p.lineTo (r.getX() + r.getWidth() * 0.75f, r.getCentreY());
                               p.lineTo (r.getRight(), r.getBottom()); return p; },
                           smolfm::GraphNodeRegistry::maxAdsr);
    noteButton .configure ("note", "Note",  [] (const juce::Rectangle<float>& r)
                           {   juce::Path p; const float cx = r.getCentreX(), cy = r.getCentreY();
                               p.startNewSubPath (cx, cy); p.lineTo (cx, r.getY());
                               p.startNewSubPath (cx - 4, cy); p.addEllipse (cx - 8, cy - 6, 10, 8);
                               return p; },
                           smolfm::GraphNodeRegistry::maxNotes);

    for (auto* b : { &oscButton, &fmButton, &scaleButton, &adsrButton, &noteButton })
    {
        addAndMakeVisible (*b);
        b->onAddRequested = [this] (const juce::String& baseId) { addNodeFromToolbar (baseId); };
    }

    // -----------------------------------------------------------------
    // Wire the panel: patch changes go to the processor, node-set changes
    // refresh the toolbar budgets.
    // -----------------------------------------------------------------
    graphPanel.onConnectionPatchChanged = [this] (const smolfm::ConnectionPatch& patch)
    {
        processorRef.applyConnectionPatch (patch);
    };

    graphPanel.onNodeSetChanged = [this] { refreshToolbarBadges(); };

    // SmolFmFile::load calls this when the XML references a node that isn't
    // currently on the canvas (e.g. "osc4").
    graphPanel.onCreateMissingNode = [this] (const juce::String& instanceId)
        -> gui::DraggableComponent*
    {
        const juce::String baseId = smolfm::GraphNodeRegistry::baseIdOf (instanceId);

        // Reuse the same factories the toolbar uses.
        if (baseId == "osc")
            return graphPanel.addNodeOfType ("osc",  processorRef.getParameters(), makeOscillatorContent);
        if (baseId == "fm")
            return graphPanel.addNodeOfType ("fm",   processorRef.getParameters(), makeFmContent);
        if (baseId == "fscale")
            return graphPanel.addNodeOfType ("fscale", processorRef.getParameters(), makeFrequencyScaleContent);
        if (baseId == "adsr")
            return graphPanel.addNodeOfType ("adsr", processorRef.getParameters(), makeAdsrContent);
        if (baseId == "note")
            return graphPanel.addNodeOfType ("note", processorRef.getParameters(), makeNoteContent);

        return nullptr;
    };

    // -----------------------------------------------------------------
    // The canvas starts empty: no seeded nodes, no default patch.  Nodes
    // appear only via the palette or a loaded .smolfm file.
    // -----------------------------------------------------------------
    refreshToolbarBadges();

    setSize (1200, 800);
}

void AudioPluginAudioProcessorEditor::addNodeFromToolbar (const juce::String& baseId)
{
    using ContentFactory = std::function<std::unique_ptr<juce::Component> (const juce::String&,
                                                                           juce::AudioProcessorValueTreeState&)>;

    static const std::map<juce::String, ContentFactory> factories
    {
        { "note",   makeNoteContent    },
        { "osc",    makeOscillatorContent },
        { "fm",     makeFmContent      },
        { "fscale", makeFrequencyScaleContent },
        { "adsr",   makeAdsrContent    }
    };

    const auto it = factories.find (baseId);
    if (it == factories.end())
        return;

    graphPanel.addNodeOfType (baseId, processorRef.getParameters(), it->second);
}

void AudioPluginAudioProcessorEditor::refreshToolbarBadges()
{
    oscButton  .setRemaining (smolfm::GraphNodeRegistry::maxOscillators     - graphPanel.countBoxesOfType ("osc"));
    fmButton   .setRemaining (smolfm::GraphNodeRegistry::maxFmAmounts       - graphPanel.countBoxesOfType ("fm"));
    scaleButton.setRemaining (smolfm::GraphNodeRegistry::maxFrequencyScales - graphPanel.countBoxesOfType ("fscale"));
    adsrButton .setRemaining (smolfm::GraphNodeRegistry::maxAdsr            - graphPanel.countBoxesOfType ("adsr"));
    noteButton .setRemaining (smolfm::GraphNodeRegistry::maxNotes           - graphPanel.countBoxesOfType ("note"));
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

    // Toolbar: title left, palette center, import/export right.
    auto toolbar = bounds.removeFromTop (34);
    titleLabel.setBounds (toolbar.removeFromLeft (110));

    auto buttonsArea = toolbar.removeFromRight (toolbar.getWidth() > 420 ? 400 : toolbar.getWidth() / 2);
    importButton.setBounds (buttonsArea.removeFromRight (84).reduced (2));
    exportButton.setBounds (buttonsArea.removeFromRight (84).reduced (2));

    // Palette tiles, 5 × 62 px each.
    auto palette = toolbar.reduced (4, 2);
    const int tileWidth = 62;
    oscButton  .setBounds (palette.removeFromLeft (tileWidth));
    fmButton   .setBounds (palette.removeFromLeft (tileWidth));
    scaleButton.setBounds (palette.removeFromLeft (tileWidth));
    adsrButton .setBounds (palette.removeFromLeft (tileWidth));
    noteButton .setBounds (palette.removeFromLeft (tileWidth));

    bounds.removeFromTop (8);

    // Patch browser: directory row + navigation above the graph canvas.
    patchBrowser.setBounds (bounds.removeFromTop (64));
    bounds.removeFromTop (4);

    graphPanel.setBounds (bounds);

    // Park the resize handle in the bottom-right corner.
    resizer.setBounds (getWidth() - 24, getHeight() - 24, 24, 24);

    // Once the graph panel has its real bounds we can restore any saved layout
    // without clamping boxes off-screen.
    graphPanel.loadLayout();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::exportPatch()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Export SmolFM patch",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.smolfm");

    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode
                            | juce::FileBrowserComponent::canSelectFiles
                            | juce::FileBrowserComponent::warnAboutOverwriting,
        [this] (const juce::FileChooser& chooser)
        {
            const juce::File target = chooser.getResult();
            if (target == juce::File())
                return;

            smolfm::SmolFmFile::save (graphPanel, processorRef.getParameters(), target);
        });
}

void AudioPluginAudioProcessorEditor::importPatch()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Import SmolFM patch",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.smolfm");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& chooser)
        {
            const juce::File source = chooser.getResult();
            if (source == juce::File() || ! source.existsAsFile())
                return;

            smolfm::SmolFmFile::load (graphPanel, processorRef.getParameters(), source);
            patchBrowser.setInstrumentName (smolfm::SmolFmFile::readInstrumentName (source));
            patchBrowser.selectPatch (source);
        });
}

