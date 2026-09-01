/*
    SmolFmFile bundles everything that defines one SmolFM patch
    into a single .smolfm XML document.

    Every node carries its OWN parameters as attributes — carrier carries its
    frequency + waveform, fm its amount, adsr its four envelope times.  Next to
    those sit the two arrangement blocks: box position, pin list (with port
    types) and the wiring that describes the signal flow.

    Loading restores parameters through the APVTS so sliders and voices follow
    immediately.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GraphNodes.h"

namespace gui { class DraggablePanel; }

namespace smolfm
{

class SmolFmFile
{
public:
    /**
        Serialise the full editor state to a .smolfm XML file.

        @param panel   the graph panel (box positions + wiring)
        @param apvts   the parameter state (sound-defining values)
        @param file    destination file (extension .smolfm is enforced)
        @return true on success
    */
    static bool save (gui::DraggablePanel& panel,
                      juce::AudioProcessorValueTreeState& apvts,
                      const juce::File& file);

    /**
        Load a .smolfm XML file and apply it to the editor.

        Restores APVTS parameters (via replaceState), box positions and
        wiring.  Missing pieces keep their current values so a partial file
        does not brick the patch.

        @param panel   the graph panel to update
        @param apvts   the parameter state to overwrite
        @param file    the .smolfm file to read
        @return true on success
    */
    static bool load (gui::DraggablePanel& panel,
                      juce::AudioProcessorValueTreeState& apvts,
                      const juce::File& file);

    /**
        Read only the instrument name of a .smolfm file.

        Falls back to the file name without extension when the file has no
        name attribute.  Cheap: parses just the XML root element.
    */
    static juce::String readInstrumentName (const juce::File& file);

    /**
        Write the instrument name into a .smolfm file without touching the
        rest of the document.  Safe to call after save().
    */
    static bool writeInstrumentName (const juce::File& file, const juce::String& name);
};

} // namespace smolfm
