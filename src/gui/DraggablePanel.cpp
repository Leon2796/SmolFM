/*
    DraggablePanel implementation.
*/

#include "DraggablePanel.h"

namespace gui
{

namespace
{
    juce::String makeDefaultPositionKey (const juce::String& boxId)
    {
        return "pos_" + boxId;   // e.g. pos_carrier, pos_modulator, ...
    }

    void writeBoxPosition (juce::PropertiesFile& props,
                           const DraggableComponent& box)
    {
        const juce::Rectangle<int> r = box.getBounds();
        // Compact storage: one string per box, no XML noise.
        props.setValue (makeDefaultPositionKey (box.getBoxId()),
                        juce::String (r.getX()) + "," + juce::String (r.getY()));
    }

    bool readBoxPosition (juce::PropertiesFile& props,
                          const juce::String& boxId,
                          juce::Point<int>& outPosition)
    {
        const juce::String s = props.getValue (makeDefaultPositionKey (boxId));
        if (s.isEmpty())
            return false;

        const int comma = s.indexOfChar (',');
        if (comma < 0)
            return false;

        outPosition = { s.substring (0, comma).getIntValue(),
                        s.substring (comma + 1).getIntValue() };
        return true;
    }
}

//==============================================================================
DraggablePanel::DraggablePanel()
{
    // The panel itself is transparent and only carries the boxes.
    setOpaque (false);
}

DraggablePanel::~DraggablePanel()
{
    saveLayout();
}

DraggableComponent& DraggablePanel::addComponent (std::unique_ptr<DraggableComponent> box)
{
    jassert (box != nullptr);
    auto* raw = box.get();
    boxes.add (std::move (box));
    addAndMakeVisible (*raw);
    return *raw;
}

void DraggablePanel::paint (juce::Graphics& g)
{
    // A subtle dark backdrop makes the boxes stand out without looking busy.
    g.fillAll (juce::Colours::black.withAlpha (0.25f));
}

void DraggablePanel::resized()
{
    // Layout is free-form; boxes are moved by the user, not by the panel.
    // Nothing to do here on purpose.
}

//==============================================================================
juce::PropertiesFile& DraggablePanel::getPropertiesFile()
{
    if (propertiesFile == nullptr)
    {
        juce::PropertiesFile::Options options;
        options.applicationName     = "SmolFM";
        options.folderName          = "SmolFM";
        options.filenameSuffix      = ".layout";
        options.osxLibrarySubFolder = "Application Support";
        options.commonToAllUsers    = false;
        options.ignoreCaseOfKeyNames = false;

        propertiesFile = std::make_unique<juce::PropertiesFile> (options);
    }

    return *propertiesFile;
}

void DraggablePanel::saveLayout()
{
    auto& props = getPropertiesFile();
    for (const DraggableComponent* box : boxes)
        writeBoxPosition (props, *box);

    props.saveIfNeeded();
}

void DraggablePanel::loadLayout()
{
    auto& props = getPropertiesFile();
    for (DraggableComponent* box : boxes)
    {
        juce::Point<int> pos;
        if (readBoxPosition (props, box->getBoxId(), pos))
        {
            // Clamp to current panel bounds so a stale layout never puts a
            // box off-screen on a smaller window.
            pos.x = juce::jlimit (0, juce::jmax (0, getWidth()  - box->getWidth()),  pos.x);
            pos.y = juce::jlimit (0, juce::jmax (0, getHeight() - box->getHeight()), pos.y);
            box->setTopLeftPosition (pos);
        }
    }
}

} // namespace gui
