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
#include "GainProcessor.h"
#include "SplitterProcessor.h"
#include "MergerProcessor.h"
#include "AdderProcessor.h"
#include "WaveshaperProcessor.h"
#include "FilterProcessor.h"
#include "ResonatorProcessor.h"
#include "FeedbackProcessor.h"
#include "FAdsrProcessor.h"
#include "SignalGraph.h"
#include "../graph/GraphNodes.h"

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

    TestResult runAdderSumTest()
    {
        TestParameters params;
        smolfm::SignalGraph graph;

        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();

        auto osc = std::make_unique<smolfm::OscillatorProcessor> (
            &params.carrierWaveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* oscPtr = osc.get();

        auto adder = std::make_unique<smolfm::AdderProcessor>();
        smolfm::AdderProcessor* addPtr = adder.get();

        if (! oscPtr->getNoteInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to oscillator" };

        // One source wired to BOTH adder inputs: the adder must return
        // exactly twice the source sample (channel-wise sum).
        if (! addPtr->getInput1().connect (oscPtr->getOutput()))
            return { false, "Failed to connect oscillator to adder in1" };

        if (! addPtr->getInput2().connect (oscPtr->getOutput()))
            return { false, "Failed to connect oscillator to adder in2" };

        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (osc));
        graph.addProcessor (std::move (adder));

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        graph.startNote();

        for (int i = 0; i < 1000; ++i)
        {
            graph.processSample();

            const smolfm::StereoFrame oscFrame = oscPtr->getOutput().getSample();
            const smolfm::StereoFrame sum      = addPtr->getOutput().getSample();

            if (sum.l != 2.0f * oscFrame.l || sum.r != 2.0f * oscFrame.r)
                return { false, "adder must double a source wired to both inputs" };
        }

        // Sanity: the source actually produced audio (test is not vacuous).
        float peak = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            graph.processSample();
            peak = std::max (peak, std::abs (oscPtr->getOutput().getSample().l));
        }

        if (peak < 0.001f)
            return { false, "Source was silent; add checks are vacuous" };

        return { true, "Adder sums channel-wise; neutral for unconnected inputs (peak "
                     + std::to_string (peak) + ")" };
    }

    // Velocity flow: note.vel -> gain.vel must scale exactly, and the same
    // gain without a vel wire must stay sound-invariant.
    TestResult runVelocityGainTest()
    {
        std::atomic<float> waveform { 0.0f };     // sine
        std::atomic<float> gainFactor { 1.0f };   // unity: transparent

        smolfm::SignalGraph graph;

        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();

        auto osc = std::make_unique<smolfm::OscillatorProcessor> (
            &waveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* oscPtr = osc.get();

        auto gain = std::make_unique<smolfm::GainProcessor> (&gainFactor);
        smolfm::GainProcessor* gainPtr = gain.get();

        // Graph order = render order: the note renders FIRST so a vel
        // consumer reads the fresh velocity in the same tick.
        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (osc));
        graph.addProcessor (std::move (gain));

        if (! oscPtr->getNoteInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to osc note_in" };

        if (! gainPtr->getInput().connect (oscPtr->getOutput()))
            return { false, "Failed to connect osc to gain in" };

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        graph.startNote();

        // Reference: without a vel connection the unity gain must pass the
        // oscillator through untouched (sound-invariance).
        float referencePeak = 0.0f;
        for (int i = 0; i < 800; ++i)
        {
            graph.processSample();
            referencePeak = std::max (referencePeak,
                std::abs (gainPtr->getOutput().getSample().l));
        }

        if (referencePeak < 0.05f)
            return { false, "Reference source was silent; velocity checks are vacuous" };

        // Wire note.vel -> gain.vel at velocity 0.5: from the very first
        // sample the gain output must be exactly half the oscillator frame.
        if (! gainPtr->getVelInput().connect (notePtr->getVelOutput()))
            return { false, "Failed to connect note vel to gain vel" };

        notePtr->setVelocity (0.5f);

        for (int i = 0; i < 800; ++i)
        {
            graph.processSample();

            const smolfm::StereoFrame oscFrame  = oscPtr->getOutput().getSample();
            const smolfm::StereoFrame gainFrame = gainPtr->getOutput().getSample();

            if (std::abs (gainFrame.l - 0.5f * oscFrame.l) > 0.001f)
                return { false, "connected gain.vel must scale by velocity 0.5" };

            if (std::abs (gainFrame.r - 0.5f * oscFrame.r) > 0.001f)
                return { false, "connected gain.vel must scale by velocity 0.5 (right)" };
        }

        // Velocity 1.0 must restore the unwired behaviour exactly.
        notePtr->setVelocity (1.0f);
        graph.processSample();

        const float full   = std::abs (gainPtr->getOutput().getSample().l);
        const float source = std::abs (oscPtr->getOutput().getSample().l);
        if (std::abs (full - source) > 0.001f)
            return { false, "velocity 1.0 must be neutral (no extra scaling)" };

        return { true, "gain.vel scales 0.5 exactly, 1.0 neutral, unwired invariant (peak "
                     + std::to_string (referencePeak) + ")" };
    }

    // Velocity flow for the waveshaper: shape.vel at velocity 0.5 with a
    // static drive of 0.5 must be sample-exact identical to an unwired
    // shaper whose static drive is 0.25 — the accent is pure drive scaling.
    TestResult runVelocityWaveshaperTest()
    {
        std::atomic<float> waveform   { 0.0f };  // sine
        std::atomic<float> shapeIndex { 0.0f };  // soft clip
        std::atomic<float> driveFull  { 0.5f };  // wired shaper, scaled by vel
        std::atomic<float> driveHalf  { 0.25f }; // unwired reference

        smolfm::SignalGraph graph;

        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();

        auto osc = std::make_unique<smolfm::OscillatorProcessor> (
            &waveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* oscPtr = osc.get();

        auto wiredShaper = std::make_unique<smolfm::WaveshaperProcessor> (&driveFull, &shapeIndex);
        smolfm::WaveshaperProcessor* wiredPtr = wiredShaper.get();

        auto referenceShaper = std::make_unique<smolfm::WaveshaperProcessor> (&driveHalf, &shapeIndex);
        smolfm::WaveshaperProcessor* refPtr = referenceShaper.get();

        // Graph order = render order: the note renders FIRST so a vel
        // consumer reads the fresh velocity in the same tick.
        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (osc));
        graph.addProcessor (std::move (wiredShaper));
        graph.addProcessor (std::move (referenceShaper));

        if (! oscPtr->getNoteInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to osc note_in" };

        if (! wiredPtr->getInput().connect (oscPtr->getOutput()))
            return { false, "Failed to connect osc to wired shaper" };

        if (! refPtr->getInput().connect (oscPtr->getOutput()))
            return { false, "Failed to connect osc to reference shaper" };

        if (! wiredPtr->getVelInput().connect (notePtr->getVelOutput()))
            return { false, "Failed to connect note vel to shaper vel" };

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        notePtr->setVelocity (0.5f);
        graph.startNote();

        float sourcePeak = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            graph.processSample();

            const smolfm::StereoFrame src     = oscPtr->getOutput().getSample();
            const smolfm::StereoFrame wired   = wiredPtr->getOutput().getSample();
            const smolfm::StereoFrame unwired = refPtr->getOutput().getSample();

            sourcePeak = std::max (sourcePeak, std::abs (src.l));

            if (std::abs (wired.l - unwired.l) > 0.0f
                || std::abs (wired.r - unwired.r) > 0.0f)
                return { false, "shape.vel must halve drive 0.5 -> 0.25 sample-exact" };
        }

        if (sourcePeak < 0.001f)
            return { false, "Source was silent; shape.vel checks are vacuous" };

        return { true, "shape.vel halves the drive (wired 0.5 @ v=0.5 == unwired 0.25; peak "
                     + std::to_string (sourcePeak) + ")" };
    }

    // Velocity flow for the FM stage: fm.vel at velocity 0.5 with a static
    // FM index of 2 must be sample-exact identical to an unwired stage with
    // index 1 — the accent scales the index linearly, nothing else.
    TestResult runVelocityFmTest()
    {
        std::atomic<float> modulatorWaveform { 0.0f }; // sine
        std::atomic<float> fmAmountFull { 2.0f };      // wired stage, scaled by vel
        std::atomic<float> fmAmountHalf { 1.0f };      // unwired reference

        smolfm::SignalGraph graph;

        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();

        auto modOsc = std::make_unique<smolfm::OscillatorProcessor> (
            &modulatorWaveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* modOscPtr = modOsc.get();

        auto wiredFm = std::make_unique<smolfm::FMModulationProcessor> (&fmAmountFull);
        smolfm::FMModulationProcessor* wiredPtr = wiredFm.get();

        auto referenceFm = std::make_unique<smolfm::FMModulationProcessor> (&fmAmountHalf);
        smolfm::FMModulationProcessor* refPtr = referenceFm.get();

        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (modOsc));
        graph.addProcessor (std::move (wiredFm));
        graph.addProcessor (std::move (referenceFm));

        if (! modOscPtr->getNoteInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to modulator note_in" };

        if (! wiredPtr->getFreqInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to wired fm carrier" };

        if (! refPtr->getFreqInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to reference fm carrier" };

        if (! wiredPtr->getModulatorInput().connect (modOscPtr->getOutput()))
            return { false, "Failed to connect modulator to wired fm" };

        if (! refPtr->getModulatorInput().connect (modOscPtr->getOutput()))
            return { false, "Failed to connect modulator to reference fm" };

        if (! wiredPtr->getVelInput().connect (notePtr->getVelOutput()))
            return { false, "Failed to connect note vel to fm vel" };

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        notePtr->setVelocity (0.5f);
        graph.startNote();

        float modulatorPeak = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            graph.processSample();

            const smolfm::StereoFrame modulator = modOscPtr->getOutput().getSample();
            const smolfm::StereoFrame wired     = wiredPtr->getOutput().getSample();
            const smolfm::StereoFrame unwired   = refPtr->getOutput().getSample();

            modulatorPeak = std::max (modulatorPeak, std::abs (modulator.l));

            if (std::abs (wired.l - unwired.l) > 0.0f)
                return { false, "fm.vel must halve the index 2 -> 1 sample-exact" };
        }

        if (modulatorPeak < 0.001f)
            return { false, "Modulator was silent; fm.vel checks are vacuous" };

        return { true, "fm.vel halves the FM index (wired 2 @ v=0.5 == unwired 1; modulator peak "
                     + std::to_string (modulatorPeak) + ")" };
    }

    // Velocity flow for the filter: filter.vel at velocity 0.5 with a static
    // cutoff of 4000 Hz must be sample-exact identical to an unwired filter
    // at 2000 Hz — the accent scales the cutoff, nothing else.
    TestResult runVelocityFilterTest()
    {
        std::atomic<float> waveform   { 0.0f };  // sine
        std::atomic<float> cutoffFull { 4000.0f }; // wired filter, scaled by vel
        std::atomic<float> cutoffHalf { 2000.0f }; // unwired reference
        std::atomic<float> resonanceShared { 0.5f };
        std::atomic<float> modeShared { 0.0f };    // low pass

        smolfm::SignalGraph graph;

        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();

        auto osc = std::make_unique<smolfm::OscillatorProcessor> (
            &waveform, nullptr, nullptr, nullptr);
        smolfm::OscillatorProcessor* oscPtr = osc.get();

        auto wiredFilter = std::make_unique<smolfm::FilterProcessor> (
            &cutoffFull, &resonanceShared, &modeShared);
        smolfm::FilterProcessor* wiredPtr = wiredFilter.get();

        auto referenceFilter = std::make_unique<smolfm::FilterProcessor> (
            &cutoffHalf, &resonanceShared, &modeShared);
        smolfm::FilterProcessor* refPtr = referenceFilter.get();

        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (osc));
        graph.addProcessor (std::move (wiredFilter));
        graph.addProcessor (std::move (referenceFilter));

        if (! oscPtr->getNoteInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to osc note_in" };

        if (! wiredPtr->getInput().connect (oscPtr->getOutput()))
            return { false, "Failed to connect osc to wired filter" };

        if (! refPtr->getInput().connect (oscPtr->getOutput()))
            return { false, "Failed to connect osc to reference filter" };

        if (! wiredPtr->getVelInput().connect (notePtr->getVelOutput()))
            return { false, "Failed to connect note vel to filter vel" };

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        notePtr->setVelocity (0.5f);
        graph.startNote();

        float sourcePeak = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            graph.processSample();

            const smolfm::StereoFrame src     = oscPtr->getOutput().getSample();
            const smolfm::StereoFrame wired   = wiredPtr->getOutput().getSample();
            const smolfm::StereoFrame unwired = refPtr->getOutput().getSample();

            sourcePeak = std::max (sourcePeak, std::abs (wired.l));

            if (std::abs (wired.l - unwired.l) > 0.0f
                || std::abs (wired.r - unwired.r) > 0.0f)
                return { false, "filter.vel must halve the cutoff 4000 -> 2000 Hz sample-exact" };
        }

        if (sourcePeak < 0.001f)
            return { false, "Source was silent; filter.vel checks are vacuous" };

        return { true, "filter.vel halves the cutoff (wired 4000 Hz @ v=0.5 == unwired 2000 Hz; peak "
                     + std::to_string (sourcePeak) + ")" };
    }

    // Velocity flow for the pitch envelope: fadsr.vel scales the ANCHOR
    // DEPTH (pull-to-neutral), not the envelope shape.  The wired node
    // (up 3.0 / down 0.5) at velocity 0.5 must be sample-exact identical to
    // a reference node running the velocity-pulled anchors (up 2.0 / down
    // 0.75) — the formula is 1 + v * (anchor - 1) per anchor, so the anchor
    // gap shrinks proportionally while A/D/S/R times stay untouched.
    TestResult runVelocityFAdsrTest()
    {
        // Envelope timings shared by both nodes (same rails in time); the
        // attack/decay sweep crosses E = 0..1..sustain inside the window so
        // every phase of the envelope is covered by the comparison.
        std::atomic<float> envAttack  { 0.002f };
        std::atomic<float> envDecay   { 0.004f };
        std::atomic<float> envSustain { 0.25f };
        std::atomic<float> envRelease { 0.01f };
        std::atomic<float> wiredUp    { 3.0f };
        std::atomic<float> wiredDown  { 0.5f };
        std::atomic<float> refUp      { 2.0f };   // == 1 + 0.5 * (3.0 - 1)
        std::atomic<float> refDown    { 0.75f };  // == 1 + 0.5 * (0.5 - 1)

        smolfm::SignalGraph graph;

        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();

        auto wired = std::make_unique<smolfm::FAdsrProcessor> (
            &envAttack, &envDecay, &envSustain, &envRelease, &wiredUp, &wiredDown);
        smolfm::FAdsrProcessor* wiredPtr = wired.get();

        auto reference = std::make_unique<smolfm::FAdsrProcessor> (
            &envAttack, &envDecay, &envSustain, &envRelease, &refUp, &refDown);
        smolfm::FAdsrProcessor* refPtr = reference.get();

        // Graph order = render order: the note renders FIRST so both
        // envelopes read the fresh frequency/velocity in the same tick.
        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (wired));
        graph.addProcessor (std::move (reference));

        if (! wiredPtr->getFreqInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to wired fadsr freq_in" };

        if (! refPtr->getFreqInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to reference fadsr freq_in" };

        if (! wiredPtr->getVelInput().connect (notePtr->getVelOutput()))
            return { false, "Failed to connect note vel to fadsr vel" };

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        notePtr->setVelocity (0.5f);
        graph.startNote();

        // v = 0.5 must equal the reference with the pulled anchors — sample
        // exact across attack peak, decay and sustain.
        float minimum = 1e9f;
        float maximum = -1e9f;
        for (int i = 0; i < 1000; ++i)
        {
            graph.processSample();

            const smolfm::StereoFrame wiredOut = wiredPtr->getOutput().getSample();
            const smolfm::StereoFrame refOut   = refPtr->getOutput().getSample();

            if (std::abs (wiredOut.l - refOut.l) > 0.0f
                || std::abs (wiredOut.r - refOut.r) > 0.0f)
                return { false, "fadsr.vel must pull the anchor depths to (2.0, 0.75) at v=0.5 sample-exact" };

            minimum = std::min (minimum, wiredOut.l);
            maximum = std::max (maximum, wiredOut.l);
        }

        if (minimum >= maximum)
            return { false, "Pitch output never moved; fadsr.vel checks are vacuous" };

        // v = 1.0 must sit exactly on the configured anchors: with the wire
        // still active the node must now equal a reference running the plain
        // configured factors (unwired-equivalent behaviour).
        refUp.store (3.0f);
        refDown.store (0.5f);
        notePtr->setVelocity (1.0f);

        for (int i = 0; i < 1000; ++i)
        {
            graph.processSample();

            const smolfm::StereoFrame wiredOut = wiredPtr->getOutput().getSample();
            const smolfm::StereoFrame refOut   = refPtr->getOutput().getSample();

            if (std::abs (wiredOut.l - refOut.l) > 0.0f
                || std::abs (wiredOut.r - refOut.r) > 0.0f)
                return { false, "fadsr.vel at velocity 1.0 must reproduce the configured anchors (3.0, 0.5) sample-exact" };
        }

        return { true, "fadsr.vel scales anchor depth linearly (v=0.5 -> anchors 2.0/0.75, v=1.0 -> 3.0/0.5), sample-exact"
                     + std::string ("; pitch range ")
                     + std::to_string (minimum) + ".." + std::to_string (maximum) + " Hz" };
    }

    TestResult runResonatorRingTest()
    {
        // One 440 Hz body: square-wave exciter (pitch mode, note 69) gated
        // off after 0.25 s; the mode must keep ringing at ~440 Hz and decay
        // towards the t60.
        std::atomic<float> tune   { 440.0f };
        std::atomic<float> q      { 0.8f };
        std::atomic<float> decay  { 0.3f };
        std::atomic<float> spread { 1.0f };
        std::atomic<float> wfm     { 2.0f };   // square
        std::atomic<float> oscMode { 0.0f };   // pitch (needs the note gate)
        std::atomic<float> oscFreq { 440.0f };
        std::atomic<float> lfo     { 1.0f };
        std::atomic<float> gate    { 1.0f };

        smolfm::SignalGraph graph;
        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();
        auto osc = std::make_unique<smolfm::OscillatorProcessor> (&wfm, &oscMode, &oscFreq, &lfo);
        smolfm::OscillatorProcessor* oscPtr = osc.get();
        auto gain = std::make_unique<smolfm::GainProcessor> (&gate);
        smolfm::GainProcessor* gainPtr = gain.get();
        auto res = std::make_unique<smolfm::ResonatorProcessor> (&tune, &q, &decay, &spread, nullptr, nullptr);
        smolfm::ResonatorProcessor* resPtr = res.get();

        if (! oscPtr->getNoteInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to osc note_in" };
        if (! gainPtr->getInput().connect (oscPtr->getOutput()))
            return { false, "Failed to connect osc to gate gain" };
        if (! resPtr->getInput().connect (gainPtr->getOutput()))
            return { false, "Failed to connect gate gain to resonator" };

        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (osc));
        graph.addProcessor (std::move (gain));
        graph.addProcessor (std::move (res));

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        graph.startNote();

        // 0.25 s excitation at full gate — the body must respond.  Do not
        // sample one instant: 12000 samples = exactly 200 periods of 800 Hz,
        // so the frame would sit on a zero crossing of the ring.  Take the
        // window peak instead.
        float excPeak = 0.0f;
        for (int i = 0; i < 12000; ++i)
        {
            const smolfm::StereoFrame y = graph.processSample();
            excPeak = std::max (excPeak, std::abs (y.l));
        }
        if (excPeak < 1.0e-2f)
            return { false, "resonator output silent during excitation (peak "
                          + std::to_string (excPeak) + ")" };

        gate.store (0.0f);   // excitation stops

        // The ring must continue at the tuning frequency: count zero
        // crossings in the first 0.125 s after the gate closes (440 Hz ->
        // ~110 crossings, +/-30 % tolerance for the envelope ramp).
        int crossings = 0;
        float prev = 0.0f;
        for (int i = 0; i < 6000; ++i)
        {
            const smolfm::StereoFrame y = graph.processSample();
            if ((prev < 0.0f) != (y.l < 0.0f) && std::abs (y.l) > 1.0e-4f)
                ++crossings;
            prev = y.l;
        }
        if (crossings < 75 || crossings > 145)
            return { false, "ring frequency off: " + std::to_string (crossings)
                          + " crossings in 0.125 s (expected ~110 at 440 Hz)" };

        if (! resPtr->hasEnergy())
            return { false, "resonator must report tail energy right after the excitation stopped" };

        // The ring must decay with the configured t60: RMS(first 0.125 s) >
        // RMS(last 0.125 s of the same 0.5 s window) by a clear factor
        // (e^(-t/tau) with tau = t60/ln(1000) predicts ~4.2, allow 2+).
        double sumEarly = 0.0;
        double sumLate  = 0.0;
        for (int i = 0; i < 24000; ++i)
        {
            const smolfm::StereoFrame y = graph.processSample();
            if (i < 6000)  sumEarly += double (y.l) * double (y.l);
            else if (i >= 18000) sumLate += double (y.l) * double (y.l);
        }
        const double rmsEarly = std::sqrt (sumEarly / 6000.0);
        const double rmsLate  = std::sqrt (sumLate  / 6000.0);
        // tau = t60/ln(1000) predicts e^(0.375/tau) ≈ 5500 for these window
        // centers; the check stays a decay-order sanity bound.
        if (rmsEarly < 1.0e-3 || rmsEarly / rmsLate < 20.0)
            return { false, "ring does not decay towards t60 (rms ratio "
                          + std::to_string (rmsEarly / rmsLate) + ")" };

        return { true, "resonator rings at tune ("
                     + std::to_string (crossings) + " crossings/0.125 s), decays after excitation"
                     + std::string ("; rms early/late = ") + std::to_string (rmsEarly / rmsLate) };
    }

    TestResult runResonatorVelDecayTest()
    {
        // Two identical bodies, one with vel spread 1 (unwired), one with
        // spread 2 wired to note.vel = 1.0: the accented body must ring
        // significantly longer (t_eff = t * (1 + v*(spread-1)) = 2*t).
        struct Setup
        {
            std::atomic<float> tune  { 440.0f };
            std::atomic<float> q     { 0.8f };
            std::atomic<float> decay { 0.3f };
            std::atomic<float> spread;
            std::atomic<float> wfm     { 2.0f };
            std::atomic<float> oscMode { 0.0f };   // pitch (needs the note gate)
            std::atomic<float> oscFreq { 440.0f };
            std::atomic<float> lfo     { 1.0f };
            std::atomic<float> gate    { 1.0f };
            std::atomic<float> velocity { 1.0f };

            Setup (float spreadValue) : spread (spreadValue) {}
        };

        auto runBody = [] (Setup& s)
        {
            smolfm::SignalGraph graph;
            auto note = std::make_unique<smolfm::NoteProcessor>();
            smolfm::NoteProcessor* notePtr = note.get();
            auto osc = std::make_unique<smolfm::OscillatorProcessor> (&s.wfm, &s.oscMode, &s.oscFreq, &s.lfo);
            smolfm::OscillatorProcessor* oscPtr = osc.get();
            auto gain = std::make_unique<smolfm::GainProcessor> (&s.gate);
            smolfm::GainProcessor* gainPtr = gain.get();
            auto res = std::make_unique<smolfm::ResonatorProcessor> (&s.tune, &s.q, &s.decay, &s.spread, nullptr, nullptr);
            smolfm::ResonatorProcessor* resPtr = res.get();

            if (! oscPtr->getNoteInput().connect (notePtr->getOutput()))
                return -1.0f;
            if (! gainPtr->getInput().connect (oscPtr->getOutput()))
                return -1.0f;
            if (! resPtr->getInput().connect (gainPtr->getOutput()))
                return -1.0f;
            if (! resPtr->getVelInput().connect (notePtr->getVelOutput()))
                return -1.0f;

            graph.addProcessor (std::move (note));
            graph.addProcessor (std::move (osc));
            graph.addProcessor (std::move (gain));
            graph.addProcessor (std::move (res));

            graph.prepare (48000.0);
            notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
            notePtr->setVelocity (s.velocity.load());
            graph.startNote();

            for (int i = 0; i < 12000; ++i)   // 0.25 s excitation
                graph.processSample();

            s.gate.store (0.0f);
            double sum = 0.0;
            for (int i = 0; i < 12000; ++i)   // window 0.25..0.5 s after stop
            {
                const smolfm::StereoFrame y = graph.processSample();
                if (i >= 7200)                // 0.15..0.25 s into the window
                    sum += double (y.l) * double (y.l);
            }
            return (float) std::sqrt (sum / 4800.0);
        };

        Setup plain (1.0f);
        Setup accented (2.0f);

        const float rmsPlain    = runBody (plain);
        const float rmsAccented = runBody (accented);

        if (rmsPlain < 0.0f || rmsAccented < 0.0f)
            return { false, "wiring failed in resonator vel test" };

        if (rmsPlain < 1.0e-4f)
            return { false, "reference ring already died; vel check vacuous (rmsPlain "
                          + std::to_string (rmsPlain) + ", rmsAccented "
                          + std::to_string (rmsAccented) + ")" };

        // spread 2 doubles t60 -> in the late window the accented ring holds
        // a clearly larger amplitude (e^(-t/tau) predicts ~10x here).
        if (rmsAccented <= rmsPlain * 2.5f)
            return { false, "vel-on-decay without effect: rms ratio "
                          + std::to_string (rmsAccented / rmsPlain) };

        return { true, "vel-on-decay: accented ring survives longer (rms ratio "
                     + std::to_string (rmsAccented / rmsPlain) + ")" };
    }

    TestResult runFeedbackLoopTest()
    {
        // Direct self-loop: out = in + amount * out_prev.  With amount 0.5 a
        // 440 Hz sine passes with gain |1/(1 - 0.5 e^-jw)| ~ 2.0; the loop
        // must stay stable and must become an exact pass-through at 0.
        std::atomic<float> amount  { 0.5f };
        std::atomic<float> wfm     { 0.0f };   // sine
        std::atomic<float> oscMode { 0.0f };   // pitch (needs the note gate)
        std::atomic<float> oscFreq { 440.0f };
        std::atomic<float> lfo     { 1.0f };

        smolfm::SignalGraph graph;
        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();
        auto osc = std::make_unique<smolfm::OscillatorProcessor> (&wfm, &oscMode, &oscFreq, &lfo);
        smolfm::OscillatorProcessor* oscPtr = osc.get();
        auto fb = std::make_unique<smolfm::FeedbackProcessor> (&amount);
        smolfm::FeedbackProcessor* fbPtr = fb.get();

        if (! oscPtr->getNoteInput().connect (notePtr->getOutput()))
            return { false, "Failed to connect note to osc note_in" };
        if (! fbPtr->getInput().connect (oscPtr->getOutput()))
            return { false, "Failed to connect osc to loop entry" };
        if (! fbPtr->getFeedbackInput().connect (fbPtr->getOutput()))
            return { false, "Failed to wire the direct self-loop (fb.out -> fb.fb_in)" };

        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (osc));
        graph.addProcessor (std::move (fb));

        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (69);   // A4 = 440 Hz
        graph.startNote();

        double sumIn  = 0.0;
        double sumOut = 0.0;
        float maxIn  = 0.0f;
        float maxOut = 0.0f;
        for (int i = 0; i < 9600; ++i)
        {
            const smolfm::StereoFrame x = oscPtr->getOutput().getSample();
            const smolfm::StereoFrame y = graph.processSample();
            sumIn  += double (x.l) * double (x.l);
            sumOut += double (y.l) * double (y.l);
            maxIn  = std::max (maxIn, std::abs (x.l));
            maxOut = std::max (maxOut, std::abs (y.l));
            maxOut = std::max (maxOut, std::abs (y.r));
        }
        // NaN must not slip through comparisons (NaN compares false).
        if (! (maxOut <= 10.0f) || ! (maxIn <= 10.0f))
            return { false, "loop produced non-finite samples: maxIn "
                          + std::to_string (maxIn) + ", maxOut " + std::to_string (maxOut) };

        const double rmsIn  = std::sqrt (sumIn  / 9600.0);
        const double rmsOut = std::sqrt (sumOut / 9600.0);
        const double ratio  = rmsOut / rmsIn;
        if (rmsIn < 1.0e-6f || ! (ratio >= 1.5 && ratio <= 2.4))
            return { false, "loop gain off: rmsIn " + std::to_string (rmsIn)
                          + ", rmsOut " + std::to_string (rmsOut)
                          + ", ratio " + std::to_string (ratio)
                          + " (expected ~2.0 = |1/(1-0.5 e^-jw)| at 440 Hz)" };

        // amount = 0 must be a sample-exact pass-through.  Read x AFTER the
        // tick: the osc runs earlier in the graph, so reading first would
        // pair the previous tick's frame with this tick's output.
        amount.store (0.0f);
        for (int i = 0; i < 480; ++i)
        {
            const smolfm::StereoFrame y = graph.processSample();
            const smolfm::StereoFrame x = oscPtr->getOutput().getSample();
            if (std::abs (x.l - y.l) > 0.0f || std::abs (x.r - y.r) > 0.0f)
                return { false, "amount=0 must be a transparent pass-through" };
        }

        return { true, "feedback loop stable and scaled (gain "
                     + std::to_string (ratio) + " at amount 0.5); amount 0 = transparent" };
    }

    // Regression (TR-808 session, 03.10.2026): the filter-mode loader used to
    // append "Mode" to the FULL cutoff template — "filter%Cutoff" + "Mode"
    // produced "filter0CutoffMode", which does not exist in the APVTS
    // ("filter0Mode").  apvts.getParameter() then returned nullptr, the
    // setParameter() call silently no-oped, and every patch file loaded with
    // the declared default mode (LP) no matter what the XML said.  This is
    // the bug that made all .smolfm filters sound like they filter nothing.
    TestResult runFilterModeParameterIdTest()
    {
        const juce::String modeId = smolfm::GraphNodeRegistry::filterModeParameterIdFor ("filter0");
        if (modeId != "filter0Mode")
            return { false, "filterModeParameterIdFor(\"filter0\") = \""
                          + modeId.toStdString()
                          + "\" — must be filter0Mode" };

        if (smolfm::GraphNodeRegistry::filterCutoffParameterIdFor ("filter0") != "filter0Cutoff")
            return { false, "filterCutoffParameterIdFor must still resolve to filter0Cutoff" };

        if (smolfm::GraphNodeRegistry::filterResonanceParameterIdFor ("filter0") != "filter0Resonance")
            return { false, "filterResonanceParameterIdFor must still resolve to filter0Resonance" };

        // The waveshaper sibling the append-pattern came from must not regress.
        const juce::String driveId = smolfm::GraphNodeRegistry::driveParameterIdFor ("shape3");
        if (driveId != "shape3Drive")
            return { false, "driveParameterIdFor(\"shape3\") = \""
                          + driveId.toStdString()
                          + "\" — must be shape3Drive" };

        return { true, "filter mode id resolves to the APVTS id (filter0Mode)" };
    }

    // TEMP diagnostic (user report: waveshaper shapes/drive feel unapplied,
    // drive barely audible).  Measures the out peak of a 0.37-amplitude sine
    // (the bass drum ring's measured level) through the waveshaper for a
    // grid of shape x drive x velocity.  No assertions — the table IS the
    // answer.
    TestResult runWaveshaperTransferTable()
    {
        std::string table = "waveshaper transfer (0.37 sine, peak out):";
        for (int shapeIndex : { 0, 1, 2 })
        for (float driveValue : { 0.05f, 0.25f })
        for (float v : { 1.0f, 0.3f })
        {
            std::atomic<float> wfm { 0.0f };   // sine
            std::atomic<float> oscMode { 0.0f };   // pitch (needs the note gate)
            std::atomic<float> oscFreq { 220.0f };
            std::atomic<float> lfo { 1.0f };
            std::atomic<float> level { 0.37f };
            std::atomic<float> drive { driveValue };
            std::atomic<float> shape { (float) shapeIndex };

            smolfm::SignalGraph graph;
            auto note = std::make_unique<smolfm::NoteProcessor>();
            smolfm::NoteProcessor* notePtr = note.get();
            auto osc = std::make_unique<smolfm::OscillatorProcessor> (&wfm, &oscMode, &oscFreq, &lfo);
            smolfm::OscillatorProcessor* oscPtr = osc.get();
            auto toneGain = std::make_unique<smolfm::GainProcessor> (&level);
            smolfm::GainProcessor* toneGainPtr = toneGain.get();
            auto shaper = std::make_unique<smolfm::WaveshaperProcessor> (&drive, &shape);
            smolfm::WaveshaperProcessor* shaperPtr = shaper.get();

            if (! oscPtr->getNoteInput().connect (notePtr->getOutput()))
                return { false, "Failed to wire note to osc" };
            if (! toneGainPtr->getInput().connect (oscPtr->getOutput()))
                return { false, "Failed to wire osc to level gain" };
            if (! shaperPtr->getInput().connect (toneGainPtr->getOutput()))
                return { false, "Failed to wire level gain to shaper" };
            if (! shaperPtr->getVelInput().connect (notePtr->getVelOutput()))
                return { false, "Failed to wire vel to shaper" };

            graph.addProcessor (std::move (note));
            graph.addProcessor (std::move (osc));
            graph.addProcessor (std::move (toneGain));
            graph.addProcessor (std::move (shaper));

            graph.prepare (48000.0);
            notePtr->setMidiNoteNumber (57);    // A3 = 220 Hz
            notePtr->setVelocity (v);
            graph.startNote();

            float peak = 0.0f;
            for (int i = 0; i < 480; ++i)       // 10 ms, well inside attack+drive
            {
                const smolfm::StereoFrame y = graph.processSample();
                peak = std::max (peak, std::abs (y.l));
            }

            const char* shapeName = shapeIndex == 0 ? "soft" : shapeIndex == 1 ? "hard" : "fold";
            table += "\n  " + std::string (shapeName)
                   + " drive=" + std::to_string (driveValue)
                   + " vel=" + std::to_string (v)
                   + " -> peak " + std::to_string (peak);
        }
        return { true, table };
    }

    // Phase-2 strike probe (03.10.2026, plan doc §9.2/§9.5): the internal
    // body strike as its own measurement, in a bare chain — the note's
    // velocity runs into a resonator whose EXCITER INPUT IS UNWIRED.
    // Whatever rings after a hit came from the internal strike alone.
    //   (a) neutrality: either strike knob at 0 (the defaults) = the body
    //       stays dead; nothing is injected (the existing ring/vel/chain
    //       probes still render the strike-off path — the regression
    //       evidence that the old behaviour is untouched).
    //   (b) determinism: two identical hits ring identically (no random
    //       spread factor anywhere in the strike).
    //   (c) the energy REALLY depends on velocity: the strike peak follows
    //       the raw accent (no sqrt curve inside the resonator), so
    //       RMS(vel 1.0) / RMS(vel 0.5) ≈ 2 — the Phase-3 level law,
    //       folded into the strike path in the same move (plan §9.5).
    TestResult runResonatorStrikeProbe()
    {
        std::atomic<float> tune  { 49.5f };
        std::atomic<float> q     { 0.85f };
        std::atomic<float> decay { 0.30f };
        std::atomic<float> spread { 1.0f };   // 1 = the strike law alone
        std::atomic<float> strikeLevelAtom { 0.4077f };   // the patch's strikelevel
        std::atomic<float> strikeTimeAtom  { 0.008f };   // the old grain gate

        smolfm::SignalGraph graph;
        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();
        auto res = std::make_unique<smolfm::ResonatorProcessor> (&tune, &q, &decay, &spread,
                                                                 &strikeLevelAtom, &strikeTimeAtom);
        smolfm::ResonatorProcessor* resPtr = res.get();
        if (! resPtr->getStrikeVelInput().connect (notePtr->getVelOutput()))
            return { false, "Failed to wire note vel to the strike accent line" };

        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (res));
        graph.prepare (48000.0);
        notePtr->setMidiNoteNumber (36);   // MIDI 36 = the GM bass drum key

        auto ringRms1k = [&] (float vel)   // first 100 ms of the ring
        {
            resPtr->reset();
            notePtr->setVelocity (vel);
            graph.startNote();
            double sum = 0.0;
            for (int i = 0; i < 4800; ++i)
            {
                const smolfm::StereoFrame y = graph.processSample();
                sum += double (y.l) * double (y.l) + double (y.r) * double (y.r);
            }
            return std::sqrt (sum / 9600.0);
        };

        // (a) both knobs out: an unwired body stays silent — the strike
        // injects NOTHING when off; either knob alone gates.
        strikeLevelAtom.store (0.0f);
        const double rmsLevelOff = ringRms1k (1.0f);
        if (rmsLevelOff > 0.0)
            return { false, "strike-out must leave an unwired body silent (rms "
                          + std::to_string (rmsLevelOff) + ")" };

        strikeLevelAtom.store (0.4077f);
        strikeTimeAtom.store (0.0f);
        const double rmsTimeOff = ringRms1k (1.0f);
        if (rmsTimeOff > 0.0)
            return { false, "strikeTime 0 must keep the strike dead (rms "
                          + std::to_string (rmsTimeOff) + ")" };

        // (b) determinism: identical hits, identical rings — byte for byte.
        strikeTimeAtom.store (0.008f);
        const double rmsFull = ringRms1k (1.0f);
        const double rmsFullB = ringRms1k (1.0f);
        if (! (rmsFull > 0.0))
            return { false, "strike body never rings (rms " + std::to_string (rmsFull) + ")" };
        if (rmsFull != rmsFullB)
            return { false, "identical hits must ring identically ("
                          + std::to_string (rmsFull) + " vs "
                          + std::to_string (rmsFullB) + ")" };

        // (c) the velocity law: the peak follows the RAW accent (linear), so
        // the ring RMS halves between vel 1.0 and vel 0.5 — ratio ≈ 2.
        const double rmsHalf = ringRms1k (0.5f);
        if (! (rmsHalf > 0.0))
            return { false, "strike body silent at vel 0.5 (rms " + std::to_string (rmsHalf) + ")" };
        const double ratio = rmsFull / rmsHalf;
        if (! (ratio >= 1.8 && ratio <= 2.2))
            return { false, "level law off: rms(1.0) " + std::to_string (rmsFull)
                          + ", rms(0.5) " + std::to_string (rmsHalf)
                          + ", ratio " + std::to_string (ratio) + " (expected ~2)" };

        return { true, "strike probe: off stays silent, identical hits ring "
                     + std::to_string (rmsFull) + " rms both times, vel 1.0/0.5 ratio "
                     + std::to_string (ratio) + " (~2 — amplitude follows the raw accent)" };
    }

    // Bass drum chain probe, now on the Phase-2 topology (03.10.2026, plan
    // doc §9.2): the internal strike replaced the noise burst as the
    // velocity-driven ignition — the live tr808-bass-drum patch reads:
    // tuned 25 ms sine impulse (x2) through the 0.22 feedback bridge, the
    // 49.5 Hz body (decay 0.30, vel spread 2.5) with strikelevel/striketime
    // carried on its own raw accent line (strike_vel: note vel directly, no
    // vsc curve inside the strike), then the soft waveshaper (drive 0.25,
    // vel-wired) into the 10.0 output gain.  The probe reports every
    // stage's level, so a dead link between node stages can never hide
    // behind a total.  It also renders the fb4d341-era chain (tuned sine
    // x2 + noise x0.5 burst through the adder, no strike knobs = the old
    // binary behaviour) as the level anchor: at vel 1.0 the exchanged body
    // level must land within a band of the old grain's contribution, and
    // the strike must be measurable against its own absence (level knob
    // to 0).
    TestResult runBassDrumChainTest()
    {
        // Calibration note (03.10.2026): this number and the patch file's
        // strikelevel MUST stay in lockstep — the probe solved it from the
        // old grain's early-ring RMS (0.328925 body-RMS legacy vs 0.032652
        // sine-only floor): solved strikeLevel 0.407744 → rounded 0.4077.
        constexpr float strikeLevelForProbe { 0.4077f };

        // Internal strike knobs (Phase 2).
        std::atomic<float> strikeLevelAtom { strikeLevelForProbe };
        std::atomic<float> strikeTimeAtom  { 0.008f };   // the old grain gate length

        // Legacy (fb4d341-era) exciter atoms: the noise burst the internal
        // strike replaces, kept for the level anchor.
        std::atomic<float> wfm     { 1.0f };   // noise
        std::atomic<float> oscMode { 1.0f };   // static
        std::atomic<float> oscFreq { 6000.0f };
        std::atomic<float> lfo     { 1.0f };
        std::atomic<float> excA  { 0.001f };   // grain gate attack
        std::atomic<float> excD  { 0.008f };   // grain gate decay (8 ms burst)
        std::atomic<float> excS  { 0.0f };
        std::atomic<float> excR  { 0.005f };
        std::atomic<float> excGainAmt { 0.5f };   // the old quiet grain level

        // Tuned sine strike atoms (both graphs share the same knobs).
        std::atomic<float> strikeWfm     { 0.0f };   // sine
        std::atomic<float> strikeOscMode { 0.0f };   // pitch — follows the note
        std::atomic<float> strikeOscFreq { 49.5f };  // static fallback
        std::atomic<float> strikeLfo     { 1.0f };
        std::atomic<float> strikeA  { 0.001f };   // tuned impulse (25 ms)
        std::atomic<float> strikeD  { 0.025f };
        std::atomic<float> strikeS  { 0.0f };
        std::atomic<float> strikeR  { 0.005f };
        std::atomic<float> strikeGainAmt { 2.0f };

        // Shared body/exit atoms (both graphs read the same knobs).
        std::atomic<float> fbAmount  { 0.22f };
        std::atomic<float> tune   { 49.5f };
        std::atomic<float> q      { 0.85f };
        std::atomic<float> decay  { 0.30f };
        std::atomic<float> spread { 2.5f };
        std::atomic<float> drive  { 0.25f };
        std::atomic<float> shape  { 0.0f };    // soft
        std::atomic<float> outGainAmt { 10.0f };

        smolfm::SignalGraph graph;         // live: post-Phase-2 topology
        smolfm::SignalGraph legacyGraph;   // anchor: fb4d341-era topology

        // Live graph: the tuned sine strike into the feedback bridge; the
        // body carries the internal strike knobs on its raw accent line.
        auto note = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* notePtr = note.get();
        auto osc = std::make_unique<smolfm::OscillatorProcessor> (&strikeWfm, &strikeOscMode, &strikeOscFreq, &strikeLfo);
        smolfm::OscillatorProcessor* oscPtr = osc.get();   // the strike sine
        auto strikeAdsr = std::make_unique<smolfm::AdsrProcessor> (&strikeA, &strikeD, &strikeS, &strikeR);
        smolfm::AdsrProcessor* strikeAdsrPtr = strikeAdsr.get();
        auto strikeGain = std::make_unique<smolfm::GainProcessor> (&strikeGainAmt);
        smolfm::GainProcessor* strikeGainPtr = strikeGain.get();
        auto fb = std::make_unique<smolfm::FeedbackProcessor> (&fbAmount);
        smolfm::FeedbackProcessor* fbPtr = fb.get();
        auto res = std::make_unique<smolfm::ResonatorProcessor> (&tune, &q, &decay, &spread,
                                                                 &strikeLevelAtom, &strikeTimeAtom);
        smolfm::ResonatorProcessor* resPtr = res.get();
        auto shaper = std::make_unique<smolfm::WaveshaperProcessor> (&drive, &shape);
        smolfm::WaveshaperProcessor* shaperPtr = shaper.get();
        auto outGain = std::make_unique<smolfm::GainProcessor> (&outGainAmt);

        if (! strikeAdsrPtr->getInput().connect (oscPtr->getOutput()))
            return { false, "Failed to connect strike sine to its gate" };
        if (! oscPtr->getNoteInput().connect (notePtr->getOutput()))
            return { false, "Failed to wire note out to the strike sine" };
        if (! strikeGainPtr->getInput().connect (strikeAdsrPtr->getOutput()))
            return { false, "Failed to connect strike gate to strike gain" };
        if (! fbPtr->getInput().connect (strikeGainPtr->getOutput()))
            return { false, "Failed to connect strike gain to feedback bridge" };
        if (! fbPtr->getFeedbackInput().connect (resPtr->getOutput()))
            return { false, "Failed to wire resonator return into feedback bridge" };
        if (! resPtr->getInput().connect (fbPtr->getOutput()))
            return { false, "Failed to connect feedback bridge to resonator body" };
        if (! resPtr->getVelInput().connect (notePtr->getVelOutput()))
            return { false, "Failed to wire note vel to resonator vel" };
        if (! resPtr->getStrikeVelInput().connect (notePtr->getVelOutput()))
            return { false, "Failed to wire note vel to the RAW strike_vel line" };
        if (! shaperPtr->getInput().connect (resPtr->getOutput()))
            return { false, "Failed to connect resonator out to waveshaper" };
        if (! shaperPtr->getVelInput().connect (notePtr->getVelOutput()))
            return { false, "Failed to wire note vel to waveshaper vel" };
        if (! outGain->getInput().connect (shaperPtr->getOutput()))
            return { false, "Failed to connect waveshaper to output gain" };
        graph.addProcessor (std::move (note));
        graph.addProcessor (std::move (osc));
        graph.addProcessor (std::move (strikeAdsr));
        graph.addProcessor (std::move (strikeGain));
        graph.addProcessor (std::move (fb));
        graph.addProcessor (std::move (res));
        graph.addProcessor (std::move (shaper));
        graph.addProcessor (std::move (outGain));

        // -- Legacy graph (fb4d341-era, the level anchor): both exciters wire
        // through the adder (sine x2 + noise x0.5), and the resonator carries
        // NO strike knobs at all (the nullptr pair = the old binary
        // behaviour, the strike structurally absent).
        auto legacyNote = std::make_unique<smolfm::NoteProcessor>();
        smolfm::NoteProcessor* legacyNotePtr = legacyNote.get();
        auto legacyNoiseOsc = std::make_unique<smolfm::OscillatorProcessor> (&wfm, &oscMode, &oscFreq, &lfo);
        smolfm::OscillatorProcessor* legacyNoiseOscPtr = legacyNoiseOsc.get();
        auto legacyStrikeOsc = std::make_unique<smolfm::OscillatorProcessor> (&strikeWfm, &strikeOscMode, &strikeOscFreq, &strikeLfo);
        auto legacyGrainAdsr = std::make_unique<smolfm::AdsrProcessor> (&excA, &excD, &excS, &excR);
        smolfm::AdsrProcessor* legacyGrainAdsrPtr = legacyGrainAdsr.get();
        auto legacyGrainGain = std::make_unique<smolfm::GainProcessor> (&excGainAmt);
        smolfm::GainProcessor* legacyGrainGainPtr = legacyGrainGain.get();
        auto legacyStrikeAdsr = std::make_unique<smolfm::AdsrProcessor> (&strikeA, &strikeD, &strikeS, &strikeR);
        smolfm::AdsrProcessor* legacyStrikeAdsrPtr = legacyStrikeAdsr.get();
        auto legacyStrikeGain = std::make_unique<smolfm::GainProcessor> (&strikeGainAmt);
        auto legacyAdd = std::make_unique<smolfm::AdderProcessor>();
        auto legacyFb = std::make_unique<smolfm::FeedbackProcessor> (&fbAmount);
        smolfm::FeedbackProcessor* legacyFbPtr = legacyFb.get();
        auto legacyRes = std::make_unique<smolfm::ResonatorProcessor> (&tune, &q, &decay, &spread, nullptr, nullptr);
        smolfm::ResonatorProcessor* legacyResPtr = legacyRes.get();
        auto legacyShaper = std::make_unique<smolfm::WaveshaperProcessor> (&drive, &shape);
        auto legacyOutGain = std::make_unique<smolfm::GainProcessor> (&outGainAmt);

        if (! legacyGrainAdsrPtr->getInput().connect (legacyNoiseOscPtr->getOutput()))
            return { false, "Failed to connect legacy noise to its gate" };
        if (! legacyGrainGain->getInput().connect (legacyGrainAdsrPtr->getOutput()))
            return { false, "Failed to connect legacy grain gate to gain" };
        if (! legacyAdd->getInput2().connect (legacyGrainGain->getOutput()))
            return { false, "Failed to connect legacy grain gain to add" };
        if (! legacyStrikeAdsr->getInput().connect (legacyStrikeOsc->getOutput()))
            return { false, "Failed to connect legacy strike sine to its gate" };
        if (! legacyStrikeOsc->getNoteInput().connect (legacyNotePtr->getOutput()))
            return { false, "Failed to wire legacy note out to the strike sine" };
        if (! legacyStrikeGain->getInput().connect (legacyStrikeAdsr->getOutput()))
            return { false, "Failed to connect legacy strike gate to gain" };
        if (! legacyAdd->getInput1().connect (legacyStrikeGain->getOutput()))
            return { false, "Failed to connect legacy strike gain to add" };
        if (! legacyFbPtr->getInput().connect (legacyAdd->getOutput()))
            return { false, "Failed to connect legacy adder to feedback bridge" };
        if (! legacyFbPtr->getFeedbackInput().connect (legacyResPtr->getOutput()))
            return { false, "Failed to wire legacy resonator return" };
        if (! legacyResPtr->getInput().connect (legacyFbPtr->getOutput()))
            return { false, "Failed to connect legacy bridge to body" };
        if (! legacyResPtr->getVelInput().connect (legacyNotePtr->getVelOutput()))
            return { false, "Failed to wire legacy note vel to resonator vel" };
        if (! legacyShaper->getInput().connect (legacyResPtr->getOutput()))
            return { false, "Failed to connect legacy body out to shaper" };
        if (! legacyShaper->getVelInput().connect (legacyNotePtr->getVelOutput()))
            return { false, "Failed to wire legacy note vel to shaper vel" };
        if (! legacyOutGain->getInput().connect (legacyShaper->getOutput()))
            return { false, "Failed to connect legacy shaper to output gain" };
        legacyGraph.addProcessor (std::move (legacyNote));
        legacyGraph.addProcessor (std::move (legacyNoiseOsc));
        legacyGraph.addProcessor (std::move (legacyStrikeOsc));
        legacyGraph.addProcessor (std::move (legacyGrainAdsr));
        legacyGraph.addProcessor (std::move (legacyGrainGain));
        legacyGraph.addProcessor (std::move (legacyStrikeAdsr));
        legacyGraph.addProcessor (std::move (legacyStrikeGain));
        legacyGraph.addProcessor (std::move (legacyAdd));
        legacyGraph.addProcessor (std::move (legacyFb));
        legacyGraph.addProcessor (std::move (legacyRes));
        legacyGraph.addProcessor (std::move (legacyShaper));
        legacyGraph.addProcessor (std::move (legacyOutGain));

        graph.prepare (48000.0);
        legacyGraph.prepare (48000.0);
        // NOTE: the patch wires note.out -> resonator0.tune, so the played
        // note tunes the body; MIDI 36 (C2) = the GM bass drum note.
        resPtr->getTuneInput().connect (notePtr->getOutput());
        legacyResPtr->getTuneInput().connect (legacyNotePtr->getOutput());
        notePtr->setMidiNoteNumber (36);
        legacyNotePtr->setMidiNoteNumber (36);
        notePtr->setVelocity (1.0f);
        legacyNotePtr->setVelocity (1.0f);

        // -- Level anchor, part 1: the legacy chain (fb4d341 era: the noise
        // grain x0.5 behind the 8 ms gate plus the tuned sine x2 through the
        // adder) hits first.  Its early-ring RMS (first 100 ms of the body
        // output) anchors the exchange level.
        legacyGraph.startNote();
        double legacyResSum = 0.0;
        float legacyGrainPeak = 0.0f;
        for (int i = 0; i < 48000; ++i)
        {
            const smolfm::StereoFrame y = legacyGraph.processSample();
            if (i < 4800)
            {
                // The anchor reads the BODY itself (same link the live chain
                // measures) — NOT the x10 output gain stage.
                const smolfm::StereoFrame body = legacyResPtr->getOutput().getSample();
                legacyResSum += double (body.l) * double (body.l);
                const smolfm::StereoFrame grain = legacyGrainGainPtr->getOutput().getSample();
                legacyGrainPeak = std::max (legacyGrainPeak, std::abs (grain.l));
            }
        }
        const double legacyEarlyRms = std::sqrt (legacyResSum / 4800.0);

        // -- Level anchor, part 2: the live chain (same note, strike knobs
        // on) after it.  Every stage's level as before, so a dead link
        // between node stages can never hide behind a total.
        graph.startNote();
        float srcPeak = 0.0f;
        float resEarlyPeak = 0.0f;
        float shaperPeak = 0.0f;
        float finalPeak = 0.0f;
        double resEarlySum = 0.0;
        double resLateSum = 0.0;
        double finalSum = 0.0;
        for (int i = 0; i < 48000; ++i)   // 1 s at 48 kHz
        {
            const smolfm::StereoFrame y = graph.processSample();
            const smolfm::StereoFrame src = oscPtr->getOutput().getSample();
            const smolfm::StereoFrame res = resPtr->getOutput().getSample();
            const smolfm::StereoFrame shp = shaperPtr->getOutput().getSample();

            shaperPeak = std::max (shaperPeak, std::abs (shp.l));
            finalPeak  = std::max (finalPeak,  std::abs (y.l));
            finalSum   += double (y.l) * double (y.l);
            if (i < 4800) {                  // first 100 ms
                srcPeak      = std::max (srcPeak,      std::abs (src.l));
                resEarlyPeak = std::max (resEarlyPeak, std::abs (res.l));
                resEarlySum += double (res.l) * double (res.l);
            }
            if (i >= 24000)                  // body-ring tail window
                resLateSum += double (res.l) * double (res.l);
        }
        const float resLateRms = (float) std::sqrt (resLateSum / 24000.0);
        const double finalRms  = std::sqrt (finalSum / 48000.0);
        const double liveEarlyRms = std::sqrt (resEarlySum / 4800.0);

        if (legacyGrainPeak < 0.01f)
            return { false, "stage probe: legacy grain silent (legacyGrainPeak "
                          + std::to_string (legacyGrainPeak) + ")" };
        if (! (legacyEarlyRms > 0.0))
            return { false, "stage probe: legacy body never rings (legacyEarlyRms "
                          + std::to_string (legacyEarlyRms) + ")" };
        if (srcPeak < 0.01f)
            return { false, "stage probe: tuned strike silent (srcPeak "
                          + std::to_string (srcPeak) + ")" };
        if (resEarlyPeak < 0.01f)
            return { false, "stage probe: body never rings (resEarlyPeak "
                          + std::to_string (resEarlyPeak) + ", srcPeak "
                          + std::to_string (srcPeak) + ")" };
        if (finalPeak < 0.02f)
            return { false, "stage probe: OUTPUT INAUDIBLE (finalPeak "
                          + std::to_string (finalPeak) + ", srcPeak "
                          + std::to_string (srcPeak) + ", resEarlyPeak "
                          + std::to_string (resEarlyPeak) + ", shaperPeak "
                          + std::to_string (shaperPeak) + ", resLateRms "
                          + std::to_string (resLateRms) + ", finalRms "
                          + std::to_string (finalRms) + ")" };

        // -- Strike audible against its own absence + calibration solve:
        // level knob to 0 (window stays), one more hit — that render IS the
        // sine-only floor c.  From the linear additivity of the body
        //   live(L)²  = c² + s(L)²    (strike share s ∝ L)
        //   legacy²   = c² + g²       (old grain share g, the x0.5 noise)
        // the probe solves the level that lands the strike exactly on the
        // old grain's contribution and prints it as the patch recommendation.
        strikeLevelAtom.store (0.0f);
        resPtr->reset();
        fbPtr->reset();
        graph.startNote();
        double offSum = 0.0;
        for (int i = 0; i < 4800; ++i)
        {
            const smolfm::StereoFrame res = resPtr->getOutput().getSample();
            offSum += double (res.l) * double (res.l);
        }
        const double strikeOffRms = std::sqrt (offSum / 4800.0);
        strikeLevelAtom.store (strikeLevelForProbe);

        if (! (liveEarlyRms > strikeOffRms * 1.02))
            return { false, "strike adds nothing measurable: strike-on rms "
                          + std::to_string (liveEarlyRms) + " vs strike-off rms "
                          + std::to_string (strikeOffRms) };

        // Calibration solve (03.10.2026): the strike's share in the body's
        // early ring should sit on the old grain's share.  The strike is a
        // COHERENT (in-phase) sine added to the carrier — a plain RMS-
        // additivity solve therefore under-counts it systematically and the
        // solved recommendation drifts with the base level it is solved
        // from (measured: 0.41 solved from L=0.5, 0.34 from L=0.4077).
        // The probe still recomputes and REPORTS it, but only flags a gross
        // drift (the additive solve carries ~20% systematic bias here).
        const double strikeShare = std::sqrt (std::max (0.0, liveEarlyRms * liveEarlyRms - strikeOffRms * strikeOffRms));
        const double grainShare  = std::sqrt (std::max (0.0, legacyEarlyRms * legacyEarlyRms - strikeOffRms * strikeOffRms));
        const double solvedLevel = strikeShare > 0.0
            ? strikeLevelForProbe * grainShare / strikeShare
            : -1.0;
        if (solvedLevel < 0.0 || std::abs (solvedLevel - (double) strikeLevelForProbe) > 0.40 * solvedLevel)
            return { false, "calibration drift: shipped strikeLevel "
                          + std::to_string (strikeLevelForProbe)
                          + " but the body measures "
                          + std::to_string (solvedLevel)
                          + " (strike share " + std::to_string (strikeShare)
                          + ", grain share " + std::to_string (grainShare)
                          + ", sine-only floor " + std::to_string (strikeOffRms) + ")" };

        // Exchange band: the internal strike must land the body's early ring
        // close to the old grain's contribution at vel 1.0.  The velocity
        // law itself is checked exactly in the strike probe.
        if (! (liveEarlyRms >= legacyEarlyRms * 0.7 && liveEarlyRms <= legacyEarlyRms * 1.4))
            return { false, "strike level off vs old grain: legacyEarlyRms "
                          + std::to_string (legacyEarlyRms) + ", liveEarlyRms "
                          + std::to_string (liveEarlyRms) + " (strikeLevel "
                          + std::to_string (strikeLevelAtom.load())
                          + ", solved level " + std::to_string (solvedLevel) + ")" };

        // -- Phase 2: fast re-triggers (16th notes at 140 BPM = 107 ms).  The
        // loop re-rings into a body that is still alive; the per-window
        // output peak must stay bounded, never climb hit over hit.
        const int windowSamples = 5142;   // 107 ms at 48 kHz
        float windowPeak[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        for (int w = 0; w < 6; ++w)
        {
            notePtr->setVelocity (1.0f);
            graph.startNote();
            for (int i = 0; i < windowSamples; ++i)
            {
                const smolfm::StereoFrame y = graph.processSample();
                windowPeak[w] = std::max (windowPeak[w], std::abs (y.l));
            }
        }

        // -- Phase 3A: single-ring decay WITH the feedback bridge.  Re-zero
        // the circulating state first so the profile starts clean.
        resPtr->reset();
        fbPtr->reset();
        notePtr->setVelocity (1.0f);
        graph.startNote();
        float decayA[8] = {};
        for (int w = 0; w < 8; ++w)
        {
            double wSum = 0.0;
            for (int i = 0; i < 12000; ++i)   // 250 ms at 48 kHz
            {
                const smolfm::StereoFrame y = graph.processSample();
                wSum += double (y.l) * double (y.l);
            }
            decayA[w] = (float) std::sqrt (wSum / 12000.0);
        }

        // -- Phase 3B: same single hit, but the feedback bridge is BYPASSED
        // (the strike gain feeds the body directly).  A passive body must
        // decay monotonically; comparison A vs B localises the misbehaviour.
        if (! resPtr->getInput().connect (strikeGainPtr->getOutput()))
            return { false, "Failed to rewire exciter directly to body" };
        resPtr->reset();
        graph.startNote();
        float decayB[8] = {};
        for (int w = 0; w < 8; ++w)
        {
            double wSum = 0.0;
            for (int i = 0; i < 12000; ++i)
            {
                const smolfm::StereoFrame y = graph.processSample();
                wSum += double (y.l) * double (y.l);
            }
            decayB[w] = (float) std::sqrt (wSum / 12000.0);
        }

        std::string peaks = "retrigger peaks:";
        for (int w = 0; w < 6; ++w)  peaks += " " + std::to_string (windowPeak[w]);
        std::string profileA = "decay rms WITH loop:";
        for (int w = 0; w < 8; ++w)  profileA += " " + std::to_string (decayA[w]);
        std::string profileB = "decay rms bypassed:";
        for (int w = 0; w < 8; ++w)  profileB += " " + std::to_string (decayB[w]);

        if (decayB[7] >= decayB[0] * 0.5f)
            return { false, "stage probe: BODY DOES NOT DECAY EVEN BYPASSED ("
                          + profileB + "; " + profileA + ")" };

        float worstPeak = windowPeak[0];
        for (int w = 1; w < 6; ++w) worstPeak = std::max (worstPeak, windowPeak[w]);

        // -- Phase 4: velocity sensitivity (the user's x10-gain report).  The
        // kick multiplies three velocity inputs (out gain, shaper drive,
        // decay spread) — the level at half velocity should show how steep
        // that stack is.  Restore the bridge wiring first (3B bypassed it)
        // and re-zero the circulating state.
        if (! resPtr->getInput().connect (fbPtr->getOutput()))
            return { false, "Failed to restore feedback bridge for velocity sweep" };
        resPtr->reset();
        fbPtr->reset();
        std::string velLine = "velocity rms (first 500 ms):";
        for (float v : { 1.0f, 0.5f, 0.25f })
        {
            // Fresh circulating state per step: without this the previous
            // hit's ring leaks into the next window and the sweep only
            // measures the decay rate, not the velocity.
            resPtr->reset();
            fbPtr->reset();
            notePtr->setVelocity (v);
            graph.startNote();
            double vSum = 0.0;
            float vPeak = 0.0f;
            for (int i = 0; i < 24000; ++i)
            {
                const smolfm::StereoFrame y = graph.processSample();
                vSum += double (y.l) * double (y.l);
                vPeak = std::max (vPeak, std::abs (y.l));
            }
            velLine += "; v=" + std::to_string (v)
                     + " rms " + std::to_string (std::sqrt (vSum / 24000.0))
                     + " peak " + std::to_string (vPeak);
        }

        return { true, "bass drum chain rings (legacyEarlyRms "
                     + std::to_string (legacyEarlyRms)
                     + ", liveEarlyRms " + std::to_string (liveEarlyRms)
                     + ", strikeOffRms " + std::to_string (strikeOffRms)
                     + ", srcPeak " + std::to_string (srcPeak)
                     + ", resEarlyPeak " + std::to_string (resEarlyPeak)
                     + ", shaperPeak " + std::to_string (shaperPeak)
                     + ", resLateRms " + std::to_string (resLateRms)
                     + ", finalPeak " + std::to_string (finalPeak)
                     + ", finalRms " + std::to_string (finalRms)
                     + "; " + peaks
                     + "; " + profileA
                     + "; " + profileB
                     + "; " + velLine + ")" };
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
    runTest ("Adder signal sum", runAdderSumTest);
    runTest ("Velocity gain scaling", runVelocityGainTest);
    runTest ("Velocity waveshaper scaling", runVelocityWaveshaperTest);
    runTest ("Velocity FM index scaling", runVelocityFmTest);
    runTest ("Velocity filter cutoff scaling", runVelocityFilterTest);
    runTest ("Filter mode parameter id", runFilterModeParameterIdTest);
    runTest ("Bass drum chain probe", runBassDrumChainTest);
    runTest ("Waveshaper transfer table", runWaveshaperTransferTable);
    runTest ("Velocity fadsr depth scaling", runVelocityFAdsrTest);
    runTest ("Resonator ring + decay", runResonatorRingTest);
    runTest ("Resonator vel-on-decay", runResonatorVelDecayTest);
    runTest ("Resonator strike", runResonatorStrikeProbe);
    runTest ("Feedback loop", runFeedbackLoopTest);

    return failures;
}
