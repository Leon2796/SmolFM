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
#include "FrequencyScaleProcessor.h"
#include "OscillatorProcessor.h"
#include "FMModulationProcessor.h"
#include "AdsrProcessor.h"
#include "SplitterProcessor.h"
#include "MergerProcessor.h"
#include "SignalGraph.h"

namespace
{
    struct TestParameters
    {
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
            &params.carrierWaveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* carrierPtr = carrier.get();

        auto modulator = std::make_unique<smolfm::OscillatorProcessor> (
            &params.modulatorWaveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* modulatorPtr = modulator.get();

        auto fm = std::make_unique<smolfm::FMModulationProcessor> (&params.fmAmount);
        smolfm::FMModulationProcessor* fmPtr = fm.get();

        auto adsr = std::make_unique<smolfm::AdsrProcessor> (
            &params.attack, &params.decay, &params.sustain, &params.release);
        smolfm::AdsrProcessor* adsrPtr = adsr.get();

        // Chain: note -> fm (Hertz bend) -> carrier.note_in -> carrier -> adsr.
        if (! fmPtr->getFreqInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to FM freq_in" };

        if (! fmPtr->getModulatorInput().connect (modulatorPtr->getOutput()))
            return { false, "Failed to connect modulator to FM modulator_in" };

        if (! carrierPtr->getNoteInput().connect (fmPtr->getOutput()))
            return { false, "Failed to connect FM to carrier note_in" };

        if (! adsrPtr->getInput().connect (carrierPtr->getOutput()))
            return { false, "Failed to connect carrier to ADSR" };

        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (modulator));
        graph.addProcessor (std::move (fm));
        graph.addProcessor (std::move (carrier));
        graph.addProcessor (std::move (adsr));

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        graph.startNote();

        // Active FM: the 220 Hz modulator bends the carrier frequency.
        params.fmAmount.store (1.0f);

        float peak = 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            const smolfm::StereoFrame frame = graph.processSample();
            const float absSample = std::max (std::abs (frame.l), std::abs (frame.r));
            peak = std::max (peak, absSample);
            sum += absSample;
        }

        if (peak < 0.001f)
            return { false, "Graph produced silence (peak too low)" };

        if (! adsrPtr->isActive())
            return { false, "ADSR became inactive during a held note" };

        return { true, "Note-driven graph produced samples (peak = " + std::to_string (peak) + ", sum = " + std::to_string (sum) + ")" };
    }

