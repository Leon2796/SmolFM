/*
    PluginProcessor.cpp wires MIDI, synthesis and parameters together.

    The most important parts are:

    1. The constructor builds the AudioProcessorValueTreeState parameter layout.
    2. prepareToPlay() passes the sample rate to the Synthesiser and each voice.
    3. processBlock() clears the output buffer and asks the Synthesiser to render.
    4. getStateInformation() / setStateInformation() save and restore APVTS state.
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
      parameters (*this, nullptr, juce::Identifier ("SmolFMParameters"), createParameterLayout())
{
    // Add the single sound definition that every voice can play.
    synth.addSound (new smolfm::SynthSound());

    // Create eight polyphonic voices.  Each voice receives a lightweight view
    // of the parameters so it can read the current slider values on the audio
    // thread without locks or allocations.
    constexpr int numberOfVoices = 8;

    smolfm::SynthVoiceParameters voiceParameters {};

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxOscillators; ++i)
    {
        const juce::String num (i);
        voiceParameters.oscWaveform  [static_cast<size_t> (i)] = parameters.getRawParameterValue ("osc" + num + "Waveform");
        voiceParameters.oscMode      [static_cast<size_t> (i)] = parameters.getRawParameterValue ("osc" + num + "Mode");
        voiceParameters.oscStaticFreq[static_cast<size_t> (i)] = parameters.getRawParameterValue ("osc" + num + "StaticFreq");
        voiceParameters.oscLfoRate   [static_cast<size_t> (i)] = parameters.getRawParameterValue ("osc" + num + "LfoRate");
        voiceParameters.oscBypass    [static_cast<size_t> (i)] = parameters.getRawParameterValue ("osc" + num + "Bypass");
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFmAmounts; ++i)
    {
        const juce::String num (i);
        voiceParameters.fmAmount[static_cast<size_t> (i)]     = parameters.getRawParameterValue ("fmAmount" + num);
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFrequencyScales; ++i)
    {
        const juce::String num (i);
        voiceParameters.freqScaleFactor   [static_cast<size_t> (i)] = parameters.getRawParameterValue ("fscale" + num + "Factor");
        voiceParameters.freqScaleInterval [static_cast<size_t> (i)] = parameters.getRawParameterValue ("fscale" + num + "Interval");
    }

        for (int i = 0; i < smolfm::GraphNodeRegistry::maxVelocityScales; ++i)
        {
            const juce::String num (i);
            voiceParameters.velScaleCurve [static_cast<size_t> (i)] = parameters.getRawParameterValue ("vsc" + num + "Curve");
            voiceParameters.velScaleScale [static_cast<size_t> (i)] = parameters.getRawParameterValue ("vsc" + num + "Scale");
        }

        for (int i = 0; i < smolfm::GraphNodeRegistry::maxAdsr; ++i)
    {
        const juce::String num (i);

        voiceParameters.adsrAttack [static_cast<size_t> (i)] = parameters.getRawParameterValue ("adsr" + num + 
"Attack");
        voiceParameters.adsrDecay  [static_cast<size_t> (i)] = parameters.getRawParameterValue ("adsr" + num + 
"Decay");
        voiceParameters.adsrSustain[static_cast<size_t> (i)] = parameters.getRawParameterValue ("adsr" + num + 
"Sustain");
        voiceParameters.adsrRelease[static_cast<size_t> (i)] = parameters.getRawParameterValue ("adsr" + num + 
"Release");
        voiceParameters.adsrBypass  [static_cast<size_t> (i)] = parameters.getRawParameterValue ("adsr" + num + "Bypass");
    }

        for (int i = 0; i < smolfm::GraphNodeRegistry::maxFAdsr; ++i)
    {
        const juce::String num (i);

        voiceParameters.fAdsrAttack [static_cast<size_t> (i)] = parameters.getRawParameterValue ("fadsr" + num + "Attack");
        voiceParameters.fAdsrDecay  [static_cast<size_t> (i)] = parameters.getRawParameterValue ("fadsr" + num + "Decay");
        voiceParameters.fAdsrSustain[static_cast<size_t> (i)] = parameters.getRawParameterValue ("fadsr" + num + "Sustain");
        voiceParameters.fAdsrRelease[static_cast<size_t> (i)] = parameters.getRawParameterValue ("fadsr" + num + "Release");
        voiceParameters.fAdsrUp     [static_cast<size_t> (i)] = parameters.getRawParameterValue ("fadsr" + num + "Up");
        voiceParameters.fAdsrDown   [static_cast<size_t> (i)] = parameters.getRawParameterValue ("fadsr" + num + "Down");
        voiceParameters.fAdsrBypass [static_cast<size_t> (i)] = parameters.getRawParameterValue ("fadsr" + num + "Bypass");
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxGains; ++i)
    {
        const juce::String num (i);
        voiceParameters.gainFactor[static_cast<size_t> (i)] = parameters.getRawParameterValue ("gain" + num + "Factor");
        voiceParameters.gainBypass[static_cast<size_t> (i)] = parameters.getRawParameterValue ("gain" + num + "Bypass");
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxWaveshapers; ++i)
    {
        const juce::String num (i);
        voiceParameters.shaperDrive[static_cast<size_t> (i)] = parameters.getRawParameterValue ("shape" + num + "Drive");
        voiceParameters.shaperShape[static_cast<size_t> (i)] = parameters.getRawParameterValue ("shape" + num + "Shape");
        voiceParameters.shaperBypass[static_cast<size_t> (i)] = parameters.getRawParameterValue ("shape" + num + "Bypass");
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFilters; ++i)
    {
        const juce::String num (i);
        voiceParameters.filterCutoff   [static_cast<size_t> (i)] = parameters.getRawParameterValue ("filter" + num + "Cutoff");
        voiceParameters.filterResonance[static_cast<size_t> (i)] = parameters.getRawParameterValue ("filter" + num + "Resonance");
        voiceParameters.filterMode     [static_cast<size_t> (i)] = parameters.getRawParameterValue ("filter" + num + "Mode");
        voiceParameters.filterBypass   [static_cast<size_t> (i)] = parameters.getRawParameterValue ("filter" + num + "Bypass");
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxAmModulators; ++i)
    {
        const juce::String num (i);
        voiceParameters.amAmount[static_cast<size_t> (i)] = parameters.getRawParameterValue ("am" + num + "Amount");
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxDelays; ++i)
    {
        const juce::String num (i);
        voiceParameters.delayTimeMs  [static_cast<size_t> (i)] = parameters.getRawParameterValue ("delay" + num + "TimeMs");
        voiceParameters.delayFeedback[static_cast<size_t> (i)] = parameters.getRawParameterValue ("delay" + num + "Feedback");
        voiceParameters.delayMix     [static_cast<size_t> (i)] = parameters.getRawParameterValue ("delay" + num + "Mix");
        voiceParameters.delaySync    [static_cast<size_t> (i)] = parameters.getRawParameterValue ("delay" + num + "SyncMode");
        voiceParameters.delayDivision[static_cast<size_t> (i)] = parameters.getRawParameterValue ("delay" + num + "Division");
        voiceParameters.delayBypass  [static_cast<size_t> (i)] = parameters.getRawParameterValue ("delay" + num + "Bypass");
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxSampleHolds; ++i)
    {
        const juce::String num (i);
        voiceParameters.holdRate     [static_cast<size_t> (i)] = parameters.getRawParameterValue ("hold" + num + "Rate");
        voiceParameters.holdSync     [static_cast<size_t> (i)] = parameters.getRawParameterValue ("hold" + num + "SyncMode");
        voiceParameters.holdDivision [static_cast<size_t> (i)] = parameters.getRawParameterValue ("hold" + num + "Division");
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxResonators; ++i)
    {
        const juce::String num (i);
        voiceParameters.resonatorTune        [static_cast<size_t> (i)] = parameters.getRawParameterValue ("resonator" + num + "Tune");
        voiceParameters.resonatorQ           [static_cast<size_t> (i)] = parameters.getRawParameterValue ("resonator" + num + "Q");
        voiceParameters.resonatorDecay       [static_cast<size_t> (i)] = parameters.getRawParameterValue ("resonator" + num + "Decay");
        voiceParameters.resonatorVelSpread   [static_cast<size_t> (i)] = parameters.getRawParameterValue ("resonator" + num + "VelSpread");
        voiceParameters.resonatorStrikeLevel [static_cast<size_t> (i)] = parameters.getRawParameterValue ("resonator" + num + "StrikeLevel");
        voiceParameters.resonatorStrikeTime  [static_cast<size_t> (i)] = parameters.getRawParameterValue ("resonator" + num + "StrikeTime");
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFeedbacks; ++i)
    {
        const juce::String num (i);
        voiceParameters.feedbackAmount[static_cast<size_t> (i)] = parameters.getRawParameterValue ("feedback" + num + "Amount");
    }

    voiceParameters.masterLevel = parameters.getRawParameterValue ("masterLevel");

    for (int i = 0; i < numberOfVoices; ++i)
        synth.addVoice (new smolfm::SynthVoice (voiceParameters));

    // No default graph: voices start with every port disconnected.  The
    // editor canvas is empty until the user adds nodes or imports a patch.
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // JUCE needs to know the current sample rate so it can pass it on to each
    // SynthesiserVoice.  The voices then prepare their oscillators and ADSR.
    juce::ignoreUnused (samplesPerBlock);
    synth.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<smolfm::SynthVoice*> (synth.getVoice (i)))
            voice->prepare (sampleRate);
    }
}

void AudioPluginAudioProcessor::releaseResources()
{
    // Nothing to release in this minimal synthesizer.  Real-time resources
    // are owned directly by the voices and are cleaned up automatically.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Read host tempo for tempo-synced processors; fall back to 120 BPM.
    double bpm = 120.0;
    if (auto* playhead = getPlayHead())
    {
        if (const auto position = playhead->getPosition())
        {
            if (const auto tempo = position->getBpm())
                bpm = *tempo;
        }
    }

    // Push the tempo into every voice so delay processors can read it per sample.
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<smolfm::SynthVoice*> (synth.getVoice (i)))
            voice->setHostTempo (bpm);

    // A synthesizer has no audio input, so start with a clean output buffer.
    buffer.clear();

    // Let JUCE handle incoming MIDI events and render all active voices.
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save the entire APVTS state as XML, plus the patch browser directory.
    auto state = parameters.copyState();
    state.setProperty ("patchDirectory", patchDirectory.getFullPathName(), nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore the saved APVTS state.  Once the value tree is updated, the
    // editor attachments will automatically reflect the restored values.
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
            patchDirectory = juce::File (xmlState->getStringAttribute ("patchDirectory"));
        }
}

//==============================================================================
juce::AudioProcessorValueTreeState& AudioPluginAudioProcessor::getParameters()
{
    return parameters;
}

void AudioPluginAudioProcessor::applyConnectionPatch (const smolfm::ConnectionPatch& patch)
{
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<smolfm::SynthVoice*> (synth.getVoice (i)))
            voice->applyConnectionPatch (patch);
}

juce::File AudioPluginAudioProcessor::getPatchDirectory() const
{
    return patchDirectory;
}

void AudioPluginAudioProcessor::setPatchDirectory (const juce::File& dir)
{
    patchDirectory = dir.isDirectory() ? dir : juce::File();
}

float AudioPluginAudioProcessor::getMasterPeakLevel() const
{
    float peak = 0.0f;
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<smolfm::SynthVoice*> (synth.getVoice (i)))
            peak = juce::jmax (peak, voice->getMasterPeakLevel());
    return peak;
}

juce::Range<float> AudioPluginAudioProcessor::getEffectiveControlRange (const smolfm::NodeType type,
                                                                        const int instanceIndex,
                                                                        const int controlIndex) const
{
    // Mirrors getMasterPeakLevel(): poll every voice, skip non-SynthVoices.
    // Idle voices are excluded here — they stopped rendering, so the value
    // their processors published last is stale (plan 1.4, "Idle-Zerfall").
    //
    // Range protocol (see SynthVoice.h): voice getters return a degenerate
    // (v, v) range for real data.  juce::Range::isEmpty() is TRUE for
    // degenerate ranges — filtering on it discarded every voice, so the
    // aggregator never produced data and the overlay never drew.  The
    // no-data signal is a NaN length (this juce::Range constructor clamps
    // a reversed pair, so a negative length cannot exist), via
    // effectiveRangeHasData().
    bool anyData = false;
    float lowest = 0.0f;
    float highest = 0.0f;

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        smolfm::SynthVoice* voice = dynamic_cast<smolfm::SynthVoice*> (synth.getVoice (i));
        if (voice == nullptr || ! voice->isVoiceActive())
            continue;

        const juce::Range<float> range = voice->getEffectiveControlRange (type, instanceIndex, controlIndex);
        if (! smolfm::effectiveRangeHasData (range))
            continue;

        lowest  = anyData ? juce::jmin (lowest, range.getStart())  : range.getStart();
        highest = anyData ? juce::jmax (highest, range.getEnd()) : range.getEnd();
        anyData = true;
    }

    // Band policy: [min over voices, max over voices] — one note playing
    // collapses the band back into a single needle position (v, v).
    return anyData ? juce::Range<float> (lowest, highest) : smolfm::noEffectiveData();
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    // Parameter IDs must stay stable forever.  They identify parameters to the
    // host for automation and presets.
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // -- Oscillator pool ---------------------------------------------------
    // 8 oscillator waveform parameters.  Frequency comes exclusively from the
    // note_in port; there is no fallback slider value.
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxOscillators; ++i)
    {
        const juce::String num (i);

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            "osc" + num + "Waveform", "Oscillator " + num + " Waveform",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Noise", "Perlin Noise", "Simplex Noise" },
            static_cast<int> (smolfm::Waveform::sine)));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            "osc" + num + "Mode", "Oscillator " + num + " Mode",
            juce::StringArray { "Pitch", "Static", "LFO" }, 0));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "osc" + num + "StaticFreq", "Oscillator " + num + " Static Freq",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 440.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "osc" + num + "LfoRate", "Oscillator " + num + " LFO Rate",
            juce::NormalisableRange<float> (0.01f, 50.0f, 0.01f, 0.35f), 1.0f));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            "osc" + num + "Bypass", "Oscillator " + num + " Bypass", false));
    }

    // -- FM pool ------------------------------------------------------------
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFmAmounts; ++i)
    {
        const juce::String num (i);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "fmAmount" + num, "FM Amount " + num,
            juce::NormalisableRange<float> (0.0f, 10.0f), 0.0f));
    }

    // -- Frequency scale pool ------------------------------------------------
    // Two multiplicative parameters per instance: the free-range factor knob
    // (0-10,  default 1) and the musical interval dropdown, whose names map
    // onto exact 12-TET ratios (see fscaleIntervalRatioFromIndex in
    // FrequencyScaleProcessor.h; "None" = no shift).  Defaults are fully
    // transparent: f_out = f_in.  The UI range caps the factor at 10.
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFrequencyScales; ++i)
    {
        const juce::String num (i);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "fscale" + num + "Factor", "Frequency Scale " + num + " Factor",
            juce::NormalisableRange<float> (0.0f, 10.0f), 1.0f));

        // Choice order must match fscaleIntervalNames() (ISmolFmParser.h)
        // and the FrequencyScaleComponent dropdown item list.
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            "fscale" + num + "Interval", "Frequency Scale " + num + " Interval",
            juce::StringArray { "None", "Octave up", "Octave down", "Fifth up",
                                "Fifth down", "Fourth up", "Fourth down",
                                "Minor third up", "Major third up",
                                "Semitone up", "Semitone down" }, 0));
    }

    // Velocity Scale: a velocity-domain rescaler (curve x scale).  Defaults
    // are transparent: v_out = v_in (curve linear, scale 1.0).  Exists so
    // analog-hardware sims can bend the MIDI velocity before it reaches the
    // gain/shape/resonator/fm vel inputs (the TR-808's accent is a trigger
    // voltage law, not raw linear MIDI velocity).
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxVelocityScales; ++i)
    {
        const juce::String num (i);
        jassert (smolfm::GraphNodeRegistry::velocityScaleCurveParameterIdFor ("vsc" + num)
                 == "vsc" + num + "Curve");

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "vsc" + num + "Scale", "Velocity Scale " + num + " Scale",
            juce::NormalisableRange<float> (0.25f, 4.0f, 0.0f, 0.5f), 1.0f));

        // Choice order must match vscCurveNames() (ISmolFmParser.h)
        // and the VelocityScaleComponent dropdown item list.
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            "vsc" + num + "Curve", "Velocity Scale " + num + " Curve",
            juce::StringArray { "Linear", "Sqrt", "Square" }, 0));
    }

    // -- ADSR pool ----------------------------------------------------------
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxAdsr; ++i)
    {
        const juce::String num (i);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "adsr" + num + "Attack", "ADSR " + num + " Attack",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.01f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "adsr" + num + "Decay", "ADSR " + num + " Decay",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.2f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "adsr" + num + "Sustain", "ADSR " + num + " Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "adsr" + num + "Release", "ADSR " + num + " Release",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.5f), 0.5f));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            "adsr" + num + "Bypass", "ADSR " + num + " Bypass", false));
    }

    // -- F-ADSR pitch-envelope pool ----------------------------------------
    // Six parameters per instance: ADSR timing plus the up/down scaling
    // factors applied to the input frequency at envelope E = 1 / E = 0.
    // Both factors default to 1.0 (transparent pass-through).
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFAdsr; ++i)
    {
        const juce::String num (i);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "fadsr" + num + "Attack", "F-ADSR " + num + " Attack",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.01f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "fadsr" + num + "Decay", "F-ADSR " + num + " Decay",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.2f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "fadsr" + num + "Sustain", "F-ADSR " + num + " Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "fadsr" + num + "Release", "F-ADSR " + num + " Release",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.5f), 0.5f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "fadsr" + num + "Up", "F-ADSR " + num + " Up Factor",
            juce::NormalisableRange<float> (0.0f, 10.0f), 1.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "fadsr" + num + "Down", "F-ADSR " + num + " Down Factor",
            juce::NormalisableRange<float> (0.0f, 10.0f), 1.0f));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            "fadsr" + num + "Bypass", "F-ADSR " + num + " Bypass", false));
    }

    // -- Gain pool ----------------------------------------------------------
    // One transparent-by-default gain factor per instance (0-10, 1.0 = unity).
    // The knob is bipolar: unity sits at the centre of the travel, the left
    // half covers attenuation 0..1 linearly (the usual mix case, e.g. ducking
    // a noise layer to 0.12) and the right half maps 1..10 on a soft
    // quadratic curve (2.0 at two thirds of the travel) so loud values keep
    // fine resolution too.  The raw factor stays a plain 0..10 value:
    // .smolfm files store 0.12 and read back 0.12 with no UI mapping applied.
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxGains; ++i)
    {
        const juce::String num (i);

        auto gainTo0To1 = [] (float, float, float gain)
        {
            if (gain <= 1.0f)
                return gain * 0.5f;

            return 0.5f + 0.5f * std::sqrt ((gain - 1.0f) / 9.0f);
        };
        auto gainFrom0To1 = [] (float, float, float proportion)
        {
            if (proportion <= 0.5f)
                return proportion * 2.0f;

            const auto x = (proportion - 0.5f) * 2.0f;
            return 1.0f + 9.0f * x * x;
        };

        // The piecewise conversion functions ride on the parameter's
        // NormalisableRange via its (start, end, from0to1, to0to1, snap)
        // constructor; the SliderAttachment forwards them to the knob.
        juce::NormalisableRange<float> gainRange (0.0f, 10.0f,
                                                  gainFrom0To1, gainTo0To1);

        // ID-drift guard: the registry template and this layout must produce
        // the same id — a drift leaves the knob unattached (shows 0, no audio
        // effect) and the .smolfm loader silently drops the factor value.
        jassert (smolfm::GraphNodeRegistry::gainParameterIdFor ("gain" + num)
                 == "gain" + num + "Factor");

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "gain" + num + "Factor", "Gain " + num + " Factor",
            gainRange, 1.0f));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            "gain" + num + "Bypass", "Gain " + num + " Bypass", false));
    }

    // -- Waveshaper pool ----------------------------------------------------
    // Two parameters per instance: drive (0-1) and shape (choice: soft, hard,
    // fold, quantize).  drive=0 with soft shape is near-transparent; on
    // quantize, drive sets the grid coarseness (64 levels down to 2).
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxWaveshapers; ++i)
    {
        const juce::String num (i);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "shape" + num + "Drive", "Waveshaper " + num + " Drive",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            "shape" + num + "Shape", "Waveshaper " + num + " Shape",
            juce::StringArray { "Soft", "Hard", "Fold", "Quantize" }, 0));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            "shape" + num + "Bypass", "Waveshaper " + num + " Bypass", false));
    }

    // -- Filter pool --------------------------------------------------------
    // Three parameters per instance: static cutoff (20-20000 Hz, log),
    // resonance (0-1) and mode (LP, BP, HP, Notch).  cutoff_mod_in overrides
    // the static cutoff when connected.
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFilters; ++i)
    {
        const juce::String num (i);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "filter" + num + "Cutoff", "Filter " + num + " Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 1000.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "filter" + num + "Resonance", "Filter " + num + " Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            "filter" + num + "Mode", "Filter " + num + " Mode",
            juce::StringArray { "LP", "BP", "HP", "Notch", "AP" }, 0));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            "filter" + num + "Bypass", "Filter " + num + " Bypass", false));
    }

        // -- AM modulator pool ------------------------------------------------------
    // Depth 0 = dry passthrough, 1 = full modulation (bipolar modulator spans
    // gain 0..2).
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxAmModulators; ++i)
    {
        const juce::String num (i);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "am" + num + "Amount", "AM " + num + " Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    }

    // -- Delay pool ------------------------------------------------------------
    // SyncMode = bool (free ms vs host-BPM); Division = choice index 0..3 (1/2, 1/4, 1/8, 1/16).
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxDelays; ++i)
    {
        const juce::String num (i);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "delay" + num + "TimeMs", "Delay " + num + " Time",
            juce::NormalisableRange<float> (1.0f, 2000.0f, 0.1f, 0.5f), 250.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "delay" + num + "Feedback", "Delay " + num + " Feedback",
            juce::NormalisableRange<float> (0.0f, 0.95f), 0.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "delay" + num + "Mix", "Delay " + num + " Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            "delay" + num + "SyncMode", "Delay " + num + " Sync", false));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            "delay" + num + "Division", "Delay " + num + " Division",
            juce::StringArray { "1/2", "1/4", "1/8", "1/16" }, 1));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            "delay" + num + "Bypass", "Delay " + num + " Bypass", false));
    }

    // -- Sample-hold pool ------------------------------------------------------
    // SyncMode = bool (free Hz vs host-BPM); Division = choice index 0..3
    // (1/2, 1/4, 1/8, 1/16) picks the hold grid when Sync is on.
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxSampleHolds; ++i)
    {
        const juce::String num (i);

        // ID-drift guard: the registry template and this layout must produce
        // the same id — a drift leaves the knob unattached (resonator precedent).
        jassert (smolfm::GraphNodeRegistry::holdRateParameterIdFor ("hold" + num) == "hold" + num + "Rate");

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "hold" + num + "Rate", "Hold " + num + " Rate",
            juce::NormalisableRange<float> (0.5f, 20000.0f, 0.01f, 0.25f), 1000.0f));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            "hold" + num + "SyncMode", "Hold " + num + " Sync", false));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            "hold" + num + "Division", "Hold " + num + " Division",
            juce::StringArray { "1/2", "1/4", "1/8", "1/16" }, 1));
    }

    // -- Resonator pool --------------------------------------------------------
    // Six parameters per instance: static tune (20-8000 Hz, log), Q (0-1),
    // decay (t60 in seconds, 0.01-10), the vel spread (0.25-4, 1 =
    // neutral) and the internal strike pair (strikeLevel 0-2 and
    // strikeTime 0-0.2 s, both defaulting to 0 = strike off = the
    // byte-identical old behaviour; plan Phase 2, §9.2).  The tune port
    // overrides the static value when connected; vel scales the decay
    // time per the plan's pull-to-neutral pattern
    // t_eff = t * (1 + v * (spread - 1)) — accented hits ring longer.
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxResonators; ++i)
    {
        const juce::String num (i);

        // ID-drift guard: the registry template and this layout must produce
        // the same id — a drift leaves the knob unattached and the .smolfm
        // loader silently drops the value.
        jassert (smolfm::GraphNodeRegistry::resonatorTuneParameterIdFor  ("resonator" + num) == "resonator" + num + "Tune");
        jassert (smolfm::GraphNodeRegistry::resonatorDecayParameterIdFor ("resonator" + num) == "resonator" + num + "Decay");
        jassert (smolfm::GraphNodeRegistry::resonatorStrikeLevelParameterIdFor ("resonator" + num) == "resonator" + num + "StrikeLevel");
        jassert (smolfm::GraphNodeRegistry::resonatorStrikeTimeParameterIdFor ("resonator" + num) == "resonator" + num + "StrikeTime");

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "resonator" + num + "Tune", "Resonator " + num + " Tune",
            juce::NormalisableRange<float> (20.0f, 8000.0f, 0.1f, 0.3f), 100.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "resonator" + num + "Q", "Resonator " + num + " Q",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "resonator" + num + "Decay", "Resonator " + num + " Decay",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.001f, 0.5f), 0.3f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "resonator" + num + "VelSpread", "Resonator " + num + " Vel Spread",
            juce::NormalisableRange<float> (0.25f, 4.0f), 1.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "resonator" + num + "StrikeLevel", "Resonator " + num + " Strike Level",
            juce::NormalisableRange<float> (0.0f, 2.0f), 0.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "resonator" + num + "StrikeTime", "Resonator " + num + " Strike Time",
            juce::NormalisableRange<float> (0.0f, 0.2f, 0.0005f), 0.0f));
    }

    // -- Feedback pool (bounded cycle breakers) ---------------------------------
    // One per-pass damping per instance (0-0.95): out = in + amount * fb_in.
    // The cycle's 1-sample delay comes from the render order (the node reads
    // its fb_in producer's previous-tick frame).
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFeedbacks; ++i)
    {
        const juce::String num (i);

        jassert (smolfm::GraphNodeRegistry::amountParameterIdFor ("feedback" + num)
                 == "feedback" + num + "Amount");

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "feedback" + num + "Amount", "Feedback " + num + " Amount",
            juce::NormalisableRange<float> (0.0f, 0.95f), 0.3f));
    }

    // -- Master output --------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "masterLevel", "Master Level",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
