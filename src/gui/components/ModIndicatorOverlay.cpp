/*
    ModIndicatorOverlay implementation.
*/

#include "ModIndicatorOverlay.h"

#include "../../EffectiveRangeTelemetry.h"   // range protocol predicate

#include <cmath>

namespace gui
{

// Animation pacing (see the header): the needle chases the incoming band's
// centre with meter ballistics, the band edges expand instantly and release
// slowly.  Both derive from the same 30 Hz tick the data arrives on — the
// master meter's pace.  ponytail: the tick is treated as exactly 1/30 s;
// timer jitter is far below what a dot this small can resolve.  Upgrade
// path: stamp Time::getMillisecondCounterHiRes() per tick and use the real
// dt.
static constexpr float tickSeconds         = 1.0f / 30.0f;
static constexpr float needleSettleSeconds = 0.025f;   // fast chase
static constexpr float arcReleaseSeconds   = 0.5f;     // slow envelope

ModIndicatorOverlay::ModIndicatorOverlay (juce::Slider& rotarySlider,
                                          RangeProvider effectiveRangeProviderIn)
    : slider (rotarySlider),
      rangeProvider (std::move (effectiveRangeProviderIn))
{
    // Paint-only overlay: the slider keeps every mouse interaction (the
    // thumb is never moved by this component, the attachment stays intact).
    setInterceptsMouseClicks (false, false);

    // Child of the slider: painted above its arcs, clipped to its bounds.
    slider.addAndMakeVisible (*this);

    startTimerHz (30);   // the MasterOutputComponent meter's pacing
}

ModIndicatorOverlay::~ModIndicatorOverlay()
{
    stopTimer();
}

void ModIndicatorOverlay::timerCallback()
{
    // Follow the slider's bounds; setBounds() is a no-op while already
    // equal, so this costs nothing between layout changes.
    setBounds (slider.getLocalBounds());

    const juce::Range<float> next = rangeProvider != nullptr
        ? rangeProvider()
        : smolfm::noEffectiveData();

    // Range protocol (see EffectiveRangeTelemetry.h): a NaN length means
    // no voice plays / the instance has not rendered — hide and re-seed.
    // isEmpty() would wrongly report a degenerate (v, v) band as empty.
    if (! smolfm::effectiveRangeHasData (next)
        || slider.getMaximum() <= slider.getMinimum())
    {
        setVisible (false);
        haveAnimationState = false;   // next data run seeds, never eases in
        return;
    }

    // Skew-aware positions: valueToProportionOfLength is the exact same
    // mapping the slider uses for its thumb, so a logarithmic cutoff scale
    // places the needle correctly without re-deriving anything here.  The
    // clamp keeps a legitimately overshooting effective value (e.g. a
    // gain factor beyond the slider's maximum) on the knob's own arc
    // instead of flicking to points somewhere in the ring.
    const float posLo = juce::jlimit (0.0f, 1.0f,
        static_cast<float> (slider.valueToProportionOfLength (next.getStart())));
    const float posHi = juce::jlimit (0.0f, 1.0f,
        static_cast<float> (slider.valueToProportionOfLength (next.getEnd())));

    const float needleAlpha = 1.0f - std::exp (-tickSeconds / needleSettleSeconds);
    const float arcAlpha    = 1.0f - std::exp (-tickSeconds / arcReleaseSeconds);

    if (haveAnimationState)
    {
        // Needle: exponential chase toward the band centre.  A slow mover's
        // window band is a sliver riding the modulated contour, so a sine
        // pendulates the needle between its true extremes; a fast mover's
        // band spans the whole excursion and the needle blurs toward its
        // centre instead of flicking to aliased sample points.
        needlePos += ((posLo + posHi) * 0.5f - needlePos) * needleAlpha;

        // Band: envelope of the observed window bands.  Attack is instant
        // (a wider band is seen immediately), release is slow (the arc
        // shows the recent modulation depth / polyphony spread).
        arcLo = juce::jmin (arcLo, posLo);
        arcHi = juce::jmax (arcHi, posHi);
        arcLo += (posLo - arcLo) * arcAlpha;
        arcHi += (posHi - arcHi) * arcAlpha;
    }
    else
    {
        // First frame after idle: seed directly, never ease in from a
        // stale position of a previous note run.
        haveAnimationState = true;
        needlePos = (posLo + posHi) * 0.5f;
        arcLo = posLo;
        arcHi = posHi;
    }

    setVisible (true);
    repaint();
}

void ModIndicatorOverlay::paint (juce::Graphics& g)
{
    if (! haveAnimationState)
        return;

    // Geometry: replicate the LookAndFeel_V4 rotary math exactly, so
    // needle and band sit pixel-exact on the knob's own arcs.  The slider
    // rect already excludes the text box below.
    const juce::Slider::SliderLayout layout
        = slider.getLookAndFeel().getSliderLayout (slider);
    const juce::Rectangle<float> bounds = layout.sliderBounds.toFloat().reduced (10.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float lineWidth = juce::jmin (8.0f, radius * 0.5f);
    const float arcRadius = radius - lineWidth * 0.5f;

    const juce::Slider::RotaryParameters params = slider.getRotaryParameters();
    const float angleRange = params.endAngleRadians - params.startAngleRadians;
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();

    // Angle convention (juce's getPointOnCircumference): 0 = 12 o'clock,
    // increasing clockwise — the domain the LookAndFeel arcs are drawn in.
    const auto angleFor = [&] (const float position)
    {
        return params.startAngleRadians + position * angleRange;
    };

    const juce::Colour ghost { juce::Colours::orange };

    // Band: translucent arc between the envelope's edges (polyphony spread
    // and modulation depth).  A degenerate band collapses the arc; the
    // needle below stays the story.  ONE dot per overlay — the old second
    // edge dot read as a duplicated needle that never moved.
    if (arcHi > arcLo)
    {
        juce::Path bandPath;
        bandPath.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                                angleFor (arcLo), angleFor (arcHi), true);
        g.setColour (ghost.withAlpha (0.35f));
        g.strokePath (bandPath, juce::PathStrokeType (lineWidth, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }

    // Ghost needle: one dot on the rim where the thumb for the animated
    // position would sit (the LookAndFeel_V4 thumb formula).
    const float angle = angleFor (needlePos);
    const juce::Point<float> needle (centreX + arcRadius * std::cos (angle - juce::MathConstants<float>::halfPi),
                                     centreY + arcRadius * std::sin (angle - juce::MathConstants<float>::halfPi));
    const float needleSize = lineWidth * 1.25f;
    g.setColour (ghost.withAlpha (0.85f));
    g.fillEllipse (juce::Rectangle<float> (needleSize, needleSize).withCentre (needle));
}

} // namespace gui