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

class PaletteButton final : public juce::Component
{
public:
    PaletteButton() = default;

    void configure (const juce::String& baseId,
                    const juce::String& label,
                    std::function<juce::Path (const juce::Rectangle<float>&)> makeIcon,
                    int maxCount)
    {
        nodeBaseId = baseId;
        nodeLabel = label;
        iconFactory = std::move (makeIcon);
        maxInstances = maxCount;
        repaint();
    }

    void setRemaining (int remaining)
    {
        const int r = juce::jmax (0, remaining);
        if (remainingCount != r)
        {
            remainingCount = r;
            setEnabled (remainingCount > 0);
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const auto tile = bounds.reduced (4.0f);

        // Rounded tile with a subtle fill; greys out when the budget is spent.
        const auto base = juce::Colour (0xff2a2d33);
        const auto edge = juce::Colours::white.withAlpha (isEnabled() ? 0.12f : 0.05f);

        g.setColour (isEnabled() ? base : base.darker (0.7f));
        g.fillRoundedRectangle (tile, 8.0f);
        g.setColour (edge);
        g.drawRoundedRectangle (tile, 8.0f, 1.0f);

        // Icon area (top).
        auto iconArea = tile.reduced (12.0f);
        iconArea.removeFromBottom (10.0f);     // leave room for label

        if (iconFactory != nullptr)
        {
            auto path = iconFactory (iconArea);
            g.setColour (isEnabled() ? juce::Colour (0xff7fd4ff).withAlpha (0.9f)
                                     : edge);
            g.strokePath (path, juce::PathStrokeType (1.8f));
        }

        // Label + remaining badge (bottom).
        g.setFont (11.0f);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.drawText (nodeLabel, getLocalBounds().removeFromBottom (22)
                                               .reduced (4, 0),
                    juce::Justification::centredLeft, true);

        const auto badge = getLocalBounds().removeFromBottom (22)
                                           .removeFromRight (26)
                                           .reduced (2);
        g.setColour (remainingCount > 0 ? juce::Colour (0xff4a90d9)
                                        : juce::Colour (0xff555a63));
        g.fillEllipse (badge.toFloat());
        g.setColour (juce::Colours::white);
        g.drawText (juce::String (remainingCount), badge, juce::Justification::centred, false);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (isEnabled() && e.mods.isLeftButtonDown() && onAddRequested != nullptr)
            onAddRequested (nodeBaseId);
    }

    std::function<void (const juce::String& baseId)> onAddRequested;

private:
    juce::String nodeBaseId;
    juce::String nodeLabel;
    std::function<juce::Path (const juce::Rectangle<float>&)> iconFactory;
    int maxInstances = 0;
    int remainingCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaletteButton)
};

} // namespace gui
