#include "PluginProcessor.h"
#include "PluginEditor.h"

MoludeAudioProcessor::MoludeAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout MoludeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "OSC_WAVEFORM", 1 }, "Waveform",
        juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Pulse", "Noise" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "PULSE_WIDTH", 1 }, "Pulse Width",
        juce::NormalisableRange<float> (0.05f, 0.95f, 0.01f),
        0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "FILTER_CUTOFF", 1 }, "Filter Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f),
        1000.0f, "Hz"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "FILTER_RESONANCE", 1 }, "Filter Resonance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "AMP_ATTACK", 1 }, "Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f),
        0.01f, "s"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "AMP_DECAY", 1 }, "Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f),
        0.1f, "s"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "AMP_SUSTAIN", 1 }, "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.8f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "AMP_RELEASE", 1 }, "Release",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f),
        0.3f, "s"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "MASTER_GAIN", 1 }, "Gain",
        juce::NormalisableRange<float> (-60.0f, 6.0f, 0.1f),
        -6.0f, "dB"));

    // Filter envelope
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "FILT_ENV_ATTACK", 1 }, "Filt Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f),
        0.01f, "s"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "FILT_ENV_DECAY", 1 }, "Filt Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f),
        0.3f, "s"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "FILT_ENV_SUSTAIN", 1 }, "Filt Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "FILT_ENV_RELEASE", 1 }, "Filt Release",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f),
        0.3f, "s"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "FILT_ENV_DEPTH", 1 }, "Filt Env Depth",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f),
        0.5f));

    return { params.begin(), params.end() };
}

void MoludeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synthEngine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void MoludeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Push parameter values to synth engine
    synthEngine.setWaveform (static_cast<int> (apvts.getRawParameterValue ("OSC_WAVEFORM")->load()));
    synthEngine.setPulseWidth (apvts.getRawParameterValue ("PULSE_WIDTH")->load());
    synthEngine.setFilterParams (apvts.getRawParameterValue ("FILTER_CUTOFF")->load(),
                                apvts.getRawParameterValue ("FILTER_RESONANCE")->load());
    synthEngine.setADSR (apvts.getRawParameterValue ("AMP_ATTACK")->load(),
                         apvts.getRawParameterValue ("AMP_DECAY")->load(),
                         apvts.getRawParameterValue ("AMP_SUSTAIN")->load(),
                         apvts.getRawParameterValue ("AMP_RELEASE")->load());

    synthEngine.setFilterEnvParams (apvts.getRawParameterValue ("FILT_ENV_ATTACK")->load(),
                                    apvts.getRawParameterValue ("FILT_ENV_DECAY")->load(),
                                    apvts.getRawParameterValue ("FILT_ENV_SUSTAIN")->load(),
                                    apvts.getRawParameterValue ("FILT_ENV_RELEASE")->load());
    synthEngine.setFilterEnvDepth (apvts.getRawParameterValue ("FILT_ENV_DEPTH")->load());

    auto gainDb = apvts.getRawParameterValue ("MASTER_GAIN")->load();
    synthEngine.setGain (juce::Decibels::decibelsToGain (gainDb, -60.0f));

    synthEngine.processBlock (buffer, midiMessages);
}

juce::AudioProcessorEditor* MoludeAudioProcessor::createEditor()
{
    return new MoludeEditor (*this);
}

void MoludeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MoludeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MoludeAudioProcessor();
}
