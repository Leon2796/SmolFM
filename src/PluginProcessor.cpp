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
        voiceParameters.oscWaveform [static_cast<size_t> (i)] = parameters.getRawParameterValue ("osc" + num + "Waveform");
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFmAmounts; ++i)
    {
        const juce::String num (i);
        voiceParameters.fmAmount[static_cast<size_t> (i)]     = parameters.getRawParameterValue ("fmAmount" + num);
    }

    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFrequencyScales; ++i)
    {
        const juce::String num (i);
        voiceParameters.freqScaleFactor[static_cast<size_t> (i)] = parameters.getRawParameterValue ("fscale" + num + "Factor");
    }

    voiceParameters.attack  = parameters.getRawParameterValue ("attack");
    voiceParameters.decay   = parameters.getRawParameterValue ("decay");
    voiceParameters.sustain = parameters.getRawParameterValue ("sustain");
    voiceParameters.release = parameters.getRawParameterValue ("release");

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

    // A synthesizer has no audio input, so start with a clean output buffer.
    buffer.clear();

    // Let JUCE handle incoming MIDI events and render all active voices.
    // Voices add their samples into the buffer, which is why we cleared it first.
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
    // Save the entire APVTS state as XML.  This includes all slider values,
    // waveform choices and ADSR settings, which is everything the host needs
    // to restore the plugin later.
    auto state = parameters.copyState();
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
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
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
            juce::StringArray { "Sine", "Saw", "Square", "Triangle" },
            static_cast<int> (smolfm::Waveform::sine)));
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
    // 1.0 is the transparent default: f_out = f_in.  The UI range caps the
    // factor at 10.
    for (int i = 0; i < smolfm::GraphNodeRegistry::maxFrequencyScales; ++i)
    {
        const juce::String num (i);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            "fscale" + num + "Factor", "Frequency Scale " + num + " Factor",
            juce::NormalisableRange<float> (0.0f, 10.0f), 1.0f));
    }

    // -- ADSR (single) ------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "attack", "Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.01f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "decay", "Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.2f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "sustain", "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "release", "Release",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.5f), 0.5f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
