/*
    PatchBrowser is the patch navigation bar above the graph panel: a single
    row with the prev/next chevrons flanking the instrument name (`< name >`)
    at the left end; the editor docks its palette toggle at the row's right
    end.  The working directory is picked in the editor's top toolbar (folder
    button + path label) and handed over via setDirectory().

    The component scans that working directory for .smolfm files and lets the
    user step through them.  Only the selected file is parsed — the list
    keeps juce::File handles plus lazily read display names.

    The directory itself lives in the processor (persisted with the plugin
    state); this component only reflects it.
*/

#pragma once

#include "../IconButton.h"

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

    /** The scanned directory (mirrored from the processor, shown in the
        editor's toolbar path label). */
    juce::File currentDirectory;

    // Icon-only chrome buttons, matching the palette tiles' look.
    IconButton prevButton      { icons::chevronLeft(),  "Previous instrument" };
    IconButton nextButton      { icons::chevronRight(), "Next instrument" };

    juce::Label nameLabel;

    std::vector<juce::File> patchFiles;
    int currentIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PatchBrowser)
};

} // namespace gui
