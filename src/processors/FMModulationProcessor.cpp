/*
    FMModulationProcessor implementation.
*/

#include "FMModulationProcessor.h"

namespace smolfm
{

FMModulationProcessor::FMModulationProcessor (std::atomic<float>* frequency,
                                              std::atomic<float>* waveform,
                                              std::atomic<float>* amount)
    : Processor (ProcessorRole::fmModulator),
      carrierFrequency (frequency),
      carrierWaveform (waveform),
      fmAmount (amount),
      modulatorInput (PortType::signal),
      carrierNoteInput (PortType::signal),
      output (PortType::signal, *this)
{
}

void FMModulationProcessor::prepare (double newSampleRate)
{
    carrier.prepare (newSampleRate);
}

void FMModulationProcessor::startNote()
{
    carrier.resetPhase();
}

float FMModulationProcessor::processSample()
{
    float freq = carrierFrequency->load();

    if (carrierNoteInput.isConnected())
        freq = carrierNoteInput.getSample();

    if (freq <= 0.0f)
        freq = 440.0f;

    carrier.setFrequency (freq);
    carrier.setWaveform (waveformFromIndex (static_cast<int> (std::round (carrierWaveform->load()))));

    float modulatorSample = modulatorInput.getSample();
    float phaseOffset = modulatorSample * fmAmount->load();

    float sample = carrier.getNextSample (phaseOffset);
    output.setSample (sample);

    return sample;
}

} // namespace smolfm
