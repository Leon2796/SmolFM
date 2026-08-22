/*
    PluginMidiTests verifies that AudioPluginAudioProcessor receives MIDI
    notes and produces non-silent audio output.

    It creates the plugin processor, prepares it, injects a MIDI note-on event
    into processBlock, and checks that the output buffer is not silent.
*/

#include <atomic>
#include <cmath>
#include <iostream>

#include "PluginProcessor.h"

static bool runMidiReceiveTest()
{
    AudioPluginAudioProcessor processor;

    const double sampleRate = 48000.0;
    const int samplesPerBlock = 512;

    processor.prepareToPlay (sampleRate, samplesPerBlock);

    juce::AudioBuffer<float> buffer (2, samplesPerBlock);
    buffer.clear();

    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 69, static_cast<juce::uint8> (100)), 0);

    processor.processBlock (buffer, midi);

    float peak = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const float* data = buffer.getReadPointer (channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            peak = std::max (peak, std::abs (data[i]));
    }

    if (peak < 0.001f)
    {
        std::cout << "[FAIL] Plugin produced silence after MIDI note-on (peak = "
                  << peak << ")\n";
        return false;
    }

    std::cout << "[PASS] Plugin received MIDI note and produced samples (peak = "
              << peak << ")\n";
    return true;
}

int main()
{
    return runMidiReceiveTest() ? 0 : 1;
}
