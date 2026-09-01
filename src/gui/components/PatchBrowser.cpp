/*
    PatchBrowser implementation.
*/

#include "PatchBrowser.h"

namespace gui
{

PatchBrowser::PatchBrowser()
{
    pathLabel.setJustificationType (juce::Justification::centredLeft);
    pathLabel.setMinimumHorizontalScale (0.6f);

    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::textColourId, juce::Colours::lightblue);
    nameLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    nameLabel.setMinimumHorizontalScale (0.5f);

    directoryButton.onClick = [this]
    {
        directoryChooser = std::make_unique<juce::FileChooser> (
            "Select SmolFM working directory",
            pathLabel.getText().isNotEmpty() ? juce::File (pathLabel.getText())
                                             : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory));

        directoryChooser->launchAsync (juce::FileBrowserComponent::openMode
                                     | juce::FileBrowserComponent::canSelectDirectories,
            [this] (const juce::FileChooser& chooser)
            {
                const juce::File dir = chooser.getResult();
                if (dir.isDirectory() && onDirectoryChosen != nullptr)
                    onDirectoryChosen (dir);
            });
    };

    prevButton.onClick = [this] { step (-1); };
    nextButton.onClick = [this] { step (1); };

    addAndMakeVisible (pathLabel);
    addAndMakeVisible (directoryButton);
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (nameLabel);

    updateDisplay();
}

void PatchBrowser::resized()
{
    auto bounds = getLocalBounds();

    auto directoryRow = bounds.removeFromTop (26);
    directoryButton.setBounds (directoryRow.removeFromRight (100).reduced (2));
    pathLabel.setBounds (directoryRow.reduced (2, 0));

    bounds.removeFromTop (4);

    auto navigationRow = bounds.removeFromTop (30);
    prevButton.setBounds (navigationRow.removeFromLeft (34).reduced (2));
    nextButton.setBounds (navigationRow.removeFromRight (34).reduced (2));
    nameLabel.setBounds (navigationRow);
}

void PatchBrowser::setDirectory (const juce::File& dir)
{
    pathLabel.setText (dir.isDirectory() ? dir.getFullPathName() : juce::String(),
                       juce::dontSendNotification);
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
    patchFiles.clear();
    currentIndex = -1;

    const juce::File dir (pathLabel.getText());
    if (dir.isDirectory())
    {
        for (const auto& entry : juce::RangedDirectoryIterator (dir, false, "*.smolfm"))
            patchFiles.push_back (entry.getFile());
    }

    // Keep the selection stable when the directory did not change.
    if (! patchFiles.empty())
        currentIndex = 0;

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