    // Without any note wiring the carrier's note_in stays unconnected and the
    // oscillator must produce silence â€” there is no frequency fallback.
    TestResult runHzOnlyGraphTest()
    {
        TestParameters params;
        smolfm::SignalGraph graph;

        auto carrier = std::make_unique<smolfm::OscillatorProcessor> (
            &params.carrierWaveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* carrierPtr = carrier.get();

        auto modulator = std::make_unique<smolfm::OscillatorProcessor> (
            &params.modulatorWaveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* modulatorPtr = modulator.get();

        auto fm = std::make_unique<smolfm::FMModulationProcessor> (&params.fmAmount);
        smolfm::FMModulationProcessor* fmPtr = fm.get();

        auto adsr = std::make_unique<smolfm::AdsrProcessor> (
            &params.attack, &params.decay, &params.sustain, &params.release);
        smolfm::AdsrProcessor* adsrPtr = adsr.get();

        if (! fmPtr->getModulatorInput().connect (modulatorPtr->getOutput()))
            return { false, "Failed to connect modulator to FM modulator_in" };

        if (! carrierPtr->getNoteInput().connect (fmPtr->getOutput()))
            return { false, "Failed to connect FM to carrier note_in" };

        if (! adsrPtr->getInput().connect (carrierPtr->getOutput()))
            return { false, "Failed to connect carrier to ADSR" };

        graph.addProcessor (std::move (modulator));
        graph.addProcessor (std::move (fm));
        graph.addProcessor (std::move (carrier));
        graph.addProcessor (std::move (adsr));

        graph.prepare (48000.0);
        graph.startNote();

        float peak = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            const smolfm::StereoFrame frame = graph.processSample();
            peak = std::max (peak, std::max (std::abs (frame.l), std::abs (frame.r)));
        }

        if (peak > 0.0001f)
            return { false, "Unconnected note_in should produce silence (peak = " + std::to_string (peak) + ")" };

        return { true, "Unconnected note_in produced silence as expected" };
    }

    // Two FM stages chained in the Hertz domain:
    //   note -> fmA.freq_in, fmA.out -> fmB.freq_in, fmB.out -> carrier.note_in
    // Verifies (1) the chain is transparent while both amounts are 0 and
    // (2) an active modulator bends the instantaneous frequency itself.
    TestResult runChainedFmTest()
    {
        std::atomic<float> carrierWaveform { 0.0f };
        std::atomic<float> modWaveform { 0.0f };
        std::atomic<float> amountA { 0.0f };
        std::atomic<float> amountB { 0.0f };
        std::atomic<float> attack { 0.01f };
        std::atomic<float> decay { 0.2f };
        std::atomic<float> sustain { 0.8f };
        std::atomic<float> release { 0.5f };

        smolfm::SignalGraph graph;

        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();

        auto carrier = std::make_unique<smolfm::OscillatorProcessor> (
            &carrierWaveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* carrierPtr = carrier.get();

        auto modulator = std::make_unique<smolfm::OscillatorProcessor> (&modWaveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* modulatorPtr = modulator.get();

        auto fmA = std::make_unique<smolfm::FMModulationProcessor> (&amountA);
        smolfm::FMModulationProcessor* fmAPtr = fmA.get();

        auto fmB = std::make_unique<smolfm::FMModulationProcessor> (&amountB);
        smolfm::FMModulationProcessor* fmBPtr = fmB.get();

        auto adsr = std::make_unique<smolfm::AdsrProcessor> (&attack, &decay, &sustain, &release);
        smolfm::AdsrProcessor* adsrPtr = adsr.get();

        // The modulator needs a frequency source too â€” without a note trace
        // the oscillator is silent (0 Hz) since the fallback was removed.
        if (! modulatorPtr->getNoteInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to modulator note_in" };

        if (! fmAPtr->getFreqInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to fmA freq_in" };

        if (! fmAPtr->getModulatorInput().connect (modulatorPtr->getOutput()))
            return { false, "Failed to connect modulator to fmA" };

        if (! fmBPtr->getFreqInput().connect (fmAPtr->getOutput()))
            return { false, "Failed to chain fmA into fmB" };

        if (! carrierPtr->getNoteInput().connect (fmBPtr->getOutput()))
            return { false, "Failed to connect fmB to carrier note_in" };

        if (! adsrPtr->getInput().connect (carrierPtr->getOutput()))
            return { false, "Failed to connect carrier to ADSR" };

        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (modulator));
        graph.addProcessor (std::move (fmA));
        graph.addProcessor (std::move (fmB));
        graph.addProcessor (std::move (carrier));
        graph.addProcessor (std::move (adsr));

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        graph.startNote();

        // (1) Both amounts at 0: 440 Hz must pass through both stages unchanged.
        graph.processSample();
        graph.processSample();
        const float passthroughHz = fmBPtr->getOutput().getSample().l;
        if (std::abs (passthroughHz - 440.0f) > 0.001f)
            return { false, "Idle FM chain is not transparent (got " + std::to_string (passthroughHz) + " Hz)" };

        // (2) amountA = 1: the 2 Hz modulator must swing the Hertz value that
        // flows through fmB, i.e. true frequency modulation, not a phase hack.
        amountA.store (1.0f);

        float minHz = 440.0f;
        float maxHz = 440.0f;
        for (int i = 0; i < 48000; ++i)
        {
            graph.processSample();
            const float hz = fmBPtr->getOutput().getSample().l;
            minHz = std::min (minHz, hz);
            maxHz = std::max (maxHz, hz);
        }

        if (minHz > 400.0f || maxHz < 480.0f)
            return { false, "Chained FM did not deviate (min = " + std::to_string (minHz)
                          + " Hz, max = " + std::to_string (maxHz) + " Hz)" };

        return { true, "Chained FM: transparent when idle, deviation "
                     + std::to_string (minHz) + ".." + std::to_string (maxHz) + " Hz when active" };
    }

    // Frequency scale: the interval dropdown multiplies with the factor knob.
    // Scaler A carries an interval (octave up = choice index 1), scaler B is
    // built the legacy way (no interval pointer, fully transparent).
    TestResult runFrequencyScaleTest()
    {
        std::atomic<float> factorA { 1.0f };
        std::atomic<float> intervalA { 1.0f };   // octave up
        std::atomic<float> factorB { 1.0f };

        smolfm::SignalGraph graph;

        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();

        auto scalerA = std::make_unique<smolfm::FrequencyScaleProcessor> (&factorA, &intervalA);
        smolfm::FrequencyScaleProcessor* aPtr = scalerA.get();

        auto scalerB = std::make_unique<smolfm::FrequencyScaleProcessor> (&factorB);
        smolfm::FrequencyScaleProcessor* bPtr = scalerB.get();

        if (! aPtr->getFreqInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to scaler A" };

        if (! bPtr->getFreqInput().connect (aPtr->getOutput()))
            return { false, "Failed to chain scaler A into scaler B" };

        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (scalerA));
        graph.addProcessor (std::move (scalerB));

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        graph.startNote();

        // (1) 440 Hz * octave up (2.0) = 880 Hz; legacy scaler B is transparent.
        graph.processSample();
        if (std::abs (aPtr->getOutput().getSample().l - 880.0f) > 0.01f)
            return { false, "Octave-up interval wrong (got " + std::to_string (aPtr->getOutput().getSample().l) + " Hz instead of 880)" };

        if (std::abs (bPtr->getOutput().getSample().l - 880.0f) > 0.01f)
            return { false, "Legacy scaler without interval pointer is not transparent (got " + std::to_string (bPtr->getOutput().getSample().l) + " Hz)" };

        // (2) Dropdown and knob multiply: 880 * 0.25 = 220 Hz downstream.
        factorB.store (0.25f);
        graph.processSample();
        if (std::abs (bPtr->getOutput().getSample().l - 220.0f) > 0.01f)
            return { false, "Interval * factor combination wrong (got " + std::to_string (bPtr->getOutput().getSample().l) + " Hz instead of 220)" };

        // (3) Interval none (index 0) is transparent again.
        factorA.store (1.0f);
        intervalA.store (0.0f);
        graph.processSample();
        if (std::abs (aPtr->getOutput().getSample().l - 440.0f) > 0.01f)
            return { false, "None interval is not transparent (got " + std::to_string (aPtr->getOutput().getSample().l) + " Hz)" };

        // (4) An out-of-range raw value falls back to transparent, not to a
        // wild ratio.
        intervalA.store (99.0f);
        graph.processSample();
        if (std::abs (aPtr->getOutput().getSample().l - 440.0f) > 0.01f)
            return { false, "Out-of-range interval did not fall back to transparent" };

        return { true, "Octave up -> 880 Hz; chained with factor 0.25 -> 220 Hz; none and out-of-range transparent" };
    }

    TestResult runStereoSplitMergeTest()
    {
        smolfm::SignalGraph graph;

        std::atomic<float> waveform { 0.0f };   // sine

        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();

        auto osc = std::make_unique<smolfm::OscillatorProcessor> (
            &waveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* oscPtr = osc.get();

        auto splitter = std::make_unique<smolfm::SplitterProcessor>();
        smolfm::SplitterProcessor* splitPtr = splitter.get();

        auto merger = std::make_unique<smolfm::MergerProcessor>();
        smolfm::MergerProcessor* mergePtr = merger.get();

        // Chain: note -> osc -> splitter -> (outL, outR) -> merger.
        if (! oscPtr->getNoteInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to osc note_in" };

        if (! splitPtr->getInput().connect (oscPtr->getOutput()))
            return { false, "Failed to connect osc to splitter in" };

        if (! mergePtr->getLeftInput().connect (splitPtr->getLeftOutput()))
            return { false, "Failed to connect splitter outL to merger in1" };

        if (! mergePtr->getRightInput().connect (splitPtr->getRightOutput()))
            return { false, "Failed to connect splitter outR to merger in2" };

        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (osc));
        graph.addProcessor (std::move (splitter));
        graph.addProcessor (std::move (merger));

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        graph.startNote();

        for (int i = 0; i < 1000; ++i)
        {
            graph.processSample();

            const smolfm::StereoFrame oscFrame = oscPtr->getOutput().getSample();
            const smolfm::StereoFrame left  = splitPtr->getLeftOutput().getSample();
            const smolfm::StereoFrame right = splitPtr->getRightOutput().getSample();
            const smolfm::StereoFrame merged = mergePtr->getOutput().getSample();

            // Splitter: each branch carries exactly one side, the other is 0.
            if (left.l != oscFrame.l || left.r != 0.0f)
                return { false, "outL must be (osc.l, 0) — got ("
                              + std::to_string (left.l) + ", " + std::to_string (left.r) + ")" };

            if (right.r != oscFrame.r || right.l != 0.0f)
                return { false, "outR must be (0, osc.r) — got ("
                              + std::to_string (right.l) + ", " + std::to_string (right.r) + ")" };

            // Merger: exact recombination of the two side branches.
            if (merged.l != oscFrame.l || merged.r != oscFrame.r)
                return { false, "merger must reconstruct (osc.l, osc.r) — got ("
                              + std::to_string (merged.l) + ", " + std::to_string (merged.r) + ")" };
        }

        // Sanity: the source actually produced audio (test is not vacuous).
        float peak = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            graph.processSample();
            peak = std::max (peak, std::abs (oscPtr->getOutput().getSample().l));
        }

        if (peak < 0.001f)
            return { false, "Source was silent; split/merge checks are vacuous" };

        // Side hints: display-only metadata, set by the processors.
        if (splitPtr->getLeftOutput().getChannelSide()  != smolfm::ChannelSide::left
         || splitPtr->getRightOutput().getChannelSide() != smolfm::ChannelSide::right)
            return { false, "Splitter ports must hint left/right" };

        if (mergePtr->getLeftInput().getChannelSide()  != smolfm::ChannelSide::left
         || mergePtr->getRightInput().getChannelSide() != smolfm::ChannelSide::right)
            return { false, "Merger ports must hint left/right" };

        return { true, "Splitter emits pure sides, merger recombines exactly (peak "
                     + std::to_string (peak) + ")" };
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
    runTest ("Chained FM stages", runChainedFmTest);
    runTest ("Frequency scale interval", runFrequencyScaleTest);
    runTest ("Stereo split & merge", runStereoSplitMergeTest);

    return failures;
}
