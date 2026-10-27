/*
    Processor.h defines the base class for all signal-processing nodes in SmolFM.

    The graph is deliberately simple: every node produces one StereoFrame (a
    left/right sample pair) per call to processSample().  Frequency-domain
    ports keep their scalar semantics; their values travel as centered pairs
    (both channels equal), so the scalar Hertz value is the .l channel.
    Connections are explicit output->input ports, so the graph can later be
    traversed, validated and visualised without touching the audio math.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>

namespace smolfm
{

/**
    Semantic type tags for ports.

    A port carries one StereoFrame per tick.  Using an enum class prevents
    accidental connections between incompatible signal kinds.
*/
enum class PortType
{
    signal,

    /**
        Carries a frequency in Hertz rather than an amplitude sample.

        Frequencies are conceptually a different "kind" of value from audio
        samples.  They only flow from sources that know how to produce
        Hertz (NoteProcessor) into an oscillator's note_in port.
    */
    frequency,

    /**
        Carries the velocity (per-note attack strength, MIDI 0..1) as a
        centered pair.

        Velocity is its own domain, distinct from audio and Hertz: sources
        emit it per note-on (NoteProcessor vel output), consumers use it to
        scale a parameter (accent gain, drive, FM index, cutoff).  Velocity
        inputs read 1.0 when unconnected, so an unwired vel pin keeps the
        processor's sound exactly as before (sound-invariance).
    */
    velocity
};

/**
    Display-only stereo side hint for signal ports.

    Pure UI metadata: it never influences connect() or the audio math.  A
    splitter/merger can set it on its ports so the UI can label a wire with
    the side it carries ("L" / "R" badge).  Defaults to none.
*/
enum class ChannelSide
{
    none,
    left,
    right
};

/**
    One audio sample as a left/right pair.

    Since the implicit stereo conversion (Welt A) the standard signal type
    carries a pair per tick instead of a single float.  Mono sources emit a
    centered pair (both channels equal), so all previous mono-style patches
    are preserved: they simply produce identical L and R.
*/
struct StereoFrame
{
    float l = 0.0f;
    float r = 0.0f;

    /** Both channels carry the same value (mono content, frequencies). */
    static StereoFrame centered (float value) noexcept
    {
        return { value, value };
    }
};

/**
    Every processor has a unique role so the graph knows what kind of node it is.
*/
enum class ProcessorRole
{
    oscillator,
    fmModulator,
    frequencyScale,
    adsr,
    masterOutput,
    ringModulator,
    generic
};

/**
    Base class for every node in the SmolFM processing graph.

    A Processor is owned by one voice.  It is prepared once, started for each
    note, and then called once per sample.  All memory must be pre-allocated;
    processSample() must be real-time safe.
*/
class Processor
{
public:
    explicit Processor (ProcessorRole roleInGraph)
        : role (roleInGraph)
    {
    }

    virtual ~Processor() = default;

    /**
        Configure the processor for a new sample rate.

        Called from SynthVoice::prepare().
    */
    virtual void prepare (double newSampleRate) = 0;

    /**
        Reset state for a new note.

        Called from SynthVoice::startNote().
    */
    virtual void startNote() = 0;

    /**
        Produce one output frame and advance internal state.

        The processor reads its connected inputs before producing output.
        Frequency-domain processors return a centered pair; the scalar Hertz
        value is then the .l channel.
    */
    virtual StereoFrame processSample() = 0;

    /**
        Wire the optional bypass parameter (an APVTS bool raw value:
        0 = process, non-zero = bypass).

        A bypassed processor returns its signal/fq input frame unchanged and
        does NOT advance its internal state — the state is remembered
        (frozen), so un-bypassing resumes the DSP exactly where it left off
        (delay tail, envelope phase, filter integrators).  A bypassed source
        (oscillator) has no input frame to pass through: it writes an exact
        zero frame and parks its phase the same way.  A null pointer
        (the default) means "always process": a processor that never receives
        bypass wiring behaves byte-identically to before.
    */
    void setBypassParam (std::atomic<float>* bypassSource) noexcept
    {
        bypassParam = bypassSource;
    }

    /** True while the wired bypass parameter is set (non-zero). */
    bool isBypassed() const noexcept
    {
        return bypassParam != nullptr && bypassParam->load() > 0.5f;
    }

    /**
        Return the role of this processor in the graph.
    */
    ProcessorRole getRole() const noexcept
    {
        return role;
    }

private:
    ProcessorRole role;

    // Optional bypass source (APVTS bool): non-zero = neutral passthrough
    // with the internal state frozen (see setBypassParam).
    std::atomic<float>* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
};

} // namespace smolfm
