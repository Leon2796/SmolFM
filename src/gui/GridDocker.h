/*
    GridDocker holds the pure unit-grid geometry behind the DraggablePanel's
    grid mode.

    The grid is made of `unit` px squares; every box is sized up to a whole
    number of cells and parks on a cell corner.  When the requested spot is
    taken, the box walks outward in rings of increasing manhattan distance
    until a free cell region is found — boxes tile flush (no overlaps, no
    gaps) and nothing ever jumps far.

    Header-only and free of GUI state, so the geometry is unit-testable
    (SmolFM_GridTests) without any components.
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

#include <vector>

namespace gui
{
namespace grid
{

/** Edge length of one unit square ("cell") in pixels.  In grid mode every
    box occupies n x m cells (1x1, 2x1, 2x2, ...).  120 fits the smallest
    real box shape (108 px) while keeping the wide shapes at 2..4 cells. */
constexpr int unit = 120;

/** Round a pixel coordinate (values < 0 clamp to the origin cell) to the
    nearest cell corner; exact halfway points round up. */
inline int snapTo (int px) noexcept
{
    return ((juce::jmax (0, px) + unit / 2) / unit) * unit;
}

/** Round a length up to a whole number of cells; a box is never smaller
    than one cell. */
inline int snapUp (int length) noexcept
{
    return juce::jmax (unit, ((juce::jmax (0, length) + unit - 1) / unit) * unit);
}

/**
    Docking placement for one box: snap the requested bounds to cell
    multiples; while the result overlaps one of the `occupied` rectangles,
    probe the neighbouring cells, ordered by increasing manhattan distance
    from the requested corner, until a free one is found.

    `keepInside` (when not empty) restricts candidates to that region, so an
    interactive placement never docks out of reach of the viewport.

    Edge-adjacent cells do not count as overlapping (strict juce::Rectangle
    semantics), which is what makes flush grid tiling possible.
*/
inline juce::Rectangle<int> dockedBounds (const juce::Rectangle<int>& requested,
                                          const std::vector<juce::Rectangle<int>>& occupied,
                                          const juce::Rectangle<int>& keepInside = {})
{
    const juce::Rectangle<int> sized { snapTo (requested.getX()), snapTo (requested.getY()),
                                       snapUp (requested.getWidth()), snapUp (requested.getHeight()) };

    for (int ring = 0; ring <= 64; ++ring)
        for (int dy = -ring; dy <= ring; ++dy)
        {
            const int rem = ring - (dy < 0 ? -dy : dy);

            // Both x directions share the same manhattan distance; rem == 0
            // collapses into the single middle candidate.
            for (int sign = rem == 0 ? 1 : 0; sign < 2; ++sign)
            {
                const int x = sized.getX() + (sign == 1 ? rem : -rem) * unit;
                const int y = sized.getY() + dy * unit;

                if (x < 0 || y < 0)
                    continue;    // the canvas lives in the non-negative quadrant

                const juce::Rectangle<int> candidate { x, y, sized.getWidth(), sized.getHeight() };

                if (! keepInside.isEmpty() && ! keepInside.contains (candidate))
                    continue;

                bool free = true;
                for (const juce::Rectangle<int>& o : occupied)
                    if (candidate.intersects (o))
                    {
                        free = false;
                        break;
                    }

                if (free)
                    return candidate;
            }
        }

    // ponytail: the ring cap of 64 (7680 px reach) covers every realistic
    // canvas; the only way to land here is a keepInside region completely
    // packed by other boxes.  Upgrade path: keep walking without a cap.
    return sized;
}

} // namespace grid
} // namespace gui