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
#include <array>
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
    triangle,
    noise,
    perlinNoise,
    simplexNoise
};

/**
    Convert an integer waveform index (as stored by juce::AudioParameterChoice)
    into the matching Waveform.  Unknown indices fall back to sine.

    The mapping must stay in sync with the choices listed in
    PluginProcessor::createParameterLayout()
    ("Sine", "Saw", "Square", "Triangle", "Noise").
*/
inline Waveform waveformFromIndex (int index) noexcept
{
    switch (index)
    {
        case 1:  return Waveform::saw;
        case 2:  return Waveform::square;
        case 3:  return Waveform::triangle;
        case 4:  return Waveform::noise;
        case 5:  return Waveform::perlinNoise;
        case 6:  return Waveform::simplexNoise;
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
        // Initialize permutation table for noise functions
        for (int i = 0; i < 256; ++i)
            permTable[i] = permutation[i % 16];
    }

    /**
        Set the oscillator frequency in Hertz.

        The frequency is only used to calculate the phase increment; the actual
        waveform is produced by getNextSample().
    */
        void setFrequency (float newFrequency)
    {
        // Clamp the instantaneous frequency to [-Nyquist, Nyquist].
        // Negative frequencies are legal: under strong FM the deviation can
        // push the carrier through zero, and the phase then runs backwards.
        // Frequencies beyond Nyquist would alias, so they stay clamped.
        frequency = juce::jlimit (-sampleRate * 0.5f, sampleRate * 0.5f, newFrequency);
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

        For non-sinusoidal waveforms (saw, square, triangle), phase wrapping
        is handled with fmod() to prevent discontinuities from frequency
        modulation.  The frequency is clamped to [-Nyquist, Nyquist] in
        setFrequency(); negative frequencies (through-zero FM) run the phase
        backwards and the wrap keeps it inside [0, 2*pi).
    */
    float getNextSample (float phaseModulation = 0.0f)
    {
                // Advance the oscillator's own phase first.
        phase += phaseIncrement;

        // Normalise phase to [0, 2*pi) range.
        // For non-sinusoidal waveforms this prevents discontinuities when
        // frequency modulation causes rapid phase changes.  For sine this is
        // mathematically equivalent but less critical since sine is continuous.
        phase = std::fmod (phase, juce::MathConstants<float>::twoPi);
        if (phase < 0.0f)
            phase += juce::MathConstants<float>::twoPi;

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

    mutable juce::Random noiseSource { 42 };   // noise must advance in const evaluateWaveform

    // Permutation table for Perlin/Simplex noise (Ken Perlin reference)
    static constexpr std::array<int, 16> permutation = { 151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225 };
    std::array<int, 256> permTable {};

    // Noise coordinates (wrapped phase scaled to tile size)
    static constexpr float noiseTileSize = 32.0f;
    static constexpr int noiseOctaves = 4;

    // Gradient lookup for simplex noise
    static constexpr float grad8[8][2] = {
        { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 },
        { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }
    };

    /**
        Recalculate phaseIncrement after frequency or sample rate changes.
    */
    void updatePhaseIncrement()
    {
        phaseIncrement = juce::MathConstants<float>::twoPi * frequency / sampleRate;
    }

    //==============================================================
    // Perlin Noise helper functions
    //==============================================================
    float fade (float t) const
    {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    float lerp (float t, float a, float b) const
    {
        return a + t * (b - a);
    }

    float grad (int hash, float x, float y) const
    {
        const float h = static_cast<float> (hash & 7);
        const float u = (h < 4) ? x : y;
        const float v = (h < 4) ? y : x;
        return ((hash & 1) ? -u : u) + ((hash & 2) ? -v : v);
    }

    float perlinNoise2D (float x, float y) const
    {
        const int X = static_cast<int> (std::floor (x)) & 255;
        const int Y = static_cast<int> (std::floor (y)) & 255;

        x -= std::floor (x);
        y -= std::floor (y);

        const float u = fade (x);
        const float v = fade (y);

        const int a = permTable[X] + Y;
        const int aa = permTable[a & 255];
        const int ab = permTable[(a + 1) & 255];
        const int b = permTable[(X + 1) & 255] + Y;
        const int ba = permTable[b & 255];
        const int bb = permTable[(b + 1) & 255];

        const float res = lerp (v,
            lerp (u, grad (permTable[aa & 255], x, y), grad (permTable[ba & 255], x - 1.0f, y)),
            lerp (u, grad (permTable[ab & 255], x, y - 1.0f), grad (permTable[bb & 255], x - 1.0f, y - 1.0f)));

        return res;
    }

    //==============================================================
    // Simplex Noise helper functions
    //==============================================================
    float simplexCorner (int hash, float x, float y) const
    {
        const float t = 0.5f - x * x - y * y;
        if (t < 0.0f) return 0.0f;

        const float t2 = t * t;
        const float t4 = t2 * t2;

        const float gx = grad8[hash & 7][0];
        const float gy = grad8[hash & 7][1];
        return t4 * (gx * x + gy * y);
    }

    float simplexNoise2D (float x, float y) const
    {
        const float F2 = 0.36602540378443864676f;
        const float G2 = 0.21132486540518711774f;

        const float s = (x + y) * F2;
        const int i = static_cast<int> (std::floor (x + s));
        const int j = static_cast<int> (std::floor (y + s));

        const float t = static_cast<float> (i + j) * G2;
        const float x0 = x - static_cast<float> (i) + t;
        const float y0 = y - static_cast<float> (j) + t;

        const int i1 = (x0 > y0) ? 1 : 0;
        const int j1 = (x0 > y0) ? 0 : 1;

        const float x1 = x0 - static_cast<float> (i1) + G2;
        const float y1 = y0 - static_cast<float> (j1) + G2;
        const float x2 = x0 - 1.0f + 2.0f * G2;
        const float y2 = y0 - 1.0f + 2.0f * G2;

        const int ii = i & 255;
        const int jj = j & 255;

        const float n0 = simplexCorner (permTable[ii + permTable[jj] & 255], x0, y0);
        const float n1 = simplexCorner (permTable[ii + i1 + permTable[jj + j1] & 255], x1, y1);
        const float n2 = simplexCorner (permTable[ii + 1 + permTable[jj + 1] & 255], x2, y2);

        return 70.0f * (n0 + n1 + n2);
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

                        case Waveform::noise:
                // White noise: uniform random value per sample, independent of
                // phase.  juce::Random is seeded once in prepare() via its
                // static default seeding; setSeed is NOT called so every
                // voice differs slightly (fine for musical noise).
                return noiseSource.nextFloat() * 2.0f - 1.0f;

            case Waveform::perlinNoise:
            {
                // Map phase [0, 2pi) to noise tile [0, noiseTileSize)
                const float x = (phaseAngle / juce::MathConstants<float>::twoPi) * noiseTileSize;
                const float y = 0.0f;  // 1D slice of 2D noise
                // Octave noise for smoother results
                float total = 0.0f;
                float freq = 1.0f;
                float amp = 1.0f;
                float maxVal = 0.0f;
                for (int i = 0; i < noiseOctaves; ++i)
                {
                    total += perlinNoise2D (x * freq, y * freq) * amp;
                    maxVal += amp;
                    amp *= 0.5f;
                    freq *= 2.0f;
                }
                return total / maxVal;
            }

            case Waveform::simplexNoise:
            {
                // Map phase [0, 2pi) to noise tile [0, noiseTileSize)
                const float x = (phaseAngle / juce::MathConstants<float>::twoPi) * noiseTileSize;
                const float y = 0.0f;  // 1D slice of 2D noise
                return simplexNoise2D (x, y);
            }
        }

        // This line is never reached, but it keeps the compiler happy.
        return 0.0f;
    }
};

} // namespace smolfm
