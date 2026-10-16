/*
    PluginEditor is the user interface of the synthesizer.

    It composes the high-level UI from dedicated child components.  Each
    component owns its own controls and layout; the editor only arranges them
    with a juce::Grid.

    Parameter flow:

        UI control  ←── attachment ──→  APVTS parameter  ←── voice reads ──→  audio engine
*/

#pragma once

#pragma once

#include "PluginProcessor.h"
#include "gui/DraggablePanel.h"
#include "gui/DraggableComponent.h"
#include "gui/PaletteButton.h"
#include "gui/IconButton.h"
#include "gui/components/PatchBrowser.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AudioPluginAudioProcessor& processorRef;

    juce::Label titleLabel;
    juce::Label pathLabel;                           ///< shows the patch browser's working directory, right of the toolbar buttons
    juce::TooltipWindow tooltipWindow { this, 400 }; ///< shows the tooltips of every control below

    // Icon-only chrome controls (see gui/IconButton.h), laid out in the top
    // toolbar in this order: export, import, working directory.
    gui::IconButton exportButton    { gui::icons::exportGlyph(), "Export current instrument as .smolfm" };
    gui::IconButton importButton    { gui::icons::importGlyph(), "Import a .smolfm instrument" };
    gui::IconButton directoryButton { gui::icons::folderGlyph(), "Choose working directory" };

    /** Toggle for the collapsible right-side node palette (docked at the
        right end of the patch navigation row, above the palette column). */
    gui::IconButton paletteToggleButton { gui::icons::paletteGlyph(), "Hide node palette" };
    bool paletteVisible = true;

    // Add-node palette: one tile per node type, with budget badge.
    gui::PaletteButton oscButton;
    gui::PaletteButton fmButton;
    gui::PaletteButton scaleButton;
    gui::PaletteButton vScaleButton;
    gui::PaletteButton adsrButton;
    gui::PaletteButton fAdsrButton;
    gui::PaletteButton gainButton;
    gui::PaletteButton shapeButton;
    gui::PaletteButton filterButton;
    gui::PaletteButton noteButton;
    gui::PaletteButton ringButton;
    gui::PaletteButton amButton;
    gui::PaletteButton delayButton;
    gui::PaletteButton holdButton;
    gui::PaletteButton outputButton;
    gui::PaletteButton splitButton;
    gui::PaletteButton mergeButton;
    gui::PaletteButton addButton;
    gui::PaletteButton resonatorButton;
    gui::PaletteButton feedbackButton;

    juce::Viewport graphViewport;  ///< Scrollable container for the graph canvas
    gui::DraggablePanel graphPanel;
    gui::PatchBrowser patchBrowser;
    // Right side palette: a scrollable, collapsible 3-column grid of tiles.
    juce::Component paletteGrid;
    juce::Viewport paletteScroll;
    std::unique_ptr<juce::FileChooser> fileChooser;

    /** The instrument file whose data the canvas and the audio engine currently
        reflect — the anchor for keeping name label, listing and sound in sync
        after failed loads or context switches. */
    juce::File lastLoadedPatch;

    void exportPatch();
    void importPatch();

    /**
        Load one .smolfm file as the current instrument: rebuilds parameters,
        box positions and wiring.  On success remembers it in lastLoadedPatch,
        shows its instrument name and grows the window.  On failure alerts and
        returns false; the previous patch keeps playing.
    */
    bool loadPatch (const juce::File& file);

    /**
        Single funnel for the working directory, matching loadPatch() for
        patches: persists the directory in the processor, rescans the patch
        browser and mirrors it into the toolbar path label.
    */
    void setWorkingDirectory (const juce::File& dir);

    void addNodeFromToolbar (const juce::String& baseId);
    void refreshToolbarBadges();

    /**
        Optional standalone start parameter:  SmolFM.exe --instrument <file.smolfm>
        (also accepted: "-i <file>" or a bare positional "<file>.smolfm").

        Only runs in the standalone binary — hosted plugin copies ignore the
        (host process') command line.  Loads the file exactly like a patch
        browser selection: switches the browser to the file's directory, loads
        the instrument and marks it in the listing.
    */
    void applyStartupInstrumentParameter();

    /** Grow the window after loading a patch so the layout fits on screen. */
    void fitWindowToContent();

    // The output node's content needs the processor for the meter provider.
    std::unique_ptr<juce::Component> makeOutputContent (const juce::String& instanceId,
                                                        juce::AudioProcessorValueTreeState& apvts);

    std::unique_ptr<juce::ComponentBoundsConstrainer> boundsConstrainer;
    juce::ResizableCornerComponent resizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

