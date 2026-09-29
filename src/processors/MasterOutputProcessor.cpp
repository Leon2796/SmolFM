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

StereoFrame MasterOutputProcessor::processSample()
{
    StereoFrame sum;

    for (auto& in : inputs)
    {
        const StereoFrame inFrame = in.getSample();
        sum.l += inFrame.l;
        sum.r += inFrame.r;
    }

    const float gainValue = level->load();
    const StereoFrame out { sum.l * gainValue, sum.r * gainValue };

    // Peak follower: instant attack, ~50 ms release at typical block rates.
    // The louder channel drives the UI meter.
    const float magnitude = juce::jmax (std::abs (out.l), std::abs (out.r));
    smoothedPeak = magnitude > smoothedPeak ? magnitude : smoothedPeak * 0.9995f;
    peakLevel.store (smoothedPeak, std::memory_order_release);

    return out;
}

} // namespace smolfm
