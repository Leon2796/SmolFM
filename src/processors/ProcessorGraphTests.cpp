/*
    ProcessorGraphTests contains a small JUCE-free test harness for the
    processor graph.  It builds the same signal chain as SynthVoice, drives it
    with a MIDI note, and verifies that non-zero samples come out of the graph.

    The test is compiled into its own console executable so that it can be run
    outside of a plugin host.
*/

#include <atomic>
#include <cmath>
#include <functional>
#include <iostream>
#include <string>

#include "NoteProcessor.h"
#include "OscillatorProcessor.h"
#include "FMModulationProcessor.h"
#include "AdsrProcessor.h"
#include "SignalGraph.h"
#include "../SynthVoice.h"
#include "../SynthSound.h"

namespace
{
    struct TestParameters
    {
        std::atomic<float> carrierFrequency { 440.0f };
        std::atomic<float> modulatorFrequency { 220.0f };
        std::atomic<float> fmAmount { 0.0f };
        std::atomic<float> carrierWaveform { 0.0f };
        std::atomic<float> modulatorWaveform { 0.0f };
        std::atomic<float> attack { 0.01f };
        std::atomic<float> decay { 0.2f };
        std::atomic<float> sustain { 0.8f };
        std::atomic<float> release { 0.5f };
    };

    struct TestResult
    {
        bool passed = false;
        std::string message;
    };

    TestResult runNoteDrivenGraphTest()
    {
        TestParameters params;
        smolfm::SignalGraph graph;

        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();

        auto carrier = std::make_unique<smolfm::OscillatorProcessor> (
            &params.carrierFrequency, &params.carrierWaveform);
        smolfm::OscillatorProcessor* carrierPtr = carrier.get();

        auto modulator = std::make_unique<smolfm::OscillatorProcessor> (
            &params.modulatorFrequency, &params.modulatorWaveform);
        smolfm::OscillatorProcessor* modulatorPtr = modulator.get();

        auto fm = std::make_unique<smolfm::FMModulationProcessor> (&params.fmAmount);
        smolfm::FMModulationProcessor* fmPtr = fm.get();

        auto adsr = std::make_unique<smolfm::AdsrProcessor> (
            &params.attack, &params.decay, &params.sustain, &params.release);
        smolfm::AdsrProcessor* adsrPtr = adsr.get();

        if (! carrierPtr->getNoteInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note input to carrier" };

        if (! fmPtr->getCarrierInput().connect (carrierPtr->getOutput()))
            return { false, "Failed to connect carrier to FM carrier_in" };

        if (! fmPtr->getModulatorInput().connect (modulatorPtr->getOutput()))
            return { false, "Failed to connect modulator to FM modulator_in" };

        if (! adsrPtr->getInput().connect (fmPtr->getOutput()))
            return { false, "Failed to connect FM to ADSR" };

        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (carrier));
        graph.addProcessor (std::move (modulator));
        graph.addProcessor (std::move (fm));
        graph.addProcessor (std::move (adsr));

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        graph.startNote();

        float peak = 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            float sample = graph.processSample();
            float absSample = std::abs (sample);
            peak = std::max (peak, absSample);
            sum += absSample;
        }

        if (peak < 0.001f)
            return { false, "Graph produced silence (peak too low)" };

        if (! adsrPtr->isActive())
            return { false, "ADSR became inactive during a held note" };

        return { true, "Note-driven graph produced samples (peak = " + std::to_string (peak) + ", sum = " + std::to_string (sum) + ")" };
    }

    TestResult runHzOnlyGraphTest()
    {
        TestParameters params;
        smolfm::SignalGraph graph;

        // No NoteProcessor: both oscillators must fall back to their Hz sliders.
        auto carrier = std::make_unique<smolfm::OscillatorProcessor> (
            &params.carrierFrequency, &params.carrierWaveform);
        smolfm::OscillatorProcessor* carrierPtr = carrier.get();

        auto modulator = std::make_unique<smolfm::OscillatorProcessor> (
            &params.modulatorFrequency, &params.modulatorWaveform);
        smolfm::OscillatorProcessor* modulatorPtr = modulator.get();

        auto fm = std::make_unique<smolfm::FMModulationProcessor> (&params.fmAmount);
        smolfm::FMModulationProcessor* fmPtr = fm.get();

        auto adsr = std::make_unique<smolfm::AdsrProcessor> (
            &params.attack, &params.decay, &params.sustain, &params.release);
        smolfm::AdsrProcessor* adsrPtr = adsr.get();

        if (! fmPtr->getCarrierInput().connect (carrierPtr->getOutput()))
            return { false, "Failed to connect carrier to FM carrier_in" };

        if (! fmPtr->getModulatorInput().connect (modulatorPtr->getOutput()))
            return { false, "Failed to connect modulator to FM modulator_in" };

        if (! adsrPtr->getInput().connect (fmPtr->getOutput()))
            return { false, "Failed to connect FM to ADSR" };

        graph.addProcessor (std::move (carrier));
        graph.addProcessor (std::move (modulator));
        graph.addProcessor (std::move (fm));
        graph.addProcessor (std::move (adsr));

        graph.prepare (48000.0);
        graph.startNote();

        float peak = 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            float sample = graph.processSample();
            float absSample = std::abs (sample);
            peak = std::max (peak, absSample);
            sum += absSample;
        }

        if (peak < 0.001f)
            return { false, "Hz-only graph produced silence (peak too low)" };

        return { true, "Hz-only graph produced samples (peak = " + std::to_string (peak) + ", sum = " + std::to_string (sum) + ")" };
    }
}

int main()
{
    int failures = 0;

    auto runTest = [&failures] (const std::string& name, std::function<TestResult()> test)
    {
        TestResult result = test();
        std::cout << "[" << (result.passed ? "PASS" : "FAIL") << "] " << name << ": " << result.message << "\n";
        if (! result.passed)
            ++failures;
    };

    runTest ("Note-driven graph", runNoteDrivenGraphTest);
    runTest ("Hz-only graph", runHzOnlyGraphTest);

    return failures;
}
