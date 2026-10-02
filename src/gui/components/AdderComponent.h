/*
    AdderComponent is the face of an adder node.

    The node itself is parameterless (out = in1 + in2), so the component
    only shows a small static hint.  The pins on the box frame carry the
    actual signal routing.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{

class AdderComponent final : public juce::Component
{
public:
    AdderComponent();
    ~AdderComponent() override = default;

    void resized() override;

private:
    juce::Label infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdderComponent)
};

} // namespace gui