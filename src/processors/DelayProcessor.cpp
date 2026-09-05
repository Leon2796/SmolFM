/*
    DelayProcessor implementation.
*/

#include "DelayProcessor.h"

#include <algorithm>
#include <cmath>

namespace smolfm
{

DelayProcessor::DelayProcessor()
    : Processor (ProcessorRole::generic)
{
    // No input wired → silence; feedback and mix default to safe values via APVTS.
    input.setDefaultValue (0.0f);
}

DelayProcessor::DelayProcessor (std::atomic<float>* timeMsP,
                                std::atomic<float>* feedbackP,
                                std::atomic<float>* mixP,
                                std::atomic<float>* syncP,
                                std::atomic<float>* divP)
    : DelayProcessor()
{
    timeMs    = timeMsP;
    feedback  = feedbackP;
    mix       = mixP;
    syncMode  = syncP;
    division  = divP;
}

void DelayProcessor::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    const int maxSamples = static_cast<int> (maxDelaySeconds * sampleRate) + 2;
    if (static_cast<int> (buffer.size()) != maxSamples)
        buffer.assign (static_cast<size_t> (maxSamples), 0.0f);

    writePos = 0;
}

void DelayProcessor::startNote()
{
    // Keep the tail: if the buffer still holds an old note, leave it alone.
    // Delay persists across notes (standard effect behaviour).
}

void DelayProcessor::reset() noexcept
{
    // Only called when a new patch is loaded — never mid-note.  Notes are
    // still allowed to reuse an existing tail (see startNote).
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
    tailPeak = 0.0f;
}

float DelayProcessor::getDelaySamples() const noexcept
{
    const float sync       = syncMode   != nullptr ? syncMode->load()   : 0.0f;
    const float timeParam  = timeMs     != nullptr ? timeMs->load()     : 250.0f;
    const float divIndex   = division   != nullptr ? division->load()   : 1.0f;  // 1/4 default

    if (sync < 0.5f)
    {
        // Free-running in milliseconds.
        return timeParam * 0.001f * static_cast<float> (sampleRate);
    }

    // Sync mode: quarter-note duration in ms, scaled by the division factor.
    const float quarterMs = 60000.0f / static_cast<float> (currentBpm);
    const float factor = (divIndex < 0.5f) ? 2.0f    // 1/2 note
                       : (divIndex < 1.5f) ? 1.0f    // 1/4
                       : (divIndex < 2.5f) ? 0.5f    // 1/8
                                           : 0.25f;  // 1/16
    return quarterMs * factor * 0.001f * static_cast<float> (sampleRate);
}

float DelayProcessor::processSample()
{
    if (buffer.empty())
        return input.getSample();

    const int maxSamples = static_cast<int> (buffer.size());
    const float ds = getDelaySamples();
    const int delayInt = juce::jlimit (1, maxSamples - 2, static_cast<int> (ds));

    // Read the delayed sample with a fractional-step-free lookup: simply read
    // the integer index behind writePos.  No interpolation — delay in this
    // range is musically fine without it.
    const int readPos = (writePos - delayInt + maxSamples) % maxSamples;
    const float wet = buffer[static_cast<size_t> (readPos)];

    const float fb = feedback != nullptr ? juce::jlimit (0.0f, 0.95f, feedback->load()) : 0.3f;
    const float m  = mix      != nullptr ? juce::jlimit (0.0f, 1.0f,  mix->load())      : 0.3f;

    const float dry = input.getSample();
    const float written = dry + wet * fb;
    buffer[static_cast<size_t> (writePos)] = written;
    writePos = (writePos + 1) % maxSamples;

    // Track tail energy (O(1)): the written value is exactly what remains in
    // the line.  Decay is slow enough to follow even long delay repeats.
    tailPeak = juce::jmax (tailPeak * 0.99998f, std::abs (written));

    const float result = dry * (1.0f - m) + wet * m;
    output.setSample (result);
    return result;
}

} // namespace smolfm
