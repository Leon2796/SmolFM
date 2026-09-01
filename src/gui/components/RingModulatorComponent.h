/*
    RingModulatorComponent is the face of a ring modulator node.

    The node itself is parameterless (out = in1 * in2), so the component
    only shows a small static hint.  The pins on the box frame carry the
    actual signal routing.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{

class RingModulatorComponent final : public juce::Component
{
public:
    RingModulatorComponent();
    ~RingModulatorComponent() override = default;

    void resized() override;

private:
    juce::Label infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingModulatorComponent)
};

} // namespace gui
