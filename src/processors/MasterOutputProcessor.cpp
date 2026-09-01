/*
    MasterOutputProcessor implementation.
*/

#include "MasterOutputProcessor.h"

namespace smolfm
{

MasterOutputProcessor::MasterOutputProcessor (std::atomic<float>* levelParameter)
    : Processor (ProcessorRole::masterOutput),
      level (levelParameter),
      inputs { InputPort (PortType::signal), InputPort (PortType::signal),
               InputPort (PortType::signal), InputPort (PortType::signal),
               InputPort (PortType::signal), InputPort (PortType::signal),
               InputPort (PortType::signal), InputPort (PortType::signal) }
{
}

void MasterOutputProcessor::prepare (double newSampleRate)
{
    juce::ignoreUnused (newSampleRate);
}

void MasterOutputProcessor::startNote()
{
    smoothedPeak = 0.0f;
    peakLevel.store (0.0f, std::memory_order_release);
}

float MasterOutputProcessor::processSample()
{
    float sum = 0.0f;
    for (auto& in : inputs)
        sum += in.getSample();

    const float out = sum * level->load();

    // Peak follower: instant attack, ~50 ms release at typical block rates.
    const float magnitude = std::abs (out);
    smoothedPeak = magnitude > smoothedPeak ? magnitude : smoothedPeak * 0.9995f;
    peakLevel.store (smoothedPeak, std::memory_order_release);

    return out;
}

} // namespace smolfm
