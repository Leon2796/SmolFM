/*
    AdsrProcessor implementation.
*/

#include "AdsrProcessor.h"

namespace smolfm
{

AdsrProcessor::AdsrProcessor (std::atomic<float>* attackParam,
                              std::atomic<float>* decayParam,
                              std::atomic<float>* sustainParam,
                              std::atomic<float>* releaseParam)
    : Processor (ProcessorRole::adsr),
      attack (attackParam),
      decay (decayParam),
      sustain (sustainParam),
      release (releaseParam),
      input (PortType::signal),
      output (PortType::signal, *this)
{
}

void AdsrProcessor::prepare (double newSampleRate)
{
    if (newSampleRate > 0.0)
    {
        sampleRate = newSampleRate;
        envelope.setSampleRate (sampleRate);
    }
}

void AdsrProcessor::startNote()
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

float AdsrProcessor::processSample()
{
    float sourceSample = input.getSample();
    float env = envelope.getNextSample();

    float sample = sourceSample * env;
    output.setSample (sample);

    return sample;
}

void AdsrProcessor::noteOff()
{
    envelope.noteOff();
}

bool AdsrProcessor::isActive() const noexcept
{
    return envelope.isActive();
}

} // namespace smolfm
