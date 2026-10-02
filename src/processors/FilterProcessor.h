/*
    FilterProcessor shapes a signal through a multi-mode state-variable
    filter (Chamberlin/TPT style, trapezoidal integration).

    Ports:
        - in               (signal):   incoming audio.
        - cutoff_mod_in    (frequency): optional live cutoff in Hertz.  When
          connected it OVERRIDES the static cutoff parameter; when left
          unconnected the filter runs at the static cutoff value.
        - resonance_mod_in (signal):   optional multiplier on the resonance.
          The connected value's stereo mean is clamped to [-1, +1] and the
          effective resonance becomes resonance * (1 + m), clamped to 0..1.
          Unconnected: the static resonance runs as-is.
        - vel              (velocity): optional per-note velocity input
          (0-1).  Connected: the stereo mean scales the cutoff (accented
          hits sit brighter, soft hits darker).  Unconnected: transparent.
        - out              (signal)   : the filtered audio.

    Parameters:
        - cutoff    20-20000 Hz (log), the static base cutoff.
        - resonance 0-1, maps to the SVF damping k = 2 * (1 - 0.99 * res);
          k = 2 is maximally damped (Q = 0.5), k -> 0 approaches self-
          oscillation but stays stable.
        - mode      0 = LP, 1 = BP, 2 = HP, 3 = Notch.

    Per sample the TPT state-variable filter integrates with:

        g   = tan(pi * fc / fs)
        k   = 2 * (1 - 0.99 * resonance)
        a1  = 1 / (1 + g * (g + k)),  a2 = g * a1,  a3 = g * a2

        v3  = x - ic2eq
        v1  = a1 * ic1eq + a2 * v3
        v2  = ic2eq + a2 * ic1eq + a3 * v3
        ic1eq = 2*v1 - ic1eq;  ic2eq = 2*v2 - ic2eq

        LP    = v2
        BP    = v1
        HP    = x - k*v1 - v2
        Notch = HP + v2   (= x - k*v1)

    Stereo: the coefficients depend only on the cutoff and are shared by
    both channels; only the TPT state (ic1eq/ic2eq) is a left/right pair,
    integrated per channel.

    Cutoff is re-derived every sample, so cutoff_mod_in can sweep freely
    (envelope, FM chain, keyboard tracking) without zipper artifacts.
*/

#pragma once

#include "ProcessorPort.h"

#include <atomic>

namespace smolfm
{

/**
    Multi-mode filter node with static and dynamically modulated cutoff.
*/
class FilterProcessor final : public Processor
{
public:
    /**
        Create the filter node.

        @param cutoffParameter    atomic pointer to the static cutoff (Hz)
        @param resonanceParameter atomic pointer to the resonance (0-1)
        @param modeParameter      atomic pointer to the mode index (0-3)
    */
    FilterProcessor (std::atomic<float>* cutoffParameter,
                     std::atomic<float>* resonanceParameter,
                     std::atomic<float>* modeParameter);

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    /**
        Zero the TPT integrator state.

        applyConnectionPatch calls this so a new patch cannot inherit a
        ringing filter tail from the previous patch (mirrors the delay
        reset there).
    */
    void reset() noexcept;

    InputPort& getInput() noexcept            { return input; }
    InputPort& getCutoffModInput() noexcept   { return cutoffMod; }
    InputPort& getResonanceModInput() noexcept { return resonanceMod; }
    InputPort& getVelInput() noexcept         { return velIn; }
    OutputPort& getOutput() noexcept          { return output; }

private:
    std::atomic<float>* cutoff;
    std::atomic<float>* resonance;
    std::atomic<float>* mode;

    InputPort input       { PortType::signal };
    InputPort cutoffMod   { PortType::frequency };
    InputPort resonanceMod { PortType::signal };
    InputPort velIn        { PortType::velocity };
    OutputPort output   { PortType::signal, *this };

    double sampleRate = 44100.0;
    StereoFrame ic1eq;   // TPT integrator state 1 (left/right pair)
    StereoFrame ic2eq;   // TPT integrator state 2 (left/right pair)
};

} // namespace smolfm