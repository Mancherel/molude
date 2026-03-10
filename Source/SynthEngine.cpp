#include "SynthEngine.h"

void SynthEngine::prepare (double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;
    preparedChannels = numChannels;

    ampAdsr.setSampleRate (sampleRate);
    ampAdsr.setParameters (ampAdsrParams);

    filterAdsr.setSampleRate (sampleRate);
    filterAdsr.setParameters (filterAdsrParams);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;

    ladderFilter.prepare (spec);
    ladderFilter.reset();
    ladderFilter.setMode (juce::dsp::LadderFilterMode::LPF24);
    ladderFilter.setCutoffFrequencyHz (baseCutoffHz);
    ladderFilter.setResonance (filterResonance);
}

void SynthEngine::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    buffer.clear();

    // Early out when idle
    if (midi.isEmpty() && ! ampAdsr.isActive())
        return;

    // Phase 1: Render oscillator + amp envelope into channel 0 (mono)
    int currentSample = 0;
    bool didRender = false;

    for (const auto metadata : midi)
    {
        auto message = metadata.getMessage();
        auto messagePosition = metadata.samplePosition;

        if (ampAdsr.isActive())
        {
            for (; currentSample < messagePosition; ++currentSample)
            {
                buffer.setSample (0, currentSample, generateSample());
                didRender = true;
            }
        }
        else
        {
            currentSample = messagePosition;
        }

        handleMidiEvent (message);
    }

    if (ampAdsr.isActive())
    {
        for (; currentSample < numSamples; ++currentSample)
        {
            buffer.setSample (0, currentSample, generateSample());
            didRender = true;
        }
    }

    if (! didRender)
        return;

    // Phase 2: Apply filter in sub-blocks with envelope modulation
    auto* channelData = buffer.getWritePointer (0);
    int samplesRemaining = numSamples;
    int offset = 0;

    while (samplesRemaining > 0)
    {
        int chunkSize = juce::jmin (samplesRemaining, filterUpdateInterval);

        // Advance filter envelope and update cutoff
        float envValue = filterAdsr.getNextSample();
        // Advance envelope for remaining samples in chunk (keep it in sync)
        for (int i = 1; i < chunkSize; ++i)
            filterAdsr.getNextSample();

        updateFilterCutoff (envValue);

        // Process this sub-block through the filter
        juce::dsp::AudioBlock<float> subBlock (&channelData, 1, static_cast<size_t> (offset),
                                                static_cast<size_t> (chunkSize));
        juce::dsp::ProcessContextReplacing<float> context (subBlock);
        ladderFilter.process (context);

        offset += chunkSize;
        samplesRemaining -= chunkSize;
    }

    // Apply gain
    if (gain != 1.0f)
        buffer.applyGain (0, 0, numSamples, gain);

    // Copy mono to remaining channels
    for (int ch = 1; ch < numChannels; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

void SynthEngine::updateFilterCutoff (float envValue)
{
    // Modulate in octaves: baseCutoff * 2^(envValue * depth * 10)
    float modOctaves = envValue * filterEnvDepth * 10.0f;
    float modulated = baseCutoffHz * std::pow (2.0f, modOctaves);
    modulated = juce::jlimit (20.0f, 20000.0f, modulated);
    ladderFilter.setCutoffFrequencyHz (modulated);
}

float SynthEngine::generateSample()
{
    if (currentMidiNote < 0 || ! ampAdsr.isActive())
        return 0.0f;

    float raw = 0.0f;

    switch (currentWaveform)
    {
        case Sine:
            raw = static_cast<float> (std::sin (2.0 * juce::MathConstants<double>::pi * phase));
            break;

        case Saw:
            raw = static_cast<float> (2.0 * phase - 1.0);
            break;

        case Square:
            raw = (phase < 0.5) ? 1.0f : -1.0f;
            break;

        case Triangle:
            raw = static_cast<float> (2.0 * std::abs (2.0 * phase - 1.0) - 1.0);
            break;

        case Pulse:
            raw = (phase < static_cast<double> (pulseWidth)) ? 1.0f : -1.0f;
            break;

        case Noise:
            raw = noiseRng.nextFloat() * 2.0f - 1.0f;
            break;

        default:
            break;
    }

    // Advance phase (not needed for noise, but harmless)
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    // Apply amp envelope and velocity
    return raw * ampAdsr.getNextSample() * currentVelocity;
}

void SynthEngine::handleMidiEvent (const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        currentMidiNote = msg.getNoteNumber();
        currentVelocity = msg.getFloatVelocity();
        phaseIncrement = juce::MidiMessage::getMidiNoteInHertz (currentMidiNote) / sampleRate;

        ampAdsr.setParameters (ampAdsrParams);
        ampAdsr.noteOn();

        filterAdsr.setParameters (filterAdsrParams);
        filterAdsr.noteOn();
    }
    else if (msg.isNoteOff())
    {
        if (msg.getNoteNumber() == currentMidiNote)
        {
            ampAdsr.noteOff();
            filterAdsr.noteOff();
        }
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        ampAdsr.reset();
        filterAdsr.reset();
        currentMidiNote = -1;
    }
}

void SynthEngine::setWaveform (int type)
{
    jassert (type >= 0 && type < NumWaveforms);
    currentWaveform = type;
}

void SynthEngine::setPulseWidth (float width)
{
    pulseWidth = juce::jlimit (0.05f, 0.95f, width);
}

void SynthEngine::setFilterParams (float cutoffHz, float resonance)
{
    baseCutoffHz = cutoffHz;
    filterResonance = resonance;
    ladderFilter.setResonance (resonance);
}

void SynthEngine::setADSR (float attack, float decay, float sustain, float release)
{
    ampAdsrParams = { attack, decay, sustain, release };
}

void SynthEngine::setFilterEnvParams (float attack, float decay, float sustain, float release)
{
    filterAdsrParams = { attack, decay, sustain, release };
}

void SynthEngine::setFilterEnvDepth (float depth)
{
    filterEnvDepth = depth;
}

void SynthEngine::setGain (float gainLinear)
{
    gain = gainLinear;
}
