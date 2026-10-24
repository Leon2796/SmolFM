/*
    SmolFM_GridTests: assert-based self-check for the pure unit-grid geometry
    behind the DraggablePanel's grid mode (gui/GridDocker.h).  Console
    executable, no GUI components needed.
*/

#include <iostream>
#include <string>
#include <vector>

#include "src/gui/GridDocker.h"

using namespace gui;
namespace
{
    int failures = 0;

    void expectTrue (bool condition, const std::string& what)
    {
        if (! condition)
        {
            std::cout << "FAIL: " << what << std::endl;
            ++failures;
        }
        else
        {
            std::cout << "ok:   " << what << std::endl;
        }
    }

    /** Cell lattice invariants: sizes round up to whole cells (never below
        one cell, exact multiples stay), coordinates quantize to the nearest
        cell corner (halfway up), negatives clamp into the origin cell. */
    void testSnapping()
    {
        expectTrue (grid::snapTo (0) == 0, "snapTo(0) stays in the origin cell");
        expectTrue (grid::snapTo (59) == 0, "snapTo(59) rounds down to the origin cell");
        expectTrue (grid::snapTo (60) == grid::unit, "halfway points round up to the next cell");
        expectTrue (grid::snapTo (grid::unit) == grid::unit, "cell corners stay cell corners");
        expectTrue (grid::snapTo (-5) == 0, "snapTo clamps negatives into the origin cell");

        expectTrue (grid::snapUp (1) == grid::unit, "anything below one cell grows to one cell");
        expectTrue (grid::snapUp (grid::unit) == grid::unit, "exact cell multiples stay unchanged");
        expectTrue (grid::snapUp (grid::unit + 1) == 2 * grid::unit, "one px over a multiple grows to the next cell");

        expectTrue (grid::snapUp (228) == 240, "oscillator box width docks to 2 cells");
        expectTrue (grid::snapUp (194) == 240, "oscillator box height docks to 2 cells");
        expectTrue (grid::snapUp (398) == 480, "resonator box width docks to 4 cells");
        expectTrue (grid::snapUp (120) == grid::unit, "gain box height docks to 1 cell");
    }

    /** Docking placement: nearest cell without obstacles, flush neighbours
        allowed, and keepInside honoured (with the overlap fallback when the
        region is fully packed). */
    void testDocking()
    {
        const juce::Rectangle<int> oscRequest (150, 130, 228, 194);

        std::vector<juce::Rectangle<int>> empty;
        expectTrue (grid::dockedBounds (oscRequest, empty)
                        == juce::Rectangle<int> (120, 120, 240, 240),
                    "an unconstrained box docks onto its nearest cell corner");

        // One 2x2 block in the way: the dock walk must land in the adjacent
        // free cell to the right (ring order: up, left, right, down, ...).
        std::vector<juce::Rectangle<int>> occupied { juce::Rectangle<int> (0, 0, 240, 240) };
        const juce::Rectangle<int> rival (0, 0, 240, 240);

        expectTrue (grid::dockedBounds (rival, occupied)
                        == juce::Rectangle<int> (240, 0, 240, 240),
                    "a rival box walks to the nearest free cell");

        // Edge-adjacent cells must not count as overlaps (flush tiling).
        std::vector<juce::Rectangle<int>> rightNeighbour { juce::Rectangle<int> (240, 0, 240, 240) };
        expectTrue (grid::dockedBounds (rival, rightNeighbour)
                        == juce::Rectangle<int> (0, 0, 240, 240),
                    "a flush neighbour does not push the box away");

        // keepInside: the preferred cell lies outside the region, so the box
        // must land on the nearest cell corner inside it.
        std::vector<juce::Rectangle<int> > blocked { juce::Rectangle<int> (480, 0, 240, 240) };
        expectTrue (grid::dockedBounds (juce::Rectangle<int> (480, 0, 240, 240), blocked,
                                        juce::Rectangle<int> (0, 0, 480, 240))
                        == juce::Rectangle<int> (240, 0, 240, 240),
                    "keepInside pulls the docked box back into the region");

        // A fully packed keepInside falls back to the snapped request.
        std::vector<juce::Rectangle<int>> packed { juce::Rectangle<int> (0, 0, 240, 240) };
        expectTrue (grid::dockedBounds (rival, packed, juce::Rectangle<int> (0, 0, 240, 240))
                        == juce::Rectangle<int> (0, 0, 240, 240),
                    "a fully packed keepInside falls back to the snapped request");
    }

    /** Sequential docking of six identical 2x2 blocks: every tile keeps its
        size, sits on a corner, avoids every earlier tile and stays near the
        preferred origin cell. */
    void testSequentialDocking()
    {
        std::vector<juce::Rectangle<int>> occupied;
        const juce::Rectangle<int> request (0, 0, 240, 240);
        const juce::Rectangle<int> neighbourhood (0, 0, 8 * grid::unit, 8 * grid::unit);

        for (int i = 0; i < 6; ++i)
        {
            const juce::Rectangle<int> placed = grid::dockedBounds (request, occupied);

            expectTrue (placed.getWidth() == 240 && placed.getHeight() == 240,
                        "tile " + std::to_string (i) + " keeps its 2x2 cell size");

            expectTrue (placed.getX() % grid::unit == 0 && placed.getY() % grid::unit == 0,
                        "tile " + std::to_string (i) + " sits on a cell corner");

            for (const juce::Rectangle<int>& earlier : occupied)
                expectTrue (! placed.intersects (earlier),
                            "tile " + std::to_string (i) + " avoids the earlier tiles");

            expectTrue (neighbourhood.contains (placed),
                        "tile " + std::to_string (i) + " stays near the preferred cell");

            occupied.push_back (placed);
        }
    }

} // namespace

int main()
{
    testSnapping();
    testDocking();
    testSequentialDocking();

    std::cout << (failures == 0 ? "All grid tests passed."
                                : "GRID TESTS FAILED")
              << std::endl;

    return failures == 0 ? 0 : 1;
}