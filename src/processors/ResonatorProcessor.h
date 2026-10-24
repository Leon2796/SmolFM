/*
    ResonatorProcessor renders the excited damped second-order response of a
    drum body (TR-808 plan P2.2, A1: Bridged-T bodies of BD/toms/congas/
    snare bodies/claves/rimshot).

    The node is a *resonator*, not an oscillator: its core is one complex
    pole pair (two state variables y[n-1], y[n-2] in a feedback loop) that
    is displaced by the exciter input and then releases the stored energy as
    a decaying near-sinusoid.  Decay and pitch are properties of that same
    pole pair — there is no envelope layered on top, the state vector itself
    shrinks.

    Ports:
        - in       (signal)   : the exciter (impulse, click, noise, tone...).
          Its spectrum decides how much energy lands in the resonance mode.
        - tune     (frequency): optional live tuning in Hertz.  Connected it
          OVERRIDES the static tune parameter entirely (same contract as the
          filter's cutoff_mod_in), so fadsr glides or key tracking (note ->
          fscale) patch onto it.
        - q_mod_in (signal)   : optional multiplier on Q.  The stereo mean is
          clamped to [-1, +1], the effective Q becomes q * (1 + m), 0..1.
          Unconnected: the static q runs as-is.
        - vel      (velocity) : accent on the ring length.  Connected the
          stereo mean (0-1) scales the effective decay time:
              t_eff = t * (1 + v * (spread - 1))
          with the `spread` parameter (0.25..4, plan P1 pull-to-neutral
          language).  Default spread 1 keeps every value neutral, and the
          unwired port (defaults 1.0) is byteidentical.  Decay is
          intentionally needle-less: velocity is its only live mover (there
          is no decay mod-in), and the needle contract covers slider ×
          mod-port only — see doc/processor-template.md.
        - out      (signal)   : the resonance response — the band-limited
          excitation while it lasts (resonance tint, one signal path) plus
          the ring after it stops.
        - strike_vel (velocity): raw accent line for the internal strike
          (below).  Unwired it defaults to 1.0 — same neutral default as
          the vel port, and the strike knobs alone decide the ignition.

    Internal body strike (plan Phase 2, doc/research/
    "resonator-simplification idea.md" §9.2): the body can ignite itself.
    At note-on the two strike knobs are captured — `strikeLevel` (peak
    factor 0..2) and `strikeTime` (window length 0..0.2 s), the
    AdsrProcessor precedent of reading parameters in startNote().  While the
    window runs, the strike adds a sine at the body's per-sample f0 — the
    phase advances with the current f0, so a tune glide transfers
    coherently — under a linear window falling from full level to silence.
    The peak follows the RAW accent on strike_vel, read per sample: no sqrt
    in here, amplitude ∝ vel directly, injected energy ∝ vel² (plan Phase 3
    in the same move; the patch-side accent chain keeps serving its own
    consumers and must not double-curve).  The contribution enters Stage A's
    input (x_eff = in + strike) and never the integrator states, so the
    retrigger contract below holds untouched.  Knob defaults 0 / 0 s leave
    the strike branch dead code: byte-identical output, the same neutrality
    contract as the vel port.

    Structure (both stages are the TPT state-variable filter of
    FilterProcessor, bandpass output, per-sample coefficients):

        Stage A  coupling stage — tune, damping kA = 2 (1 - 0.99 q).
            Models how the exciter couples into the body: its bandwidth is
            the mode's selectivity (sharp click = broadband = good
            excitation, slow swell = poor) and it adds the tinting "snap".
        Stage B  ring stage — same tune, damping derived from decay:
            kB = ln(1000) / (pi * f0 * t60)     (t60 = decay seconds)
            i.e. the amplitude envelope A(t) = A0 * exp(-t / tau) with
            tau = t60 / ln(1000); high Q and long ring are the same number.
            Its bandpass output is peak-normalised (multiplied by kB) so a
            sustained tone at f0 passes at unity gain.

    Both behaviours of an 808 body come out of this one core: with a long
    decay the node is the ring source (claves, tom/conga bodies, BD), with a
    short decay it is the body filter (exciter passed with tint + short
    tail, snare 185/330 pair).  The difference is quantitative, not
    structural.

    Stereo: all coefficients are shared; only the TPT state (ic1/ic2 pairs)
    is per channel.  Per-sample processing is real-time safe: no
    allocations, no locks, only atomic reads.
*/

#pragma once

#include "ProcessorPort.h"

#include "../EffectiveRangeTelemetry.h"      // NeedleWindowTelemetry

#include <atomic>

