/*
    WaveshaperProcessor distorts a signal through a selectable transfer
    function.

    Ports:
        - in            (signal): incoming audio.
        - drive_mod_in  (signal): optional multiplier on the drive.  The
          connected value's stereo mean is clamped to [-1, +1] and the
          effective drive becomes drive * (1 + m), clamped to 0..1.
          Unconnected: the static drive runs as-is.
        - vel          (velocity): optional per-note velocity input (0-1).
          Connected: the stereo mean scales the effective drive (accent
          hits push into the curve harder).  Unconnected: transparent.
        - out (signal): the shaped audio.

    Per sample (per channel — standard stereo distortion behaviour):

        y_n = f(x_n)      with f one of four static transfer functions:
          0 soft clip:  x / (1 + |x|)                (tanh-like, warm)
          1 hard clip:  clamp(x * (1 + 9*drive), -1, 1)   (aggressive)
          2 fold:       foldback of x * (1 + 19*drive)    (metallic)
          3 quantize:   round(x * L) / L                  (stepped, lo-fi)
                        L = clamp(round(64 * (1 - drive)), 2, 64)

    drive scales the input before the curve so the transfer function can be
    pushed harder (shapes 0-2); on quantize it sets the grid coarseness:
    drive=0 keeps 64 levels per unit (error <= 1/128, near-transparent),
    drive=1 collapses to 2 levels (sign steps).  shape=0 with drive=0 is
    near-transparent (identity at small amplitudes).
*/

#pragma once

#include "ProcessorPort.h"

#include "../EffectiveRangeTelemetry.h"      // NeedleWindowTelemetry

namespace smolfm
{

/**
    Waveshaper node with four selectable transfer functions.
*/
class WaveshaperProcessor final : public Processor
{
public:
    /**
        Create the waveshaper node.

        @param driveParameter atomic pointer to the drive amount (0–1 in the
                              UI; scales the input into the curve)
        @param shapeParameter atomic pointer to the shape index (0–3: soft,
                              hard, fold, quantize)
    */
    WaveshaperProcessor (std::atomic<float>* driveParameter,
                         std::atomic<float>* shapeParameter);

    void prepare (double newSampleRate) override;
    void startNote() override;
    StereoFrame processSample() override;

    InputPort& getInput() noexcept   { return input; }
    InputPort& getDriveModInput() noexcept { return driveMod; }
    InputPort& getVelInput() noexcept { return velIn; }
    OutputPort& getOutput() noexcept { return output; }

    /** Effective (modulated) drive band currently rendered, for the UI's
        ghost needle: min/max of drive * (1 + m) over the telemetry window.
        Display-only telemetry, thread-safe. */
    juce::Range<float> getEffectiveDriveRange() const noexcept
    {
        return uiEffectiveDrive.read();
    }

private:
    std::atomic<float>* drive;
    std::atomic<float>* shape;

    InputPort input  { PortType::signal };
    InputPort driveMod { PortType::signal };
    InputPort velIn     { PortType::velocity };
    OutputPort output { PortType::signal, *this };

    // Telemetry for the UI's mod indicator (ghost needle): min/max of the
    // effective drive after drive_mod_in (velocity is sound-only, never
    // needle data) over the needle window — a per-sample overwrite would
    // alias against the overlay's 30 Hz poll (see NeedleWindowTelemetry).
    NeedleWindowTelemetry uiEffectiveDrive;
};

} // namespace smolfm