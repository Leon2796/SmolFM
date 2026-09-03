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
#include "gui/components/MasterOutputComponent.h"
#include "gui/components/RingModulatorComponent.h"
#include "gui/components/AmComponent.h"

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

    std::unique_ptr<juce::Component> makeAdsrContent (const juce::String& instanceId,
                                                      juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::AdsrPanel> (
            apvts,
            smolfm::GraphNodeRegistry::adsrParameterIdFor (instanceId, "Attack"),
            smolfm::GraphNodeRegistry::adsrParameterIdFor (instanceId, "Decay"),
            smolfm::GraphNodeRegistry::adsrParameterIdFor (instanceId, "Sustain"),
            smolfm::GraphNodeRegistry::adsrParameterIdFor (instanceId, "Release"));
    }

        std::unique_ptr<juce::Component> makeNoteContent (const juce::String&,
                                                      juce::AudioProcessorValueTreeState&)
    {
        return std::make_unique<gui::NoteNodeComponent>();
    }

        std::unique_ptr<juce::Component> makeRingModulatorContent (const juce::String&,
                                                               juce::AudioProcessorValueTreeState&)
    {
        return std::make_unique<gui::RingModulatorComponent>();
    }

    std::unique_ptr<juce::Component> makeAmContent (const juce::String& instanceId,
                                                    juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::AmComponent> (
            apvts, smolfm::GraphNodeRegistry::amountParameterIdFor (instanceId));
    }
}

std::unique_ptr<juce::Component> AudioPluginAudioProcessorEditor::makeOutputContent (const juce::String& instanceId,
                                                                                     juce::AudioProcessorValueTreeState& apvts)
{
    return std::make_unique<gui::MasterOutputComponent> (
        apvts,
        smolfm::GraphNodeRegistry::levelParameterIdFor (instanceId),
        [this] { return processorRef.getMasterPeakLevel(); });
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
    
    // Wrap the graph panel in a scrollable viewport so large patches
    // can be navigated even when they exceed the window size.
    graphViewport.setScrollBarsShown (true, true);  // vertical + horizontal
    graphViewport.setScrollBarThickness (12);
    graphViewport.setViewedComponent (&graphPanel, false);  // false = don't delete
    addAndMakeVisible (graphViewport);
    
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
        {
            patchBrowser.setInstrumentName (smolfm::SmolFmFile::readInstrumentName (file));
            fitWindowToContent();
        }
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
    outputButton.configure ("output", "Out", [] (const juce::Rectangle<float>& r)
                           {   juce::Path p; p.addRectangle (r.reduced (2.0f)); return p; },
                           smolfm::GraphNodeRegistry::maxMasterOutputs);

        ringButton .configure ("ring", "Ring",  [] (const juce::Rectangle<float>& r)
                           {   juce::Path p; p.addEllipse (r.reduced (1.0f));
                               const float cx = r.getCentreX(), cy = r.getCentreY();
                               p.startNewSubPath (cx - 5, cy - 5); p.lineTo (cx + 5, cy + 5);
                               p.startNewSubPath (cx + 5, cy - 5); p.lineTo (cx - 5, cy + 5);
                               return p; },
                           smolfm::GraphNodeRegistry::maxRingModulators);

    amButton   .configure ("am",   "AM",    [] (const juce::Rectangle<float>& r)
                           {   juce::Path p;
                               // Modulated sine icon: a wave whose amplitude tapers
                               const float cx = r.getCentreX();
                               const float cy = r.getCentreY();
                               p.startNewSubPath (r.getX(), cy);
                               p.quadraticTo (cx - r.getWidth() * 0.15f, r.getY() + 2.0f, cx, cy);
                               p.quadraticTo (cx + r.getWidth() * 0.15f, r.getBottom() - 2.0f, r.getRight(), cy);
                               return p; },
                           smolfm::GraphNodeRegistry::maxAmModulators);

    for (auto* b : { &oscButton, &fmButton, &scaleButton, &adsrButton, &noteButton, &ringButton, &amButton, &outputButton })
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

                // Reuse the same factories the toolbar uses.  Patch-loaded nodes
        // stay invisible until updateVisibilityFromConnections() shows
        // only the ones that are actively wired.
        constexpr bool nodeStartsHidden = false;   // makeVisible parameter
        if (baseId == "osc")
            return graphPanel.addNodeOfType ("osc",  processorRef.getParameters(), makeOscillatorContent, nodeStartsHidden);
        if (baseId == "fm")
            return graphPanel.addNodeOfType ("fm",   processorRef.getParameters(), makeFmContent, nodeStartsHidden);
        if (baseId == "fscale")
            return graphPanel.addNodeOfType ("fscale", processorRef.getParameters(), makeFrequencyScaleContent, nodeStartsHidden);
        if (baseId == "adsr")
            return graphPanel.addNodeOfType ("adsr", processorRef.getParameters(), makeAdsrContent, nodeStartsHidden);
        if (baseId == "note")
            return graphPanel.addNodeOfType ("note", processorRef.getParameters(), makeNoteContent, nodeStartsHidden);
                if (baseId == "ring")
            return graphPanel.addNodeOfType ("ring", processorRef.getParameters(), makeRingModulatorContent, nodeStartsHidden);
        if (baseId == "am")
            return graphPanel.addNodeOfType ("am", processorRef.getParameters(), makeAmContent, nodeStartsHidden);
        if (baseId == "output")
            return graphPanel.addNodeOfType ("output", processorRef.getParameters(),
                                             [this] (const juce::String& id, juce::AudioProcessorValueTreeState& a)
                                             { return makeOutputContent (id, a); },
                                             nodeStartsHidden);

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
        { "adsr",   makeAdsrContent    },
        { "ring",   makeRingModulatorContent },
        { "am",     makeAmContent        }
    };

    if (baseId == "output")
    {
        graphPanel.addNodeOfType (baseId, processorRef.getParameters(),
                                  [this] (const juce::String& id, juce::AudioProcessorValueTreeState& a)
                                  { return makeOutputContent (id, a); });
        return;
    }

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
    adsrButton  .setRemaining (smolfm::GraphNodeRegistry::maxAdsr            - graphPanel.countBoxesOfType ("adsr"));
    noteButton  .setRemaining (smolfm::GraphNodeRegistry::maxNotes           - graphPanel.countBoxesOfType ("note"));
    ringButton  .setRemaining (smolfm::GraphNodeRegistry::maxRingModulators   - graphPanel.countBoxesOfType ("ring"));
    amButton    .setRemaining (smolfm::GraphNodeRegistry::maxAmModulators      - graphPanel.countBoxesOfType ("am"));
    outputButton.setRemaining (smolfm::GraphNodeRegistry::maxMasterOutputs   - graphPanel.countBoxesOfType ("output"));
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

    // Palette tiles, 6 × 62 px each.
    auto palette = toolbar.reduced (4, 2);
    const int tileWidth = 62;
    oscButton   .setBounds (palette.removeFromLeft (tileWidth));
    fmButton    .setBounds (palette.removeFromLeft (tileWidth));
    scaleButton .setBounds (palette.removeFromLeft (tileWidth));
    adsrButton  .setBounds (palette.removeFromLeft (tileWidth));
            noteButton .setBounds (palette.removeFromLeft (tileWidth));
    ringButton .setBounds (palette.removeFromLeft (tileWidth));
    amButton   .setBounds (palette.removeFromLeft (tileWidth));
    outputButton.setBounds (palette.removeFromLeft (tileWidth));

    bounds.removeFromTop (8);

    // Patch browser: directory row + navigation above the graph canvas.
    patchBrowser.setBounds (bounds.removeFromTop (64));
        bounds.removeFromTop (4);

    // The viewport gets the remaining space; the panel sizes itself to content.
    graphViewport.setBounds (bounds);
    
    // Tell the panel its ideal size based on content (it will be scrollable if larger).
    const auto content = graphPanel.getContentBounds();
    if (!content.isEmpty())
        graphPanel.setSize (juce::jmax (content.getRight(), bounds.getWidth()),
                            juce::jmax (content.getBottom(), bounds.getHeight()));
    else
        graphPanel.setSize (bounds.getWidth(), bounds.getHeight());

    // Park the resize handle in the bottom-right corner.
    resizer.setBounds (getWidth() - 24, getHeight() - 24, 24, 24);

    // Once the graph panel has its real bounds we can restore any saved layout
    // without clamping boxes off-screen.
    graphPanel.loadLayout();
}

