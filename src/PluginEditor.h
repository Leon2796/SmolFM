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
    juce::TextButton exportButton { "Export" };
    juce::TextButton importButton { "Import" };

    // Add-node palette: one tile per node type, with budget badge.
    gui::PaletteButton oscButton;
    gui::PaletteButton fmButton;
    gui::PaletteButton scaleButton;
    gui::PaletteButton adsrButton;
    gui::PaletteButton noteButton;
    gui::PaletteButton outputButton;

    gui::DraggablePanel graphPanel;
    gui::PatchBrowser patchBrowser;
    std::unique_ptr<juce::FileChooser> fileChooser;

    void exportPatch();
    void importPatch();
    void addNodeFromToolbar (const juce::String& baseId);
    void refreshToolbarBadges();

    /** Grow the window after loading a patch so the layout fits on screen. */
    void fitWindowToContent();

    // The output node's content needs the processor for the meter provider.
    std::unique_ptr<juce::Component> makeOutputContent (const juce::String& instanceId,
                                                        juce::AudioProcessorValueTreeState& apvts);

    std::unique_ptr<juce::ComponentBoundsConstrainer> boundsConstrainer;
    juce::ResizableCornerComponent resizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

