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

float FAdsrProcessor::processSample()
{
    const float sourceFrequency = freqInput.getSample();
    const float env = envelope.getNextSample();

    // Interpolate the scaling factor between the down factor (E = 0) and
    // the up factor (E = 1).  Neutral when both equal 1.0.
    const float upValue   = up->load();
    const float downValue = down->load();
    const float scaled = sourceFrequency * (downValue + (upValue - downValue) * env);

    output.setSample (scaled);
    return scaled;
}

void FAdsrProcessor::noteOff()
{
    envelope.noteOff();
}

bool FAdsrProcessor::isActive() const noexcept
{
    return envelope.isActive();
}

} // namespace smolfm