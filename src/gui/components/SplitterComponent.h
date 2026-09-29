/*
    SplitterComponent is the face of a splitter node.

    The node itself is parameterless (in -> outL = (L,0), outR = (0,R)), so
    the component only shows a small static hint.  The pins on the box frame
    carry the actual signal routing.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{

class SplitterComponent final : public juce::Component
{
public:
    SplitterComponent();
    ~SplitterComponent() override = default;

    void resized() override;

private:
    juce::Label infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SplitterComponent)
};

} // namespace gui