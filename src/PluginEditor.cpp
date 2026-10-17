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
#include "gui/components/VelocityScaleComponent.h"
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
#include "gui/components/SampleHoldComponent.h"
#include "gui/components/SplitterComponent.h"
#include "gui/components/MergerComponent.h"
#include "gui/components/AdderComponent.h"
#include "gui/components/ResonatorComponent.h"
#include "gui/components/FeedbackComponent.h"

namespace
{
    namespace icons
    {
        // Stroke-only glyph builders for the palette tiles.  Every path is
        // sketched in a virtual 24x24 box; PaletteButton scales and strokes
        // it, so all icons share exactly one line weight.  Tiles without an
        // icon show their letters instead.

        juce::Path sine()      // oscillator
        {
            juce::Path p;
            p.startNewSubPath (2, 12);
            p.cubicTo (5, 2, 9, 2, 12, 12);
            p.cubicTo (15, 22, 19, 22, 22, 12);
            return p;
        }

        juce::Path lowpass()   // filter: flat, knee, tail
        {
            juce::Path p;
            p.startNewSubPath (2, 6);
            p.lineTo (10, 6);
            p.quadraticTo (17, 6, 19, 16);
            p.lineTo (22, 16);
            return p;
        }

        juce::Path speaker()   // gain: wedge + emission arcs
        {
            juce::Path p;
            p.startNewSubPath (3, 9);
            p.lineTo (8, 9);
            p.lineTo (13, 4);
            p.lineTo (13, 20);
            p.lineTo (8, 15);
            p.lineTo (3, 15);
            p.closeSubPath();
            p.addArc (14, 6, 6, 12, -0.9f, 0.9f, true);
            p.addArc (16, 3, 8, 18, -0.75f, 0.75f, true);
            return p;
        }

        juce::Path envelope()  // ADSR: attack, decay, sustain, release
        {
            juce::Path p;
            p.startNewSubPath (2, 20);
            p.lineTo (6, 4);
            p.lineTo (10, 13);
            p.lineTo (16, 13);
            p.lineTo (22, 20);
            return p;
        }

        juce::Path curve()     // waveshaper: transfer curve
        {
            juce::Path p;
            p.startNewSubPath (4, 20);
            p.cubicTo (12, 20, 12, 4, 20, 4);
            return p;
        }

        juce::Path noteGlyph() // note: head, stem, flag
        {
            juce::Path p;
            p.addEllipse (5, 15, 7, 5);
            p.startNewSubPath (12, 17.5f);
            p.lineTo (12, 4);
            p.quadraticTo (17, 3, 18, 8);
            return p;
        }

        juce::Path taps()      // delay: receding echo taps
        {
            juce::Path p;
            p.addEllipse (3, 9.5f, 5, 5);
            p.addEllipse (11, 10, 4, 4);
            p.addEllipse (18, 10.5f, 3, 3);
            return p;
        }

        juce::Path splitY()    // splitter: one in, two out
        {
            juce::Path p;
            p.startNewSubPath (2, 12);
            p.lineTo (10, 12);
            p.startNewSubPath (10, 12);
            p.lineTo (21, 5);
            p.startNewSubPath (10, 12);
            p.lineTo (21, 19);
            return p;
        }

