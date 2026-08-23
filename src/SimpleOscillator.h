/*
    SimpleOscillator is a tiny oscillator we use for FM synthesis.

    JUCE already has juce::dsp::Oscillator, but it is built around adding an
    input signal to the oscillator output.  For phase modulation FM it is
    clearer to keep the phase, the waveform function and the phase increment
    in one small class.  This also makes the FM math easy to read:

        output = waveform (phase + phaseModulation)

    The oscillator function receives a phase angle in radians and returns a
    sample value between -1.0f and 1.0f.
*/

#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

namespace smolfm
{

/**
    The waveforms we can choose from.

    Using an enum class means the compiler will not silently treat a waveform
    value as a normal integer, which helps avoid accidental mistakes.
*/
enum class Waveform
{
    sine,
    saw,
    square,
    triangle
};

/**
    Convert an integer waveform index (as stored by juce::AudioParameterChoice)
    into the matching Waveform.  Unknown indices fall back to sine.

    The mapping must stay in sync with the choices listed in
    PluginProcessor::createParameterLayout() ("Sine", "Saw", "Square", "Triangle").
*/
inline Waveform waveformFromIndex (int index) noexcept
{
    switch (index)
    {
        case 1:  return Waveform::saw;
        case 2:  return Waveform::square;
        case 3:  return Waveform::triangle;
        default: return Waveform::sine;
    }
}

/**
    A small oscillator with selectable waveform and explicit phase handling.

    It is deliberately simple: it does not use band-limited waveforms, so very
    high frequencies will alias.  That is acceptable for a first synthesizer.
    Band-limited waveforms can be added later without changing the structure.
*/
class SimpleOscillator
{
public:
    /**
        Prepare the oscillator for a new sample rate.

        The phase increment is the amount the phase advances each sample.  It
        depends on the desired frequency and the sample rate:

            phaseIncrement = 2 * pi * frequency / sampleRate
    */
    void prepare (double newSampleRate)
    {
        sampleRate = static_cast<float> (newSampleRate);
        updatePhaseIncrement();
    }

    /**
        Set the oscillator frequency in Hertz.

        The frequency is only used to calculate the phase increment; the actual
        waveform is produced by getNextSample().
    */
    void setFrequency (float newFrequency)
    {
        frequency = newFrequency;
        updatePhaseIncrement();
    }

    /**
        Choose which waveform this oscillator produces.
    */
    void setWaveform (Waveform newWaveform)
    {
        waveform = newWaveform;
    }

    /**
        Generate the next oscillator sample.

        The optional phaseModulation argument is the FM phase offset in radians.
        The carrier calls this with the modulator's output scaled by the FM
        amount, so the carrier's effective phase becomes:

            phase + phaseModulation

        This is the heart of FM synthesis: the modulator is not added to the
        carrier's output, it pushes the carrier's phase forward and backward.
    */
    float getNextSample (float phaseModulation = 0.0f)
    {
        // Advance the oscillator's own phase first.
        phase += phaseIncrement;

        // Wrap the phase so it always stays in the range [0, 2*pi).
        // This avoids the phase growing without bound and keeps the math tidy.
        while (phase >= juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;

        // Evaluate the selected waveform at the modulated phase.
        return evaluateWaveform (phase + phaseModulation);
    }

    /**
        Reset the oscillator phase to zero.

        Called when a new note starts so each note begins from the same phase.
    */
    void resetPhase()
    {
        phase = 0.0f;
    }

private:
    float sampleRate = 44100.0f;
    float frequency  = 0.0f;
    float phase      = 0.0f;
    float phaseIncrement = 0.0f;
    Waveform waveform = Waveform::sine;

    /**
        Recalculate phaseIncrement after frequency or sample rate changes.
    */
    void updatePhaseIncrement()
    {
        phaseIncrement = juce::MathConstants<float>::twoPi * frequency / sampleRate;
    }

    /**
        Convert a phase angle into a waveform sample.

        All functions expect a phase in radians and produce values in the
        range [-1.0f, 1.0f].  The phase is wrapped to [0, 2*pi) before the
        waveform is evaluated so callers can pass modulated phases safely.
    */
    float evaluateWaveform (float phaseAngle) const
    {
        // Wrap any phase offset back into the fundamental [0, 2*pi) range.
        // fmod keeps the sign of the numerator; adding twoPi once after a
        // negative result normalises back into [0, 2*pi).
        phaseAngle = std::fmod (phaseAngle, juce::MathConstants<float>::twoPi);
        if (phaseAngle < 0.0f)
            phaseAngle += juce::MathConstants<float>::twoPi;

        switch (waveform)
        {
            case Waveform::sine:
                return std::sin (phaseAngle);

            case Waveform::saw:
                // A sawtooth ramps from -1 to +1 across one cycle.
                // This is not band-limited and will alias at high frequencies.
                return 2.0f * (phaseAngle / juce::MathConstants<float>::twoPi) - 1.0f;

            case Waveform::square:
                return (phaseAngle < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;

            case Waveform::triangle:
                // A triangle ramps linearly from -1 to +1 and back across one
                // cycle.  Not band-limited; aliases like the saw at high
                // frequencies, but softer because its harmonics decay faster.
                return (phaseAngle < juce::MathConstants<float>::pi)
                     ? 2.0f * (phaseAngle / juce::MathConstants<float>::pi) - 1.0f
                     : 3.0f - 2.0f * (phaseAngle / juce::MathConstants<float>::pi);
        }

        // This line is never reached, but it keeps the compiler happy.
        return 0.0f;
    }
};

} // namespace smolfm
