/*
    PluginEditor is the user interface of the synthesizer.

    It composes the high-level UI from dedicated child components.  Each
    component owns its own controls and layout; the editor only arranges them
    with a juce::Grid.

    Parameter flow:

        UI control  ←── attachment ──→  APVTS parameter  ←── voice reads ──→  audio engine
*/

#pragma once

#include "PluginProcessor.h"
#include "gui/DraggablePanel.h"
#include "gui/DraggableComponent.h"
#include "gui/components/OscillatorPanel.h"
#include "gui/components/FmAmountComponent.h"
#include "gui/components/AdsrPanel.h"
#include "gui/components/NoteNodeComponent.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AudioPluginAudioProcessor& processorRef;

    // Top-level title label.
    juce::Label titleLabel;

    // Thin toolbar with patch import/export next to the title.
    juce::TextButton exportButton { "Export" };
    juce::TextButton importButton { "Import" };

    // Draggable canvas hosting the signal-graph nodes.  This includes ADSR
    // and all processor UIs as draggable boxes.
    gui::DraggablePanel graphPanel;

    // File chooser for the toolbar buttons.  Kept alive so the async dialog
    // stays valid until the user picks a file.
    std::unique_ptr<juce::FileChooser> fileChooser;

    void exportPatch();
    void importPatch();

    // Window resize handle (bottom-right corner).  Declared before the
    // resizer below so the constrainer exists when the corner component is
    // constructed.
    std::unique_ptr<juce::ComponentBoundsConstrainer> boundsConstrainer;
    juce::ResizableCornerComponent resizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