        juce::Path mergeY()    // merger: two in, one out
        {
            juce::Path p;
            p.startNewSubPath (2, 5);
            p.lineTo (11, 12);
            p.lineTo (2, 19);
            p.startNewSubPath (11, 12);
            p.lineTo (22, 12);
            return p;
        }
    }

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
            apvts,
            smolfm::GraphNodeRegistry::amountParameterIdFor (instanceId),
            smolfm::GraphNodeRegistry::freqScaleIntervalParameterIdFor (instanceId));
    }

    std::unique_ptr<juce::Component> makeVelocityScaleContent (const juce::String& instanceId,
                                                               juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::VelocityScaleComponent> (
            apvts,
            smolfm::GraphNodeRegistry::velocityScaleCurveParameterIdFor (instanceId),
            smolfm::GraphNodeRegistry::amountParameterIdFor (instanceId));
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

    std::unique_ptr<juce::Component> makeSplitterContent (const juce::String&,
                                                          juce::AudioProcessorValueTreeState&)
    {
        return std::make_unique<gui::SplitterComponent>();
    }

    std::unique_ptr<juce::Component> makeMergerContent (const juce::String&,
                                                        juce::AudioProcessorValueTreeState&)
    {
        return std::make_unique<gui::MergerComponent>();
    }

    std::unique_ptr<juce::Component> makeAddContent (const juce::String&,
                                                     juce::AudioProcessorValueTreeState&)
    {
        return std::make_unique<gui::AdderComponent>();
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

    std::unique_ptr<juce::Component> makeHoldContent (const juce::String& instanceId,
                                                      juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::SampleHoldComponent> (
            apvts,
            smolfm::GraphNodeRegistry::holdRateParameterIdFor     (instanceId),
            smolfm::GraphNodeRegistry::holdSyncParameterIdFor     (instanceId),
            smolfm::GraphNodeRegistry::holdDivisionParameterIdFor (instanceId));
    }

    std::unique_ptr<juce::Component> makeResonatorContent (const juce::String& instanceId,
                                                           juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::ResonatorComponent> (
            apvts,
            smolfm::GraphNodeRegistry::resonatorTuneParameterIdFor         (instanceId),
            smolfm::GraphNodeRegistry::resonatorQParameterIdFor            (instanceId),
            smolfm::GraphNodeRegistry::resonatorDecayParameterIdFor        (instanceId),
            smolfm::GraphNodeRegistry::resonatorVelSpreadParameterIdFor    (instanceId),
            smolfm::GraphNodeRegistry::resonatorStrikeLevelParameterIdFor  (instanceId),
            smolfm::GraphNodeRegistry::resonatorStrikeTimeParameterIdFor   (instanceId));
    }

    std::unique_ptr<juce::Component> makeFeedbackContent (const juce::String& instanceId,
                                                          juce::AudioProcessorValueTreeState& apvts)
    {
        return std::make_unique<gui::FeedbackComponent> (
            apvts, smolfm::GraphNodeRegistry::amountParameterIdFor (instanceId));
    }

    //==========================================================================
    // Palette column geometry, shared by resized() and fitWindowToContent().
    constexpr int paletteTile      = 56;
    constexpr int paletteGap       = 2;
    constexpr int paletteColumns   = 3;
    constexpr int paletteTileCount = 20;   // keep in sync with the addAndMakeVisible loop
    constexpr int paletteRows      = (paletteTileCount + paletteColumns - 1) / paletteColumns;
    constexpr int paletteGridW     = paletteColumns * paletteTile + (paletteColumns - 1) * paletteGap;
    constexpr int paletteGridH     = paletteRows    * paletteTile + (paletteRows    - 1) * paletteGap;
    constexpr int palettePad       = 4;    // padding inside the palette column
    constexpr int paletteW         = paletteGridW + 2 * palettePad + 8;  // + reserve for the vertical scrollbar
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
    addAndMakeVisible (directoryButton);
    addAndMakeVisible (gridToggleButton);
    addAndMakeVisible (paletteToggleButton);
    addAndMakeVisible (pathLabel);

    exportButton.onClick = [this] { exportPatch(); };
    importButton.onClick = [this] { importPatch(); };

    // The grid switch is a real toggle: IconButton derives from juce::Button,
    // which only flips its state when clicking-toggles-state is enabled.
    // Without this line getToggleState() stayed false forever.
    gridToggleButton.setClickingTogglesState (true);

    // Grid mode: docking may park boxes further right/below than the
    // free-form layout ever reached, so re-fit the window after toggling.
    gridToggleButton.onClick = [this]
    {
        graphPanel.setGridMode (gridToggleButton.getToggleState());
        gridToggleButton.setTooltip (gridToggleButton.getToggleState()
                                         ? "Grid mode on: boxes dock into the unit cells"
                                         : "Grid mode");
        fitWindowToContent();
    };

    // Toolbar path label: shows the directory the patch browser scans.
    pathLabel.setJustificationType (juce::Justification::centredLeft);
    pathLabel.setMinimumHorizontalScale (0.6f);
    pathLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    pathLabel.setFont (juce::FontOptions (12.0f));

    // The palette toggle shows/hides the collapsible right-side node palette.
    paletteToggleButton.onClick = [this]
    {
        paletteVisible = ! paletteVisible;
        paletteToggleButton.setTooltip (paletteVisible ? "Hide node palette"
                                                       : "Show node palette");
        resized();
    };

    // Working-directory picker: adopts the chosen directory through the very
    // same path as the patch browser's directory handling (see the
    // onDirectoryChosen lambda below).
    directoryButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> ("Select SmolFM working directory",
            processorRef.getPatchDirectory());

        fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                | juce::FileBrowserComponent::canSelectDirectories,
            [this] (const juce::FileChooser& chooser)
            {
                if (const juce::File dir = chooser.getResult(); dir.isDirectory())
                    patchBrowser.onDirectoryChosen (dir);
            });
    };

    // Patch browser: directory lives in the processor state; selection loads
    // the chosen patch into the graph panel.
    //
    // Context-switch rules so label, listing and sounding instrument always
    // agree:
    //  - a directory choice loads its newly selected first patch (a stable
    //    rescan with an unchanged directory keeps the selection and nothing
    //    is reloaded);
    //  - a failed load alerts and reverts the listing to what still plays.
    patchBrowser.onDirectoryChosen = [this] (const juce::File& dir)
    {
        setWorkingDirectory (dir);

        if (const juce::File selected = patchBrowser.getSelectedPatch();
            selected.existsAsFile() && selected != lastLoadedPatch)
        {
            if (! loadPatch (selected))
                patchBrowser.selectPatch (lastLoadedPatch);
        }
    };
    patchBrowser.onPatchSelected = [this] (const juce::File& file)
    {
        if (! loadPatch (file))
            patchBrowser.selectPatch (lastLoadedPatch);
    };
    setWorkingDirectory (processorRef.getPatchDirectory());

    boundsConstrainer->setMinimumSize (700, 500);
    boundsConstrainer->setMaximumSize (2400, 1600);
    addAndMakeVisible (resizer);
    setResizable (true, true);

    // -----------------------------------------------------------------
    // Side palette: one square tile per node type, showing how many can
    // still be added.  Icons where a glyph beats the letters.
    // -----------------------------------------------------------------
    oscButton     .configure ("osc",      "Osc", smolfm::GraphNodeRegistry::maxOscillators,     icons::sine);
    fmButton      .configure ("fm",       "FM",  smolfm::GraphNodeRegistry::maxFmAmounts);
    scaleButton   .configure ("fscale",   "FSC", smolfm::GraphNodeRegistry::maxFrequencyScales);
    vScaleButton  .configure ("vsc",      "VSC", smolfm::GraphNodeRegistry::maxVelocityScales);
    adsrButton    .configure ("adsr",     "ADSR", smolfm::GraphNodeRegistry::maxAdsr,  icons::envelope);
    fAdsrButton   .configure ("fadsr",    "FAD", smolfm::GraphNodeRegistry::maxFAdsr);
    gainButton    .configure ("gain",     "GN",  smolfm::GraphNodeRegistry::maxGains,  icons::speaker);
    shapeButton   .configure ("shape",    "SHP", smolfm::GraphNodeRegistry::maxWaveshapers, icons::curve);
    filterButton  .configure ("filter",   "FLT", smolfm::GraphNodeRegistry::maxFilters, icons::lowpass);
    resonatorButton.configure ("resonator", "RES", smolfm::GraphNodeRegistry::maxResonators);
    feedbackButton.configure  ("feedback", "FDB", smolfm::GraphNodeRegistry::maxFeedbacks);
    noteButton    .configure ("note",      "NT",  smolfm::GraphNodeRegistry::maxNotes,  icons::noteGlyph);
    ringButton    .configure ("ring",      "RNG", smolfm::GraphNodeRegistry::maxRingModulators);
    amButton      .configure ("am",        "AM",  smolfm::GraphNodeRegistry::maxAmModulators);
    delayButton   .configure ("delay",     "DLY", smolfm::GraphNodeRegistry::maxDelays, icons::taps);
    holdButton    .configure ("hold",      "SH",  smolfm::GraphNodeRegistry::maxSampleHolds);
    outputButton  .configure ("output",    "OUT", smolfm::GraphNodeRegistry::maxMasterOutputs);
    splitButton   .configure ("split",     "SPL", smolfm::GraphNodeRegistry::maxSplitters, icons::splitY);
    mergeButton   .configure ("merge",     "MRG", smolfm::GraphNodeRegistry::maxMergers, icons::mergeY);
    addButton     .configure ("add",       "ADD", smolfm::GraphNodeRegistry::maxAdds);

    // Right palette column: the scroll view keeps every row reachable when
    // the window is short.
    paletteScroll.setScrollBarsShown (true, false);   // vertical only
    paletteScroll.setScrollBarThickness (8);
    paletteScroll.setViewedComponent (&paletteGrid, false);
    addAndMakeVisible (paletteScroll);

    for (auto* b : { &oscButton, &fmButton, &scaleButton, &vScaleButton, &adsrButton, &fAdsrButton, &gainButton, &shapeButton, &filterButton, &resonatorButton, &feedbackButton, &noteButton, &ringButton, &amButton, &delayButton, &holdButton, &outputButton, &splitButton, &mergeButton, &addButton })
    {
        paletteGrid.addAndMakeVisible (*b);
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
        if (baseId == "vsc")
            return graphPanel.addNodeOfType ("vsc", processorRef.getParameters(), makeVelocityScaleContent, nodeStartsHidden);
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
        if (baseId == "resonator")
            return graphPanel.addNodeOfType ("resonator", processorRef.getParameters(), makeResonatorContent, nodeStartsHidden);
        if (baseId == "feedback")
            return graphPanel.addNodeOfType ("feedback", processorRef.getParameters(), makeFeedbackContent, nodeStartsHidden);
                if (baseId == "note")
            return graphPanel.addNodeOfType ("note", processorRef.getParameters(), makeNoteContent, nodeStartsHidden);
                if (baseId == "ring")
            return graphPanel.addNodeOfType ("ring", processorRef.getParameters(), makeRingModulatorContent, nodeStartsHidden);
        if (baseId == "split")
            return graphPanel.addNodeOfType ("split", processorRef.getParameters(), makeSplitterContent, nodeStartsHidden);
        if (baseId == "merge")
            return graphPanel.addNodeOfType ("merge", processorRef.getParameters(), makeMergerContent, nodeStartsHidden);
        if (baseId == "add")
            return graphPanel.addNodeOfType ("add", processorRef.getParameters(), makeAddContent, nodeStartsHidden);
                if (baseId == "am")
            return graphPanel.addNodeOfType ("am", processorRef.getParameters(), makeAmContent, nodeStartsHidden);
        if (baseId == "delay")
            return graphPanel.addNodeOfType ("delay", processorRef.getParameters(), makeDelayContent, nodeStartsHidden);
        if (baseId == "hold")
            return graphPanel.addNodeOfType ("hold", processorRef.getParameters(), makeHoldContent, nodeStartsHidden);
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
        loadPatch (initialPatch);

    setSize (1200, 800);

    // Optional: --instrument <file.smolfm> from the standalone app's command
    // line overrides the browser's default first patch (see method comment).
    applyStartupInstrumentParameter();
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
        { "vsc",    makeVelocityScaleContent },
        { "adsr",   makeAdsrContent    },
        { "fadsr",  makeFAdsrContent   },
        { "gain",   makeGainContent    },
        { "shape",  makeWaveshaperContent },
        { "filter", makeFilterContent },
        { "resonator", makeResonatorContent },
        { "feedback", makeFeedbackContent },
        { "ring",   makeRingModulatorContent },
                { "am",     makeAmContent        },
        { "delay",  makeDelayContent     },
        { "hold",   makeHoldContent      },
        { "split",  makeSplitterContent  },
        { "merge",  makeMergerContent    },
        { "add",    makeAddContent       }
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
    vScaleButton.setRemaining (smolfm::GraphNodeRegistry::maxVelocityScales - graphPanel.countBoxesOfType ("vsc"));
    adsrButton  .setRemaining (smolfm::GraphNodeRegistry::maxAdsr            - graphPanel.countBoxesOfType ("adsr"));
    fAdsrButton .setRemaining (smolfm::GraphNodeRegistry::maxFAdsr            - graphPanel.countBoxesOfType ("fadsr"));
    gainButton  .setRemaining (smolfm::GraphNodeRegistry::maxGains             - graphPanel.countBoxesOfType ("gain"));
    shapeButton .setRemaining (smolfm::GraphNodeRegistry::maxWaveshapers       - graphPanel.countBoxesOfType ("shape"));
    filterButton .setRemaining (smolfm::GraphNodeRegistry::maxFilters          - graphPanel.countBoxesOfType ("filter"));
    resonatorButton .setRemaining (smolfm::GraphNodeRegistry::maxResonators    - graphPanel.countBoxesOfType ("resonator"));
    feedbackButton .setRemaining (smolfm::GraphNodeRegistry::maxFeedbacks      - graphPanel.countBoxesOfType ("feedback"));
    noteButton  .setRemaining (smolfm::GraphNodeRegistry::maxNotes           - graphPanel.countBoxesOfType ("note"));
    ringButton  .setRemaining (smolfm::GraphNodeRegistry::maxRingModulators   - graphPanel.countBoxesOfType ("ring"));
    amButton    .setRemaining (smolfm::GraphNodeRegistry::maxAmModulators      - graphPanel.countBoxesOfType ("am"));
    delayButton .setRemaining (smolfm::GraphNodeRegistry::maxDelays            - graphPanel.countBoxesOfType ("delay"));
    holdButton  .setRemaining (smolfm::GraphNodeRegistry::maxSampleHolds       - graphPanel.countBoxesOfType ("hold"));
    outputButton.setRemaining (smolfm::GraphNodeRegistry::maxMasterOutputs   - graphPanel.countBoxesOfType ("output"));
    splitButton .setRemaining (smolfm::GraphNodeRegistry::maxSplitters         - graphPanel.countBoxesOfType ("split"));
    mergeButton .setRemaining (smolfm::GraphNodeRegistry::maxMergers           - graphPanel.countBoxesOfType ("merge"));
    addButton   .setRemaining (smolfm::GraphNodeRegistry::maxAdds              - graphPanel.countBoxesOfType ("add"));
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
    auto bounds = getLocalBounds();

    // The chrome hugs the window edges: compact toolbar row on top, patch
    // navigation below, the canvas fills the rest.
    bounds = bounds.withTrimmedLeft (8).withTrimmedTop (6)
                   .withTrimmedRight (4).withTrimmedBottom (8);   // left, top, seam right of the canvas, bottom

    // Toolbar: title, icon-only export/import/directory (tooltips name them),
    // then the working-directory path.  The palette toggle lives at the right
    // end of the patch navigation row instead of the toolbar.
    auto toolbar = bounds.removeFromTop (28);
    titleLabel.setBounds (toolbar.removeFromLeft (70));
    toolbar.removeFromLeft (6);
    exportButton.setBounds (toolbar.removeFromLeft (28).reduced (1));
    toolbar.removeFromLeft (4);
    importButton.setBounds (toolbar.removeFromLeft (28).reduced (1));
    toolbar.removeFromLeft (4);
    directoryButton.setBounds (toolbar.removeFromLeft (28).reduced (1));
    toolbar.removeFromLeft (4);
    gridToggleButton.setBounds (toolbar.removeFromLeft (28).reduced (1));
    pathLabel.setBounds (toolbar);

    auto placeTile = [&] (gui::PaletteButton& b, int index)
    {
        const int col = index % paletteColumns, row = index / paletteColumns;
        b.setBounds (palettePad + col * (paletteTile + paletteGap),
                     palettePad + row * (paletteTile + paletteGap), paletteTile, paletteTile);
    };

    int index = 0;
    placeTile (oscButton, index++);
    placeTile (fmButton, index++);
    placeTile (scaleButton, index++);
    placeTile (vScaleButton, index++);
    placeTile (adsrButton, index++);
    placeTile (fAdsrButton, index++);
    placeTile (gainButton, index++);
    placeTile (shapeButton, index++);
    placeTile (filterButton, index++);
    placeTile (resonatorButton, index++);
    placeTile (feedbackButton, index++);
    placeTile (noteButton, index++);
    placeTile (ringButton, index++);
    placeTile (amButton, index++);
    placeTile (delayButton, index++);
    placeTile (holdButton, index++);
    placeTile (outputButton, index++);
    placeTile (splitButton, index++);
    placeTile (mergeButton, index++);
    placeTile (addButton, index++);
    jassert (index == paletteTileCount);   // tile list and grid geometry out of sync?

    bounds.removeFromTop (6);

    // Patch navigation above the canvas: `< name >` flanking cluster at the
    // left end (directory button and path label moved into the toolbar
    // above).  The row's right end is reserved for the palette toggle.
    auto navRow = bounds.removeFromTop (30);
    auto toggleSlot = navRow.removeFromRight (28);
    patchBrowser.setBounds (navRow);
    bounds.removeFromTop (4);

    // Collapsible right side palette: a fixed 3-column grid of square tiles,
    // one per node type, barely separated (2px) so the grid reads as one
    // block.  Its column sits beside the canvas only, sharing the draggable
    // panel's top and bottom edges, and can be hidden via the toggle inside
    // the panel.  The scroll view keeps the last rows reachable on short
    // windows.
    const int paletteAreaW = paletteVisible ? paletteW : 0;
    auto paletteArea = bounds.removeFromRight (paletteAreaW);
    paletteScroll.setVisible (paletteVisible);
    paletteScroll.setBounds (paletteArea.withTrimmedLeft (2));   // right edge stays flush at the window seam
    paletteGrid.setBounds (0, 0, paletteArea.getWidth() - 8, paletteGridH + 2 * palettePad);

    // The palette toggle sits at the right end of the navigation row: flush
    // with the window seam, directly above the palette column it switches,
    // and entirely clear of canvas content also while the palette is hidden.
    paletteToggleButton.setBounds (toggleSlot.reduced (1));

    // The viewport gets the remaining space; the panel owns its scrollable
    // canvas sizing (visible-area minimum plus grid reserve cell, see
    // DraggablePanel::refreshCanvasSize — scrollbar on/off follows from
    // canvas vs. viewport automatically).
    graphViewport.setBounds (bounds);
    graphPanel.refreshCanvasSize();

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

    // Fixed chrome: slim margins (8 left, 6 top, 8 bottom) + slim toolbar
    // (28) + gaps (6+4) + patch navigation (30).  Right: seam (4) + palette
    // column; the collapsible palette contributes nothing while hidden.
    const int chromeTop    = 6 + 28 + 6 + 30 + 4;
    const int chromeLeft   = 8;
    const int chromeRight  = 4 + (paletteVisible ? paletteW : 0);
    const int chromeBottom = 8;

    // content.getRight() is the panel-local right edge of the layout; the
    // window needs that plus the chrome around the canvas.
    const int neededW = content.getRight()  + chromeLeft + chromeRight;
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
/*
    Single funnel for every patch-switch path (startup, browser step, directory
    choice, import): the editor's guarantee is that the name label, the browser
    selection and the sounding graph always refer to the same file.
*/
bool AudioPluginAudioProcessorEditor::loadPatch (const juce::File& file)
{
    jassert (file != juce::File());   // callers screen existence already

    if (! smolfm::SmolFmFile::load (graphPanel, processorRef.getParameters(), file))
    {
        // The previous patch keeps playing; callers may revert their listing.
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                "SmolFM",
                                                "Could not load instrument:\n"
                                                + file.getFullPathName());
        return false;
    }

    lastLoadedPatch = file;
    patchBrowser.setInstrumentName (smolfm::SmolFmFile::readInstrumentName (file));

    // Grid mode: loaded boxes dock into the unit grid — sizes snap up to
    // whole cells, positions move to the nearest collision-free cell —
    // before the window is fitted to the docked content.
    if (graphPanel.isGridMode())
        graphPanel.dockToGrid();

    fitWindowToContent();
    return true;
}

