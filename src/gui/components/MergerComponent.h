/*
    MergerComponent is the face of a merger node.

    The node itself is parameterless (out = (in1.l, in2.r)), so the component
    only shows a small static hint.  The pins on the box frame carry the
    actual signal routing.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{

class MergerComponent final : public juce::Component
{
public:
    MergerComponent();
    ~MergerComponent() override = default;

    void resized() override;

private:
    juce::Label infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MergerComponent)
};

} // namespace gui