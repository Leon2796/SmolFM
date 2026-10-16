/*
    PatchBrowser implementation.
*/

#include "PatchBrowser.h"

namespace gui
{

PatchBrowser::PatchBrowser()
{
    nameLabel.setJustificationType (juce::Justification::centred);   // name sits centred between the chevrons
    nameLabel.setColour (juce::Label::textColourId, juce::Colours::lightblue);
    nameLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    nameLabel.setMinimumHorizontalScale (0.5f);

    prevButton.onClick = [this] { step (-1); };
    nextButton.onClick = [this] { step (1); };

    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (nameLabel);

    updateDisplay();
}

void PatchBrowser::resized()
{
    // Flanked title at the left end: `< name >` — the instrument title sits
    // between the two chevrons.  Its slot has a fixed width (so the chevrons
    // never wander from click to click) and the text is centred inside that
    // slot.  The row's right end stays free for the editor's palette toggle.
    auto row = getLocalBounds();

    const int button = 28;
    const int gap    = 6;

    const int nameWidth = juce::jmin (220, juce::jmax (0, row.getWidth() - 2 * (button + gap)));

    auto cluster = row.removeFromLeft (button + gap + nameWidth + gap + button);
    prevButton.setBounds (cluster.removeFromLeft (button).withSizeKeepingCentre (button, button));
    cluster.removeFromLeft (gap);
    nameLabel.setBounds (cluster.removeFromLeft (nameWidth));
    cluster.removeFromLeft (gap);
    nextButton.setBounds (cluster.removeFromLeft (button).withSizeKeepingCentre (button, button));
}

void PatchBrowser::setDirectory (const juce::File& dir)
{
    currentDirectory = dir;
    rescan();
}

void PatchBrowser::selectPatch (const juce::File& file)
{
    for (int i = 0; i < static_cast<int> (patchFiles.size()); ++i)
    {
        if (patchFiles[static_cast<size_t> (i)] == file)
        {
            currentIndex = i;
            updateDisplay();
            return;
        }
    }
}

juce::File PatchBrowser::getSelectedPatch() const
{
    if (currentIndex >= 0 && currentIndex < static_cast<int> (patchFiles.size()))
        return patchFiles[static_cast<size_t> (currentIndex)];

    return {};
}

void PatchBrowser::setInstrumentName (const juce::String& name)
{
    nameLabel.setText (name, juce::dontSendNotification);
}

void PatchBrowser::rescan()
{
    // Remember the selection across the rescan: rescans also run with an
    // unchanged directory (e.g. the same directory chosen again), and the
    // user must not silently jump to the first patch in that case.
    const juce::File previous = getSelectedPatch();

    patchFiles.clear();
    currentIndex = -1;

    const juce::File dir (currentDirectory);
    if (dir.isDirectory())
    {
        for (const auto& entry : juce::RangedDirectoryIterator (dir, false, "*.smolfm"))
            patchFiles.push_back (entry.getFile());
    }

    // First patch wins when the previous selection is not in the new
    // listing (a genuinely new directory); otherwise it stays put.
    if (! patchFiles.empty())
        currentIndex = 0;

    for (int i = 0; i < static_cast<int> (patchFiles.size()); ++i)
    {
        if (patchFiles[static_cast<std::size_t> (i)] == previous)
        {
            currentIndex = i;
            break;
        }
    }

    updateDisplay();
}

void PatchBrowser::step (int delta)
{
    if (patchFiles.empty())
        return;

    const int count = static_cast<int> (patchFiles.size());
    currentIndex = ((currentIndex + delta) % count + count) % count;
    updateDisplay();

    if (onPatchSelected != nullptr)
        onPatchSelected (getSelectedPatch());
}

void PatchBrowser::updateDisplay()
{
    const bool hasSelection = currentIndex >= 0 && currentIndex < static_cast<int> (patchFiles.size());

    nameLabel.setText (hasSelection ? patchFiles[static_cast<size_t> (currentIndex)].getFileNameWithoutExtension()
                                    : juce::String(),
                       juce::dontSendNotification);

    prevButton.setEnabled (patchFiles.size() > 1);
    nextButton.setEnabled (patchFiles.size() > 1);
}

} // namespace gui
