/*
    ModIndicatorOverlay draws the *effective* (modulated) control values on
    top of a rotary slider, so the UI stops telling a lie while a mod port
    bends a parameter (velocity deliberately stays sound-only and never
    moves the needle — a per-hit accent is articulation, not a patch move):

        - ghost needle: ONE dot following the effective band with meter
          ballistics (exponential chase toward the band centre, ~25 ms
          settle).  A slow mover (e.g. an LFO sine) makes it pendulate
          smoothly between the true extremes at the modulation's own tempo;
          an audio-rate mover blurs it toward the band centre instead of
          flicking to arbitrary points of the ring.
        - band: a translucent arc between the envelope edges of the
          observed window bands — attack instant, release ~0.5 s.  Wide
          while a mover sweeps (or while several voices spread), shrinking
          to nothing when the control rests at a constant value.

    The overlay never moves the slider's thumb or its attachment — the thumb
    stays the *parameter*, the needle shows the *modulation* (the Vital/
    Serum ghost-needle pattern).  Values arrive through a provider callback
    polled at 30 Hz (the MasterOutputComponent meter precedent); they are
    NeedleWindowTelemetry bands — the min/max of the effective value over a
    ~5 ms window (src/EffectiveRangeTelemetry.h), so the poll cannot alias
    a moving signal into random sample points.  A NaN-length range — no
    voice playing, instance not rendered — hides the drawing and re-seeds
    the animation; a degenerate (v, v) band draws the resting needle only.
    Never test the no-data case with Range::isEmpty(): it is also true for
    (v, v) (see the protocol comment in src/EffectiveRangeTelemetry.h —
    that trap once hid the needle forever).
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace gui
{

class ModIndicatorOverlay final : public juce::Component,
                                  private juce::Timer
{
public:
    /** Effective value band in slider units; a NaN-length range shows nothing. */
    using RangeProvider = std::function<juce::Range<float>()>;

    /**
        Create the overlay on top of a rotary slider.  The overlay attaches
        itself as a child of the slider (painted above its arcs, no mouse
        intercepted — drags reach the slider untouched) and runs its own
        30 Hz poll, like the master meter.
    */
    ModIndicatorOverlay (juce::Slider& rotarySlider, RangeProvider effectiveRangeProvider);
    ~ModIndicatorOverlay() override;

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    juce::Slider& slider;
    RangeProvider rangeProvider;

    // Animation state, all in skew-aware slider proportions (0..1).  The
    // timer callback (writer) and paint (reader) both run on the message
    // thread, so plain members are enough — no atomics on the UI side.
    bool  haveAnimationState = false;   // false while hidden: re-seed, don't ease
    float needlePos = 0.0f;             // the animated needle dot
    float arcLo = 0.0f, arcHi = 0.0f;   // the band arc's envelope edges

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModIndicatorOverlay)
};

} // namespace gui