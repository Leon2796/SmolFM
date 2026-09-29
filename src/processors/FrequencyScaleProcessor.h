/*
    FrequencyScaleProcessor scales a frequency by a constant factor in the
    frequency domain.

    Ports:
        - freq_in (frequency): incoming frequency in Hertz, wired from
                               note.out, an FM stage or another scaler.
        - out     (frequency): the scaled frequency in Hertz.

    Per sample:

        f_out = f_in * interval_ratio * factor

    The interval parameter is the musical shortcut: a named interval
    (octave, fifth, ...) maps onto its exact 12-TET ratio, so a patch can
    say "octave_up" instead of writing 2.0.  The factor stays free-range
    for arbitrary ratios; both multiply together.

    interval = none with factor = 1 passes the frequency through unchanged,
    so an idle scaler in a chain is fully transparent.

    Stateless: Hertz in, Hertz out.  The oscillator at the end of the chain
    keeps owning the phase integration, so this node can sit anywhere in a
    frequency cascade (transpose by octaves with factor 2.0, mute with 0.0).
*/

#pragma once

#include "ProcessorPort.h"

namespace smolfm
{

/**
    Number of interval shortcut choices; must match the choice count in
    PluginProcessor::createParameterLayout() and the name table in
    ISmolFmParser.h (fscaleIntervalNames).
*/
inline constexpr int numFscaleIntervals = 11;

/**
    Exact 12-TET frequency ratios for the interval shortcut, 2^(steps/12).
    Order must stay in sync with:

        PluginProcessor::createParameterLayout()  (APVTS choice order)
        ISmolFmParser.h::fscaleIntervalNames()    (file attribute names)
        FrequencyScaleComponent                   (dropdown item order)
*/
inline constexpr float fscaleIntervalRatios[numFscaleIntervals]
{
    1.0f,      // none           (transparent)
    2.0f,      // octave up
    0.5f,      // octave down
    1.498307f, // fifth up       (2^(7/12))
    0.667420f, // fifth down
    1.334840f, // fourth up      (2^(5/12))
    0.749154f, // fourth down
    1.189207f, // minor third up (2^(3/12))
    1.259921f, // major third up (2^(4/12))
    1.059463f, // semitone up    (2^(1/12))
    0.943874f  // semitone down
};

/** Map an interval choice index onto its ratio.  Out-of-range indices and
    "none" fall back to 1.0, i.e. no shift (keeps a wrong raw value safe). */
inline float fscaleIntervalRatioFromIndex (int index) noexcept
{
    if (index < 0 || index >= numFscaleIntervals)
        return 1.0f;

    return fscaleIntervalRatios[index];
}

/**
    Chainable frequency scaler operating on Hertz values.

    The single input carries a frequency; the output carries that frequency
    multiplied by the interval ratio (musical dropdown) and the factor
    parameter (free-range knob).
*/
class FrequencyScaleProcessor final : public Processor
{
public:
    /**
        Create the frequency scale node.

        @param factorParameter   atomic pointer to the scale factor (0-10 in
                                 the UI; 1.0 means transparent).
        @param intervalParameter atomic pointer to the interval choice index
                                 (0 = none; see fscaleIntervalRatioFromIndex).
                                 nullptr keeps the node legacy-transparent.
    */
    explicit FrequencyScaleProcessor (std::atomic<float>* factorParameter,
                                      std::atomic<float>* intervalParameter = nullptr);

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    InputPort& getFreqInput() noexcept      { return freqInput; }
    OutputPort& getOutput() noexcept        { return output; }

private:
    std::atomic<float>* factor;
    std::atomic<float>* interval;

    InputPort freqInput { PortType::frequency };
    OutputPort output { PortType::frequency, *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyScaleProcessor)
};

} // namespace smolfm
