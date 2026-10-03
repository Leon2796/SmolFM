/*
    VelocityScaleProcessor implementation.
*/

#include "VelocityScaleProcessor.h"

#include <cmath>

namespace smolfm
{

VelocityScaleProcessor::VelocityScaleProcessor (std::atomic<float>* curveParameter,
                                                std::atomic<float>* scaleParameter)
    : Processor (ProcessorRole::generic),
      curve (curveParameter),
      scale (scaleParameter)
{
    // An unwired velocity input reads the velocity-domain neutral (1.0), the
    // same contract as every vel consumer's default: the node stays inert,
    // never silent.
    velInput.setDefaultValue (1.0f);
}

void VelocityScaleProcessor::prepare (double /*newSampleRate*/)
{
    // Stateless: velocity in, velocity out.  No sample-rate dependent state.
}

void VelocityScaleProcessor::startNote()
{
    // No state to reset.
}

StereoFrame VelocityScaleProcessor::processSample()
{
    const StereoFrame inputVel = velInput.getSample();

    // Curve choice: read the raw choice value with round, mirroring the
    // other enum-parameters; the helper clamps bad indices to linear.
    const int curveIndex = curve != nullptr
        ? static_cast<int> (std::round (curve->load()))
        : 0;

    const float scaleValue = scale != nullptr
        ? juce::jlimit (0.25f, 4.0f, scale->load())
        : 1.0f;

    const float scaledL = juce::jlimit (0.0f, 1.0f,
        vscCurveFromIndex (curveIndex, inputVel.l) * scaleValue);
    const float scaledR = juce::jlimit (0.0f, 1.0f,
        vscCurveFromIndex (curveIndex, inputVel.r) * scaleValue);

    const StereoFrame result { scaledL, scaledR };
    output.setSample (result);
    return result;
}

} // namespace smolfm