namespace smolfm
{

/**
    Excited damped second-order resonator with tune, Q, decay,
    velocity-on-decay and an optional internal velocity-driven strike
    (default-neutral).
*/
class ResonatorProcessor final : public Processor
{
public:
    /**
        Create the resonator node.

        @param tuneParameter      atomic pointer to the static tune (Hz)
        @param qParameter         atomic pointer to the resonator Q (0-1)
        @param decayParameter     atomic pointer to the decay (t60, seconds)
        @param velSpreadParameter atomic pointer to the vel-to-decay spread
                                  (0.25-4; 1 = neutral)
        @param strikeLevelParameter atomic pointer to the internal strike
                                  peak factor (0-2; 0 = strike off)
        @param strikeTimeParameter  atomic pointer to the internal strike
                                  window (seconds, 0-0.2; 0 = strike off)
    */
    ResonatorProcessor (std::atomic<float>* tuneParameter,
                        std::atomic<float>* qParameter,
                        std::atomic<float>* decayParameter,
                        std::atomic<float>* velSpreadParameter,
                        std::atomic<float>* strikeLevelParameter,
                        std::atomic<float>* strikeTimeParameter);

    void prepare (double newSampleRate) override;

    /**
        Keep the per-note behaviour of a real body: a retrigger excites the
        still-ringing network again (energy adds up), so the tail is NOT
        cleared here.  Only reset() (new patch / new sample rate) zeroes it.
    */
    void startNote() override;

    StereoFrame processSample() override;

    /**
        Zero both TPT stages and the strike window state.

        applyConnectionPatch calls this so a new patch cannot inherit the
        previous patch's ring (mirrors the filter/delay resets).
    */
    void reset() noexcept;

    /**
        True while the ring still holds audible energy.

        Used by the voice lifetime policy: a voice keeps rendering after
        key release while the resonator tail rings out (same contract as
        the delay tail estimate) — a pure exciter -> resonator -> output
        patch therefore decays naturally instead of being chopped.
    */
    bool hasEnergy() const noexcept { return tailPeak > 1.0e-3f; }

    InputPort& getInput()         noexcept { return input; }
    InputPort& getTuneInput()     noexcept { return tuneIn; }
    InputPort& getQModInput()     noexcept { return qMod; }
    InputPort& getVelInput()      noexcept { return velIn; }
    InputPort& getStrikeVelInput() noexcept { return strikeVel; }
    OutputPort& getOutput()       noexcept { return output; }

    /** Effective (modulated) Q band currently rendered, for the UI's
        ghost needle: min/max of q * (1 + m) over the telemetry window.
        Display-only telemetry, thread-safe. */
    juce::Range<float> getEffectiveQRange() const noexcept
    {
        return uiEffectiveQ.read();
    }

private:
    std::atomic<float>* tune = nullptr;
    std::atomic<float>* q = nullptr;
    std::atomic<float>* decay = nullptr;
    std::atomic<float>* velSpread = nullptr;
    std::atomic<float>* strikeLevel = nullptr;
    std::atomic<float>* strikeTime = nullptr;

    InputPort input  { PortType::signal };
    InputPort tuneIn { PortType::frequency };
    InputPort qMod   { PortType::signal };
    InputPort velIn  { PortType::velocity };
    InputPort strikeVel { PortType::velocity };   // raw accent for the strike
    OutputPort output { PortType::signal, *this };

    double sampleRate = 44100.0;
    StereoFrame ic1A;   // stage A TPT integrator state 1 (left/right pair)
    StereoFrame ic2A;   // stage A TPT integrator state 2 (left/right pair)
    StereoFrame ic1B;   // stage B TPT integrator state 1 (left/right pair)
    StereoFrame ic2B;   // stage B TPT integrator state 2 (left/right pair)

    // Internal strike window.  Knobs are captured in startNote() (the
    // AdsrProcessor precedent); the accent value is read per sample because
    // at the exact startNote() call the vel port still holds the previous
    // tick's frame — a start-time capture would lag one hit behind.  The
    // line is stable for the whole hit, so both reads agree with "captured
    // at the hit".
    bool strikeActive = false;
    float strikeWindowSamples = 0.0f;   // window length in samples (> 0)
    float strikeElapsed = 0.0f;         // ticks since the strike fired
    double strikePhase = 0.0;           // strike sine phase, follows f0
    float strikePeak = 0.0f;            // captured peak factor

    // O(1) tail-energy estimate: a peak follower over the samples written
    // to the output.  Decays slowly so it never under-reports a ringing
    // tail by much; zero when the stage is cleared.
    float tailPeak = 0.0f;

    // Telemetry for the UI's mod indicator (ghost needle): min/max of the
    // effective Q after q_mod_in's (1+m) scaling over the needle window —
    // a per-sample overwrite would alias against the overlay's 30 Hz poll
    // (see NeedleWindowTelemetry).  Decay stays needle-less: velocity is
    // its only live mover and never feeds telemetry.
    NeedleWindowTelemetry uiEffectiveQ;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorProcessor)
};

} // namespace smolfm
