/*
    NoteNodeComponent is the UI placeholder for the NoteProcessor in the
    signal-graph view.

    The NoteProcessor converts the last played MIDI note into a frequency
    that feeds the carrier oscillator.  This panel shows a static readout
    rather than editable controls: the MIDI input itself is the "control".
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{

class NoteNodeComponent final : public juce::Component
{
public:
    NoteNodeComponent();
    ~NoteNodeComponent() override = default;

    void resized() override;

private:
    juce::Label infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteNodeComponent)
};

} // namespace gui
