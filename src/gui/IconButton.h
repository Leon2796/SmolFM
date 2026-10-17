/*
    IconButton is a small icon-only flat button for the chrome controls
    (import/export, patch browser directory + navigation).  It shares the
    palette tiles' visual language: dark flat fill, hairline border, hover
    and pressed lifts, and a stroke glyph sketched in a virtual 24x24 box
    (same convention as the palette icon factories in PluginEditor.cpp).

    Tooltips show wherever a juce::TooltipWindow lives in the component tree.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{

class IconButton final : public juce::Button
{
public:
    /** @param glyphToUse  Stroke path sketched inside a virtual 24x24 box.
        @param tooltipText   Text shown on hover. */
    IconButton (const juce::Path& glyphToUse, const juce::String& tooltipText)
        : juce::Button ("icon-button"), glyph (glyphToUse)
    {
        setTooltip (tooltipText);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        const bool enabled = isEnabled();

        // Toggle buttons (e.g. the grid-mode switch) lift into an accent
        // tint while engaged, independent of hover/press.
        const bool active = getClickingTogglesState() && getToggleState();

        auto fill = juce::Colour (0xff2a2d33);
        if (enabled && active)    fill = juce::Colour (0xff1d4a63);
        else if (enabled && down) fill = juce::Colour (0xff4d545f);
        else if (enabled && over) fill = juce::Colour (0xff373c45);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (enabled && active ? 0.30f
                                              : (enabled && over ? 0.14f : 0.07f)));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        g.setColour (juce::Colours::white.withAlpha (enabled ? (active ? 1.0f : 0.85f) : 0.3f));
        const auto transform = glyph.getTransformToScaleToFit (bounds.reduced (6.0f), true);
        g.strokePath (glyph,
                      juce::PathStrokeType (1.7f,
                                            juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded),
                      transform);
    }

private:
    juce::Path glyph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IconButton)
};

namespace icons
{
    // Chrome stroke glyphs, sketched in a virtual 24x24 box.

    inline juce::Path importGlyph()      // arrow down into a tray
    {
        juce::Path p;
        p.startNewSubPath (12, 3);
        p.lineTo (12, 13);
        p.startNewSubPath (7.5f, 9);
        p.lineTo (12, 13.5f);
        p.lineTo (16.5f, 9);
        p.startNewSubPath (3, 16);
        p.lineTo (3, 20);
        p.lineTo (21, 20);
        p.lineTo (21, 16);
        return p;
    }

    inline juce::Path exportGlyph()      // arrow up out of a tray
    {
        juce::Path p;
        p.startNewSubPath (12, 13.5f);
        p.lineTo (12, 3);
        p.startNewSubPath (7.5f, 7.5f);
        p.lineTo (12, 3);
        p.lineTo (16.5f, 7.5f);
        p.startNewSubPath (3, 16);
        p.lineTo (3, 20);
        p.lineTo (21, 20);
        p.lineTo (21, 16);
        return p;
    }

    inline juce::Path folderGlyph()
    {
        juce::Path p;
        p.startNewSubPath (3, 19);
        p.lineTo (3, 5);
        p.lineTo (9.5f, 5);
        p.lineTo (12, 7.5f);
        p.lineTo (21, 7.5f);
        p.lineTo (21, 19);
        p.closeSubPath();
        return p;
    }

    inline juce::Path paletteGlyph()     // node palette: 2x2 grid of squares
    {
        juce::Path p;
        p.addRectangle (3, 3, 7.5f, 7.5f);
        p.addRectangle (13.5f, 3, 7.5f, 7.5f);
        p.addRectangle (3, 13.5f, 7.5f, 7.5f);
        p.addRectangle (13.5f, 13.5f, 7.5f, 7.5f);
        return p;
    }

    inline juce::Path gridGlyph()        // grid mode: 3x3 lattice of unit cells
    {
        juce::Path p;
        for (int col = 0; col < 3; ++col)
            for (int row = 0; row < 3; ++row)
                p.addRectangle (3.0f + col * 7.5f, 3.0f + row * 7.5f, 6.0f, 6.0f);
        return p;
    }

    inline juce::Path chevronLeft()
    {
        juce::Path p;
        p.startNewSubPath (14.5f, 6);
        p.lineTo (8.5f, 12);
        p.lineTo (14.5f, 18);
        return p;
    }

    inline juce::Path chevronRight()
    {
        juce::Path p;
        p.startNewSubPath (9.5f, 6);
        p.lineTo (15.5f, 12);
        p.lineTo (9.5f, 18);
        return p;
    }
}

} // namespace gui