/*
    PatchBrowser is the patch selection bar above the graph panel.

    Layout (top to bottom):
      - directory row: path label (left) + "Directory" button (right)
      - navigation row: "<" arrow, centred instrument name label, ">" arrow

    The component scans one working directory for .smolfm files and lets the
    user step through them.  Only the selected file is parsed — the list
    keeps juce::File handles plus lazily read display names.

    The directory itself lives in the processor (persisted with the plugin
    state); this component only reflects and edits it.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{

class PatchBrowser final : public juce::Component
{
public:
    PatchBrowser();

    void resized() override;

    /** Called when the user picks a directory via the button. */
    std::function<void (const juce::File&)> onDirectoryChosen;

    /** Called when the selected patch changes (arrows or after a rescan). */
    std::function<void (const juce::File&)> onPatchSelected;

    /** Reflect the directory from the processor and rescan its contents. */
    void setDirectory (const juce::File& dir);

    /** Show an instrument name without requiring a rescan (from the file). */
    void setInstrumentName (const juce::String& name);

    /** Select the given file if it is part of the current listing. */
    void selectPatch (const juce::File& file);

    juce::File getSelectedPatch() const;

private:
    void rescan();
    void step (int delta);
    void updateDisplay();

    juce::Label pathLabel;
    juce::TextButton directoryButton { "Directory" };
    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::Label nameLabel;

    std::unique_ptr<juce::FileChooser> directoryChooser;

    std::vector<juce::File> patchFiles;
    int currentIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PatchBrowser)
};

} // namespace gui