//==============================================================================
/*
    Single funnel for the working directory, matching loadPatch() for patches:
    the processor state (persisted), the patch browser's listing and the
    toolbar path label always agree on the scanned directory.
*/
void AudioPluginAudioProcessorEditor::setWorkingDirectory (const juce::File& dir)
{
    processorRef.setPatchDirectory (dir);
    patchBrowser.setDirectory (dir);
    pathLabel.setText (dir.getFullPathName(), juce::dontSendNotification);
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

            // Load first: only a successful import adopts the file's directory
            // as the browser context.  An early directory switch would leave
            // the listing (and a subsequent arrowse step) pointing at a
            // different instrument than the one actually loaded.
            if (! loadPatch (source))
                return;

            const juce::File directory = source.getParentDirectory();
            setWorkingDirectory (directory);
            patchBrowser.selectPatch (source);
        });
}

//==============================================================================
/*
    Optional start parameter of the standalone binary:

        SmolFM.exe --instrument <path\file.smolfm>
        SmolFM.exe --instrument=<path\file.smolfm>
        SmolFM.exe -i <path\file.smolfm>
        SmolFM.exe <path\file.smolfm>

    Relative paths resolve against the current working directory.  The file is
    loaded exactly like a patch browser selection: the browser switches to the
    file's directory, the instrument is parsed and its name is displayed.
*/
void AudioPluginAudioProcessorEditor::applyStartupInstrumentParameter()
{
    // A JUCEApplication instance and a standalone wrapper type both exist
    // only in our own binary.  Hosted plugin copies must not react to the
    // (host process') command line, so bail out there.
    if (juce::JUCEApplicationBase::getInstance() == nullptr
        || processorRef.wrapperType != juce::AudioProcessor::wrapperType_Standalone)
        return;

    const juce::StringArray tokens (juce::JUCEApplicationBase::getCommandLineParameterArray());

    // Accepted flag spellings; the first match wins.
    juce::String optionPath;

    for (int i = 0; i < tokens.size(); ++i)
    {
        const juce::String& token = tokens.getReference (i);

        if ((token == "--instrument" || token == "-i") && i + 1 < tokens.size())
        {
            optionPath = tokens [i + 1];
            break;
        }

        if (token.startsWith ("--instrument="))
        {
            optionPath = token.fromFirstOccurrenceOf ("=", false, false);
            break;
        }
    }

    // Fallback: one bare positional token naming an instrument file.
    if (optionPath.isEmpty())
    {
        for (const auto& token : tokens)
        {
            if (! token.startsWith ("-") && token.endsWithIgnoreCase (".smolfm"))
            {
                optionPath = token;
                break;
            }
        }
    }

    if (optionPath.isEmpty())
        return;

    // getChildFile() returns the argument itself when it is already an
    // absolute path; otherwise it resolves against the working directory.
    const juce::File instrumentFile (juce::File::getCurrentWorkingDirectory()
                                        .getChildFile (optionPath));

    if (! instrumentFile.existsAsFile() || ! instrumentFile.hasFileExtension (".smolfm"))
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                "SmolFM",
                                                "Instrument file not found:\n"
                                                + instrumentFile.getFullPathName());
        return;
    }

    // Behave like choosing the file through the patch browser: switch the
    // working directory, mark the file in the listing, then load and show it.
    const juce::File directory (instrumentFile.getParentDirectory());

    setWorkingDirectory (directory);
    patchBrowser.selectPatch (instrumentFile);

    loadPatch (instrumentFile);
}

