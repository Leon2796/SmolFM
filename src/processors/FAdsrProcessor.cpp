/*
    FAdsrProcessor implementation.
*/

#include "FAdsrProcessor.h"

namespace smolfm
{

FAdsrProcessor::FAdsrProcessor (std::atomic<float>* attackParam,
                                std::atomic<float>* decayParam,
                                std::atomic<float>* sustainParam,
                                std::atomic<float>* releaseParam,
                                std::atomic<float>* upParam,
                                std::atomic<float>* downParam)
    : Processor (ProcessorRole::adsr),
      attack (attackParam),
      decay (decayParam),
      sustain (sustainParam),
      release (releaseParam),
      up (upParam),
      down (downParam),
      freqInput (PortType::frequency),
      output (PortType::frequency, *this)
{
    // Unwired vel input is neutral (v = 1.0): no anchor pull is applied and
    // the sound stays exactly as before — the same rule as the other vel
    // pins (gain, shape, fm, filter).
    velIn.setDefaultValue (1.0f);
}

void FAdsrProcessor::prepare (double newSampleRate)
{
    if (newSampleRate > 0.0)
        sampleRate = newSampleRate;
}

void FAdsrProcessor::startNote()
{
    if (sampleRate <= 0.0)
        sampleRate = 44100.0;

    envelope.setSampleRate (sampleRate);

    juce::ADSR::Parameters params;
    params.attack  = attack->load();
    params.decay   = decay->load();
    params.sustain = sustain->load();
    params.release = release->load();

    envelope.reset();
    envelope.setParameters (params);
    envelope.noteOn();
}

StereoFrame FAdsrProcessor::processSample()
{
    const float sourceFrequency = freqInput.getSample().l;
    const float env = envelope.getNextSample();

    // Velocity (accent depth): the connected vel input's stereo mean (0-1)
    // pulls both anchors from neutral 1.0 toward their configured values —
    // soft notes play the pitch gesture flatter, hard notes with full depth.
    // Depth only: A/D/S/R times and the anchor directions stay untouched.
    // Unwired: fully sound-invariant (byte-identical to the old behaviour).
    float upEffective   = up->load();
    float downEffective = down->load();
    if (velIn.isConnected())
    {
        const StereoFrame vp = velIn.getSample();
        const float v = 0.5f * (vp.l + vp.r);

        upEffective   = 1.0f + v * (upEffective   - 1.0f);
        downEffective = 1.0f + v * (downEffective - 1.0f);
    }

    // Interpolate the scaling factor between the effective down factor
    // (E = 0) and the effective up factor (E = 1).  Neutral when both
    // equal 1.0.
    const float scaled = sourceFrequency
                       * (downEffective + (upEffective - downEffective) * env);

    const StereoFrame result = StereoFrame::centered (scaled);
    output.setSample (result);
    return result;
}

void FAdsrProcessor::noteOff()
{
    envelope.noteOff();
}

void FAdsrProcessor::reset() noexcept
{
    envelope.reset();
}

bool FAdsrProcessor::isActive() const noexcept
{
    return envelope.isActive();
}

} // namespace smolfm