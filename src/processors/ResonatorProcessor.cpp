/*
    ResonatorProcessor implementation.

    Both stages reuse the FilterProcessor coefficient algebra (TPT SVF,
    trapezoidal integration) with mode == bandpass; coefficients are
    re-derived every sample so tune sweeps (fadsr glide, FM chains) are
    zipper-free.
*/

#include "ResonatorProcessor.h"

#include <algorithm>
#include <cmath>

namespace smolfm
{

namespace
{
    // One TPT-SVF band pass step (FilterProcessor's TPT with mode == BP).
    // The states travel through by reference exactly as in the filter.
    inline float tptBandpassStep (float channelInput, float a1, float a2, float a3,
                                  float& ic1, float& ic2) noexcept
    {
        const float v3 = channelInput - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;

        return v1;   // band pass output
    }
}

ResonatorProcessor::ResonatorProcessor (std::atomic<float>* tuneParameter,
                                        std::atomic<float>* qParameter,
                                        std::atomic<float>* decayParameter,
                                        std::atomic<float>* velSpreadParameter)
    : Processor (ProcessorRole::generic),
      tune (tuneParameter),
      q (qParameter),
      decay (decayParameter),
      velSpread (velSpreadParameter)
{
    input.setDefaultValue (0.0f);

    // Unwired tune falls back to the static tune parameter, so the
    // modulation port itself can default to any value (it is only read
    // when connected).
    tuneIn.setDefaultValue (440.0f);
    qMod.setDefaultValue (0.0f);

    // Unwired vel input is neutral: spread is multiplied by 1.0, the decay
    // stays exactly as configured (sound-invariance).
    velIn.setDefaultValue (1.0f);
}

void ResonatorProcessor::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void ResonatorProcessor::startNote()
{
    // Keep the tail: a real body retriggers while it still rings and the
    // new excitation adds onto the stored energy.  Only a new patch clears
    // the state (see reset()).
}

void ResonatorProcessor::reset() noexcept
{
    // Called when a new patch is loaded (or a rate change re-prepares the
    // voice) — never mid-note on purpose.  No stale ring survives an
    // instrument change.
    ic1A = StereoFrame {};
    ic2A = StereoFrame {};
    ic1B = StereoFrame {};
    ic2B = StereoFrame {};
    tailPeak = 0.0f;
}

StereoFrame ResonatorProcessor::processSample()
{
    // Tuning: the modulation input (frequency domain) replaces the static
    // parameter entirely when connected — the static tune is the fallback,
    // not an offset (mirrors the filter's cutoff_mod_in contract).
    float f0 = tuneIn.isConnected()
        ? tuneIn.getSample().l
        : (tune != nullptr ? tune->load() : 100.0f);

    f0 = juce::jlimit (20.0f, 8000.0f, f0);

    // Effective decay (t60): velocity on decay per the plan's pull-to-
    // neutral pattern t_eff = t * (1 + v * (spread - 1)).  Unwired: v = 1
    // and spread defaults to 1 -> byteidentical.
    float t60 = decay != nullptr ? decay->load() : 0.3f;
    if (velIn.isConnected())
    {
        const StereoFrame vp = velIn.getSample();
        const float v = juce::jlimit (0.0f, 1.0f, 0.5f * (vp.l + vp.r));
        const float spread = velSpread != nullptr ? velSpread->load() : 1.0f;
        t60 *= (1.0f + v * (spread - 1.0f));
    }

    t60 = juce::jlimit (0.005f, 10.0f, t60);

    // Q: static value plus optional multiplier mod (same contract as the
    // filter's resonance_mod_in).
    float resBase = q != nullptr ? q->load() : 0.5f;
    if (qMod.isConnected())
    {
        const StereoFrame pair = qMod.getSample();
        const float m = juce::jlimit (-1.0f, 1.0f, 0.5f * (pair.l + pair.r));
        resBase *= (1.0f + m);
    }
    const float res = juce::jlimit (0.0f, 1.0f, resBase);

    // Stage A (coupling): bandwidth from Q, same mapping as the filter.
    const float kA = 2.0f * (1.0f - 0.99f * res);

    // Stage B (ring): pole damping from the decay time.  The analog envelope
    // A(t) = A0 * exp(-t / tau) with tau = t60 / ln(1000) (t60 = decay to
    // -60 dB) gives k = 2 / (w0 * tau) = ln(1000) / (pi * f0 * t60); clamped
    // so extreme values stay numerically tame.
    const float kB = juce::jlimit (0.005f, 2.0f,
        6.9077553f / (juce::MathConstants<float>::pi * f0 * t60));

    // Shared TPT integrator coefficient; both stages tune to the same f0.
    const float g  = std::tan (juce::MathConstants<float>::pi
                               * juce::jlimit (0.0005f, 0.49f, f0 / (float) sampleRate));
    const float a1A = 1.0f / (1.0f + g * (g + kA));
    const float a2A = g * a1A;
    const float a3A = g * a2A;
    const float a1B = 1.0f / (1.0f + g * (g + kB));
    const float a2B = g * a1B;
    const float a3B = g * a2B;

    const StereoFrame x = input.getSample();

    // Stage A: the exciter couples into the mode (band-limited, tinted).
    const StereoFrame e
    {
        tptBandpassStep (x.l, a1A, a2A, a3A, ic1A.l, ic2A.l),
        tptBandpassStep (x.r, a1A, a2A, a3A, ic1A.r, ic2A.r)
    };

    // Stage B: the mode releases its stored energy as the decaying
    // near-sinusoid.  Peak-normalised (multiplied by kB) so a sustained
    // tone at f0 passes at unity gain.
    const StereoFrame y
    {
        kB * tptBandpassStep (e.l, a1B, a2B, a3B, ic1B.l, ic2B.l),
        kB * tptBandpassStep (e.r, a1B, a2B, a3B, ic1B.r, ic2B.r)
    };

    // Track tail energy (O(1), same policy as the delay's estimate): the
    // louder channel decides when the ring has died out.
    const float peak = std::max (std::abs (y.l), std::abs (y.r));
    tailPeak = juce::jmax (tailPeak * 0.99998f, peak);

    output.setSample (y);
    return y;
}

} // namespace smolfm