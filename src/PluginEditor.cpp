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
#include "gui/components/FAdsrComponent.h"
#include "gui/components/GainComponent.h"
#include "gui/components/WaveshaperComponent.h"
#include "gui/components/FilterComponent.h"
#include "gui/components/NoteNodeComponent.h"
#include "gui/components/MasterOutputComponent.h"
#include "gui/components/RingModulatorComponent.h"
#include "gui/components/AmComponent.h"
#include "gui/components/DelayComponent.h"

namespace
{
    // Node-type content factories.  Each produces the panel a box needs, bound
    // to the instance's own parameter ids (via GraphNodeRegistry).

    std::unique_ptr<juce::Component> makeOscillatorContent (const juce::String& instanceId,
                                                            juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::OscillatorPanel> (
            apvts, "Oscillator",
            smolfm::GraphNodeRegistry::waveformParameterIdFor    (instanceId),
            smolfm::GraphNodeRegistry::oscModeParameterIdFor     (instanceId),
            smolfm::GraphNodeRegistry::oscStaticFreqParameterIdFor (instanceId),
            smolfm::GraphNodeRegistry::oscLfoRateParameterIdFor  (instanceId));
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

    std::unique_ptr<juce::Component> makeFAdsrContent (const juce::String& instanceId,
                                                       juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::FAdsrComponent> (
            apvts,
            smolfm::GraphNodeRegistry::fAdsrParameterIdFor (instanceId, "Attack"),
            smolfm::GraphNodeRegistry::fAdsrParameterIdFor (instanceId, "Decay"),
            smolfm::GraphNodeRegistry::fAdsrParameterIdFor (instanceId, "Sustain"),
            smolfm::GraphNodeRegistry::fAdsrParameterIdFor (instanceId, "Release"),
            smolfm::GraphNodeRegistry::fAdsrParameterIdFor (instanceId, "Up"),
            smolfm::GraphNodeRegistry::fAdsrParameterIdFor (instanceId, "Down"));
    }

    std::unique_ptr<juce::Component> makeGainContent (const juce::String& instanceId,
                                                      juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::GainComponent> (
            apvts, smolfm::GraphNodeRegistry::gainParameterIdFor (instanceId));
    }

    std::unique_ptr<juce::Component> makeWaveshaperContent (const juce::String& instanceId,
                                                           juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::WaveshaperComponent> (
            apvts,
            smolfm::GraphNodeRegistry::driveParameterIdFor (instanceId),
            smolfm::GraphNodeRegistry::shapeParameterIdFor (instanceId));
    }

    std::unique_ptr<juce::Component> makeFilterContent (const juce::String& instanceId,
                                                       juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::FilterComponent> (
            apvts,
            smolfm::GraphNodeRegistry::filterCutoffParameterIdFor    (instanceId),
            smolfm::GraphNodeRegistry::filterResonanceParameterIdFor (instanceId),
            smolfm::GraphNodeRegistry::filterModeParameterIdFor      (instanceId));
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

    std::unique_ptr<juce::Component> makeDelayContent (const juce::String& instanceId,
                                                       juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::DelayComponent> (
            apvts,
            smolfm::GraphNodeRegistry::delayTimeParameterIdFor     (instanceId),
            smolfm::GraphNodeRegistry::delayFeedbackParameterIdFor (instanceId),
            smolfm::GraphNodeRegistry::delayMixParameterIdFor      (instanceId),
            smolfm::GraphNodeRegistry::delaySyncParameterIdFor     (instanceId),
            smolfm::GraphNodeRegistry::delayDivisionParameterIdFor (instanceId));
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
        oscButton  .configure ("osc",   "Osc",  smolfm::GraphNodeRegistry::maxOscillators);
    fmButton   .configure ("fm",   "FM",   smolfm::GraphNodeRegistry::maxFmAmounts);
    scaleButton.configure ("fscale", "FSC",  smolfm::GraphNodeRegistry::maxFrequencyScales);
    adsrButton .configure ("adsr",  "ADSR", smolfm::GraphNodeRegistry::maxAdsr);
    fAdsrButton .configure ("fadsr", "FAD", smolfm::GraphNodeRegistry::maxFAdsr);
    gainButton .configure ("gain",  "GN",   smolfm::GraphNodeRegistry::maxGains);
    shapeButton .configure ("shape", "SHP",  smolfm::GraphNodeRegistry::maxWaveshapers);
    filterButton .configure ("filter", "FLT", smolfm::GraphNodeRegistry::maxFilters);
    noteButton .configure ("note",  "NT",   smolfm::GraphNodeRegistry::maxNotes);
    ringButton .configure ("ring",  "RNG",  smolfm::GraphNodeRegistry::maxRingModulators);
    amButton   .configure ("am",   "AM",   smolfm::GraphNodeRegistry::maxAmModulators);
    delayButton .configure ("delay", "DLY",  smolfm::GraphNodeRegistry::maxDelays);
    outputButton .configure ("output", "OUT", smolfm::GraphNodeRegistry::maxMasterOutputs);

        for (auto* b : { &oscButton, &fmButton, &scaleButton, &adsrButton, &fAdsrButton, &gainButton, &shapeButton, &filterButton, &noteButton, &ringButton, &amButton, &delayButton, &outputButton })
    {
        addAndMakeVisible (*b);
                b->onCountChanged = [this] (const juce::String& baseId, bool add)
        {
            if (add)
                addNodeFromToolbar (baseId);
            else
                graphPanel.removeLastNodeOfType (baseId);
        };
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
        if (baseId == "fadsr")
            return graphPanel.addNodeOfType ("fadsr", processorRef.getParameters(), makeFAdsrContent, nodeStartsHidden);
        if (baseId == "gain")
            return graphPanel.addNodeOfType ("gain", processorRef.getParameters(), makeGainContent, nodeStartsHidden);
        if (baseId == "shape")
            return graphPanel.addNodeOfType ("shape", processorRef.getParameters(), makeWaveshaperContent, nodeStartsHidden);
        if (baseId == "filter")
            return graphPanel.addNodeOfType ("filter", processorRef.getParameters(), makeFilterContent, nodeStartsHidden);
                if (baseId == "note")
            return graphPanel.addNodeOfType ("note", processorRef.getParameters(), makeNoteContent, nodeStartsHidden);
                if (baseId == "ring")
            return graphPanel.addNodeOfType ("ring", processorRef.getParameters(), makeRingModulatorContent, nodeStartsHidden);
                if (baseId == "am")
            return graphPanel.addNodeOfType ("am", processorRef.getParameters(), makeAmContent, nodeStartsHidden);
        if (baseId == "delay")
            return graphPanel.addNodeOfType ("delay", processorRef.getParameters(), makeDelayContent, nodeStartsHidden);
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

    // First launch: the patch browser shows patch #1's name without loading
    // anything (rescan does not fire onPatchSelected).  Load the selected
    // patch once so the canvas matches the name label at startup.  Placed
    // here so onConnectionPatchChanged/onNodeSetChanged are already wired.
    const juce::File initialPatch = patchBrowser.getSelectedPatch();
    if (initialPatch.existsAsFile())
        smolfm::SmolFmFile::load (graphPanel, processorRef.getParameters(), initialPatch);

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
        { "fadsr",  makeFAdsrContent   },
        { "gain",   makeGainContent    },
        { "shape",  makeWaveshaperContent },
        { "filter", makeFilterContent },
        { "ring",   makeRingModulatorContent },
                { "am",     makeAmContent        },
        { "delay",  makeDelayContent     }
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
    fAdsrButton .setRemaining (smolfm::GraphNodeRegistry::maxFAdsr            - graphPanel.countBoxesOfType ("fadsr"));
    gainButton  .setRemaining (smolfm::GraphNodeRegistry::maxGains             - graphPanel.countBoxesOfType ("gain"));
    shapeButton .setRemaining (smolfm::GraphNodeRegistry::maxWaveshapers       - graphPanel.countBoxesOfType ("shape"));
    filterButton .setRemaining (smolfm::GraphNodeRegistry::maxFilters          - graphPanel.countBoxesOfType ("filter"));
    noteButton  .setRemaining (smolfm::GraphNodeRegistry::maxNotes           - graphPanel.countBoxesOfType ("note"));
    ringButton  .setRemaining (smolfm::GraphNodeRegistry::maxRingModulators   - graphPanel.countBoxesOfType ("ring"));
    amButton    .setRemaining (smolfm::GraphNodeRegistry::maxAmModulators      - graphPanel.countBoxesOfType ("am"));
    delayButton .setRemaining (smolfm::GraphNodeRegistry::maxDelays            - graphPanel.countBoxesOfType ("delay"));
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

        // Toolbar: title left, palette center, import/export right.  Height fits the
    // +/- tiles (52px + margins).
    auto toolbar = bounds.removeFromTop (60);
    titleLabel.setBounds (toolbar.removeFromLeft (110));

    auto buttonsArea = toolbar.removeFromRight (toolbar.getWidth() > 420 ? 400 : toolbar.getWidth() / 2);
    importButton.setBounds (buttonsArea.removeFromRight (84).reduced (2));
    exportButton.setBounds (buttonsArea.removeFromRight (84).reduced (2));

        // Palette tiles with +/- buttons: 80×52 px each (set by PaletteButton::setSize).
        auto palette = toolbar.reduced (4, 2);
        const int tileWidth = 80;
        const int tileHeight = 52;
        const int tileSpacing = 8;

        // Arrange in two rows if space is tight
        const bool twoRows = palette.getWidth() < (tileWidth + tileSpacing) * 13;
        int x = 0, y = 0;

        auto placeTile = [&] (gui::PaletteButton& b)
        {
            if (twoRows && x + tileWidth > palette.getWidth())
            {
                x = 0;
                y += tileHeight + tileSpacing;
            }
            b.setBounds (palette.getX() + x, palette.getY() + y, tileWidth, tileHeight);
            x += tileWidth + tileSpacing;
        };

        placeTile (oscButton);
        placeTile (fmButton);
        placeTile (scaleButton);
        placeTile (adsrButton);
        placeTile (fAdsrButton);
        placeTile (gainButton);
        placeTile (shapeButton);
        placeTile (filterButton);
        placeTile (noteButton);
        placeTile (ringButton);
        placeTile (amButton);
        placeTile (delayButton);
        placeTile (outputButton);

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
    const int chromeTop    = 24 + 60 + 8 + 64 + 4;  // toolbar now 60px
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

