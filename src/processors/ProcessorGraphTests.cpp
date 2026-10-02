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
        auto res = std::make_unique<smolfm::ResonatorProcessor> (&tune, &q, &decay, &spread);
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
            auto res = std::make_unique<smolfm::ResonatorProcessor> (&s.tune, &s.q, &s.decay, &s.spread);
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
    runTest ("Velocity fadsr depth scaling", runVelocityFAdsrTest);
    runTest ("Resonator ring + decay", runResonatorRingTest);
    runTest ("Resonator vel-on-decay", runResonatorVelDecayTest);
    runTest ("Feedback loop", runFeedbackLoopTest);

    return failures;
}
