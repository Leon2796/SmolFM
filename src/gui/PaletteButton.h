/*
    PaletteButton is one tile in the editor toolbar: an icon + a badge showing
    how many of that node type may still be added.  Clicking asks the panel to
    instantiate a new node of this type.  When the budget hits zero the button
    greys out.

*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../graph/GraphNodes.h"

namespace gui
{

class PaletteButton final : public juce::Component,
                            private juce::Button::Listener
{
public:
    PaletteButton()
    {
        addAndMakeVisible (plusButton);
        addAndMakeVisible (minusButton);
        addAndMakeVisible (countLabel);
        addAndMakeVisible (nameLabel);

        plusButton.addListener (this);
        minusButton.addListener (this);

        plusButton.setButtonText ("+");
        minusButton.setButtonText ("-");

        countLabel.setJustificationType (juce::Justification::centred);
        countLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        countLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        countLabel.setBorderSize (juce::BorderSize<int> (1));
        countLabel.setColour (juce::Label::outlineColourId, juce::Colours::grey.withAlpha (0.5f));

        nameLabel.setJustificationType (juce::Justification::centred);
        nameLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        nameLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);

        setSize (80, 52);
    }

    void configure (const juce::String& baseId,
                    const juce::String& label,
                    int maxCount)
    {
        nodeBaseId = baseId;
        maxInstances = maxCount;
        remainingCount = maxCount;  // Start with all available
        nameLabel.setText (label.toUpperCase(), juce::dontSendNotification);
        updateDisplay();
    }

    void setRemaining (int remaining)
    {
        remainingCount = juce::jlimit (0, maxInstances, remaining);
            updateDisplay();
        }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour (juce::Colour (0xff2a2d33));
        g.fillRoundedRectangle (bounds, 6.0f);

        // Border
        g.setColour (juce::Colours::grey.withAlpha (0.3f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (6);  // 6px margin

        // Name label at top
        nameLabel.setBounds (bounds.removeFromTop (18));

        bounds.removeFromTop (4);  // spacing

        // Button row
        auto row = bounds.removeFromTop (26);
        const int btnSize = 24;
        const int spacing = 4;
        const int countWidth = row.getWidth() - 2 * btnSize - 2 * spacing;

        minusButton.setBounds (row.removeFromLeft (btnSize));
        row.removeFromLeft (spacing);
        countLabel.setBounds (row.removeFromLeft (countWidth));
        row.removeFromLeft (spacing);
        plusButton.setBounds (row);
    }

    std::function<void (const juce::String& baseId, bool add)> onCountChanged;

private:
    void buttonClicked (juce::Button* button) override
    {
        if (onCountChanged == nullptr)
            return;

        if (button == &plusButton)
            onCountChanged (nodeBaseId, true);
        else if (button == &minusButton)
            onCountChanged (nodeBaseId, false);
    }

    void updateDisplay()
    {
        countLabel.setText (juce::String (remainingCount), juce::dontSendNotification);
        plusButton.setEnabled (remainingCount > 0);
        minusButton.setEnabled (remainingCount < maxInstances);
    }

    juce::String nodeBaseId;
    int maxInstances = 0;
    int remainingCount = 0;

    juce::TextButton plusButton;
    juce::TextButton minusButton;
    juce::Label countLabel;
    juce::Label nameLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaletteButton)
};

} // namespace gui

