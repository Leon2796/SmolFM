/*
    VelocityScaleProcessor rescales the velocity domain before consumers read
    it.

    Ports:
        - vel_in (velocity): the incoming velocity value (0-1), wired from
                 note.vel or another scaler.
        - out    (velocity): the scaled velocity, clamped to 0..1.

    Per sample (per channel):

        v_out = clamp (curve (v_in) * scale, 0.0f, 1.0f)

    The curve parameter is the perceptual shortcut: "linear" passes the MIDI
    velocity as-is, "sqrt" fattens soft hits (the square root compresses the
    low end upward — the natural loudness-feel), "square" thins soft hits and
    reserves the top of the range for hard accents.  The scale parameter
    multiplies on top; 1.0 keeps the curve output untouched.

    The velocity domain mirrors the frequency-domain scaler (fscale): both
    are chainable, stateless, and neutral when idle.  curve = linear with
    scale = 1.0 passes the velocity through unchanged, so an idle scaler
    costs nothing.  Unwired input reads the velocity-domain default (1.0),
    which also maps through the curve to 1.0 — an unwired node is inert, not
    silent, because velocity flows as level/timbre modifiers, not as audio.

    Purpose: analog-hardware simulations assume their own accent laws (the
    TR-808's trigger voltage is not raw linear MIDI velocity), so patches
    need a place to bend the velocity domain before it reaches
    gain.vel / shape.vel / resonator.vel / filter.vel / fm.vel ports.
*/

#pragma once

#include "ProcessorPort.h"

namespace smolfm
{

/**
    Number of curve shortcut choices; must match the choice count in
    PluginProcessor::createParameterLayout() and the name table in
    ISmolFmParser.h (vscCurveNames).
*/
inline constexpr int numVscCurves = 3;

/**
    Applies one velocity curve.  Order must stay in sync with:

        PluginProcessor::createParameterLayout()  (APVTS choice order)
        ISmolFmParser.h::vscCurveNames()          (file attribute names)
        VelocityScaleComponent                    (dropdown item order)
*/
inline float vscCurveFromIndex (int index, float velocity) noexcept
{
    if (velocity <= 0.0f)
        return 0.0f;

    switch (index)
    {
        case 1:  return std::sqrt (velocity);   // sqrt: soft hits fatten
        case 2:  return velocity * velocity;    // square: soft hits thin
        default: return velocity;               // linear (also for bad indices)
    }
}

/**
    Chainable velocity scaler operating on velocity values.
*/
class VelocityScaleProcessor final : public Processor
{
public:
    /**
        Create the velocity scale node.

        @param curveParameter atomic pointer to the curve choice index
                              (0 = linear; see vscCurveFromIndex).
        @param scaleParameter atomic pointer to the scale factor (0.25-4;
                              1.0 means transparent).
    */
    explicit VelocityScaleProcessor (std::atomic<float>* curveParameter,
                                     std::atomic<float>* scaleParameter);

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    InputPort& getVelInput() noexcept  { return velInput; }
    OutputPort& getOutput() noexcept   { return output; }

private:
    std::atomic<float>* curve;
    std::atomic<float>* scale;

    InputPort velInput { PortType::velocity };
    OutputPort output  { PortType::velocity, *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelocityScaleProcessor)
};

} // namespace smolfm