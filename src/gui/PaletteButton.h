/*
    PaletteButton is one square tile in the right-side palette grid: an icon
    (or the node type's letters) on top, a tight control row of [-] count [+]
    at the bottom.  The +/− buttons ask the editor to add/remove a node of
    this type; when the budget is used up the tile dims and + greys out.

    Icons arrive as juce::Path factories (stroke glyphs sketched in a virtual
    24x24 box); the tile scales and strokes them so every icon shares one line
    weight.  Tiles without an icon fall back to their label letters.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../graph/GraphNodes.h"

namespace gui
{

class PaletteButton final : public juce::SettableTooltipClient, public juce::Component
{
public:
    PaletteButton()
    {
        addAndMakeVisible (plusButton);
        addAndMakeVisible (minusButton);
        addAndMakeVisible (countLabel);

        countLabel.setJustificationType (juce::Justification::centred);
        countLabel.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        countLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey.withAlpha (0.9f));
        countLabel.setInterceptsMouseClicks (false, false);

        plusButton.onClick = [this]
        {
            if (onCountChanged != nullptr)
                onCountChanged (nodeBaseId, true);
        };
        minusButton.onClick = [this]
        {
            if (onCountChanged != nullptr)
                onCountChanged (nodeBaseId, false);
        };

        setSize (56, 56);
    }

    /** The optional icon factory replaces the letters with a stroke glyph
        (a juce::Path sketched inside a virtual 24x24 box). */
    void configure (const juce::String& baseId,
                    const juce::String& label,
                    int maxCount,
                    std::function<juce::Path()> icon = nullptr)
    {
        nodeBaseId = baseId;
        maxInstances = maxCount;
        remainingCount = maxCount;   // start with everything available
        nameText = label.toUpperCase();

        // Tooltip title: the registry's display title (the same source the
        // box title bars use); letters as fallback for an unknown type.
        const smolfm::NodeSpec* spec = smolfm::GraphNodeRegistry::findSpec (baseId);
        tooltipName = spec != nullptr ? spec->title : nameText;

        iconFactory = std::move (icon);
        updateDisplay();
    }

    void setRemaining (int remaining)
    {
        remainingCount = juce::jlimit (0, maxInstances, remaining);
        updateDisplay();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Tile surface: flat dark fill, hairline border that lifts on hover.
        const bool over = isMouseOver();
        g.setColour (juce::Colour (over ? 0xff33383f : 0xff2a2d33));
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (juce::Colours::white.withAlpha (over ? 0.16f : 0.07f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

        // Icon (or letters), dimmed once the budget is used up.
        const float alpha = remainingCount > 0 ? 0.88f : 0.35f;
        g.setColour (juce::Colours::white.withAlpha (alpha));

        if (iconFactory)
        {
            auto glyph = iconFactory();
            const auto transform = glyph.getTransformToScaleToFit (
                iconArea.toFloat().reduced (2.0f), true);
            g.strokePath (glyph, juce::PathStrokeType (1.8f,
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded),
                          transform);
        }
        else
        {
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawFittedText (nameText, iconArea, juce::Justification::centred, 1);
        }
    }

    void resized() override
    {
        auto body = getLocalBounds().reduced (4);

        // Tight control row: - count +  (2px gaps, nothing to speak of).
        auto row = body.removeFromBottom (16);
        minusButton.setBounds (row.removeFromLeft (16));
        row.removeFromLeft (2);
        plusButton.setBounds (row.removeFromRight (16));
        row.removeFromRight (2);
        countLabel.setBounds (row);

        body.removeFromBottom (2);
        iconArea = body;
    }

    std::function<void (const juce::String& baseId, bool add)> onCountChanged;

private:
    void updateDisplay()
    {
        countLabel.setText (juce::String (remainingCount), juce::dontSendNotification);
        plusButton.setEnabled (remainingCount > 0);
        minusButton.setEnabled (remainingCount < maxInstances);

        // Tooltips carry the node's real name plus the remaining budget, so
        // the dimmed tiles explain themselves.
        setTooltip (tooltipName + "\n" + juce::String (remainingCount) + " of "
                    + juce::String (maxInstances) + " free");

        repaint();
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { repaint(); }

    /** Minimal square control button: flat fill with hover/down states and a
        crisp +/- glyph instead of the LookAndFeel's text chrome. */
    class GlyphButton final : public juce::Button
    {
    public:
        explicit GlyphButton (bool drawPlus) : juce::Button ("palette-glyph"), plus (drawPlus) {}

        void paintButton (juce::Graphics& g,
                          bool over,
                          bool down) override
        {
            const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
            const bool enabled = isEnabled();

            auto fill = juce::Colour (0xff3a4048);
            if (enabled && down)      fill = juce::Colour (0xff59606a);
            else if (enabled && over) fill = juce::Colour (0xff494f59);

            g.setColour (fill.withAlpha (enabled ? 1.0f : 0.3f));
            g.fillRoundedRectangle (bounds, 3.0f);

            const auto centre = bounds.getCentre();
            const float len = 7.0f;
            juce::Path glyph;
            glyph.startNewSubPath (centre.x - len * 0.5f, centre.y);
            glyph.lineTo (centre.x + len * 0.5f, centre.y);
            if (plus)
            {
                glyph.startNewSubPath (centre.x, centre.y - len * 0.5f);
                glyph.lineTo (centre.x, centre.y + len * 0.5f);
            }

            g.setColour (juce::Colours::white.withAlpha (enabled ? 0.92f : 0.3f));
            g.strokePath (glyph, juce::PathStrokeType (1.6f,
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        }

    private:
        bool plus;
    };

    juce::String nodeBaseId;
    int maxInstances = 0;
    int remainingCount = 0;
    juce::String nameText;
    juce::String tooltipName;
    std::function<juce::Path()> iconFactory;
    juce::Rectangle<int> iconArea;

    GlyphButton plusButton { true };
    GlyphButton minusButton { false };
    juce::Label countLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaletteButton)
};

} // namespace gui