void AudioPluginAudioProcessorEditor::fitWindowToContent()
{
    // The panel needs this much room for the loaded layout.
    const auto content = graphPanel.getContentBounds();
    if (content.isEmpty())
        return;

    // Fixed chrome: margins (24 each side) + toolbar (34) + gaps (8+4) +
    // patch browser (64).  The canvas sits below all of that.
    const int chromeTop    = 24 + 34 + 8 + 64 + 4;
    const int chromeRight  = 24;
    const int chromeBottom = 24;

    // content.getRight() is the panel-local right edge of the layout; the
    // window needs that plus the chrome around the canvas.
    const int neededW = content.getRight()  + chromeRight;
    const int neededH = content.getBottom() + chromeTop + chromeBottom;

        if (neededW <= getWidth() && neededH <= getHeight())
        return;   // everything already fits

    // Grow towards the needed size, but cap at a reasonable maximum.
    // The viewport inside will provide scrolling for larger layouts.
    const auto screen = juce::Desktop::getInstance().getDisplays()
                            .getPrimaryDisplay()->userArea;

    // Cap window growth to 90% of screen or current size + 400px, whichever is smaller.
    // This prevents the window from becoming unmanageably large while the
    // viewport handles scrolling for the remaining content.
    const int maxW = juce::jmin (screen.getWidth() * 9 / 10, getWidth() + 400);
    const int maxH = juce::jmin (screen.getHeight() * 9 / 10, getHeight() + 300);

    const int newW = juce::jmin (neededW, maxW);
    const int newH = juce::jmin (neededH, maxH);

    setSize (juce::jmax (getWidth(),  newW),
             juce::jmax (getHeight(), newH));
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
            fitWindowToContent();
        });
}

