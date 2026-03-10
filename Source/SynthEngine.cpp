#include "SynthEngine.h"

#include <algorithm>

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

    pulseWidthSmoothed.reset (sampleRate, parameterRampSeconds);
    pulseWidthSmoothed.setCurrentAndTargetValue (pulseWidth);

    cutoffSmoothed.reset (sampleRate, parameterRampSeconds);
    cutoffSmoothed.setCurrentAndTargetValue (baseCutoffHz);

    resonanceSmoothed.reset (sampleRate, parameterRampSeconds);
    resonanceSmoothed.setCurrentAndTargetValue (filterResonance);

    filterEnvDepthSmoothed.reset (sampleRate, parameterRampSeconds);
    filterEnvDepthSmoothed.setCurrentAndTargetValue (filterEnvDepth);

    gainSmoothed.reset (sampleRate, parameterRampSeconds);
    gainSmoothed.setCurrentAndTargetValue (gain);
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

    // Phase 2: Apply filter and gain with per-sample smoothing
    auto* channelData = buffer.getWritePointer (0);
    float* monoChannel[] { channelData };

    for (int sample = 0; sample < numSamples; ++sample)
    {
        ladderFilter.setResonance (resonanceSmoothed.getNextValue());
        ladderFilter.setCutoffFrequencyHz (getSmoothedFilterCutoff (filterAdsr.getNextSample()));

        juce::dsp::AudioBlock<float> sampleBlock (monoChannel, 1, static_cast<size_t> (sample), 1);
        juce::dsp::ProcessContextReplacing<float> context (sampleBlock);
        ladderFilter.process (context);

        channelData[sample] *= gainSmoothed.getNextValue();
    }

    // Copy mono to remaining channels
    for (int ch = 1; ch < numChannels; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

float SynthEngine::getSmoothedFilterCutoff (float envValue)
{
    // Modulate in octaves: baseCutoff * 2^(envValue * depth * 10)
    auto modOctaves = envValue * filterEnvDepthSmoothed.getNextValue() * 10.0f;
    auto modulated = cutoffSmoothed.getNextValue() * std::pow (2.0f, modOctaves);
    return juce::jlimit (20.0f, 20000.0f, modulated);
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
            raw = (phase < static_cast<double> (pulseWidthSmoothed.getNextValue())) ? 1.0f : -1.0f;
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

void SynthEngine::startNote (int midiNote, float velocity, bool retriggerEnvelope)
{
    currentMidiNote = midiNote;
    currentVelocity = velocity;
    phaseIncrement = juce::MidiMessage::getMidiNoteInHertz (currentMidiNote) / sampleRate;

    if (retriggerEnvelope)
    {
        ampAdsr.setParameters (ampAdsrParams);
        ampAdsr.noteOn();

        filterAdsr.setParameters (filterAdsrParams);
        filterAdsr.noteOn();
    }
}

void SynthEngine::releaseCurrentNote()
{
    ampAdsr.noteOff();
    filterAdsr.noteOff();
}

void SynthEngine::removeHeldNote (int midiNote)
{
    heldNotes.erase (std::remove_if (heldNotes.begin(), heldNotes.end(),
                                     [midiNote] (const HeldNote& note)
                                     {
                                         return note.noteNumber == midiNote;
                                     }),
                     heldNotes.end());
}

void SynthEngine::handleMidiEvent (const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        auto noteNumber = msg.getNoteNumber();
        removeHeldNote (noteNumber);
        heldNotes.push_back ({ noteNumber, msg.getFloatVelocity() });
        startNote (noteNumber, msg.getFloatVelocity(), true);
    }
    else if (msg.isNoteOff())
    {
        auto releasedNote = msg.getNoteNumber();
        auto wasCurrentNote = (releasedNote == currentMidiNote);
        removeHeldNote (releasedNote);

        if (heldNotes.empty())
        {
            if (wasCurrentNote)
                releaseCurrentNote();
        }
        else if (wasCurrentNote)
        {
            const auto& fallbackNote = heldNotes.back();
            startNote (fallbackNote.noteNumber, fallbackNote.velocity, false);
        }
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        heldNotes.clear();
        ampAdsr.reset();
        filterAdsr.reset();
        currentMidiNote = -1;
        currentVelocity = 0.0f;
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
    pulseWidthSmoothed.setTargetValue (pulseWidth);
}

void SynthEngine::setFilterParams (float cutoffHz, float resonance)
{
    baseCutoffHz = cutoffHz;
    filterResonance = resonance;
    cutoffSmoothed.setTargetValue (baseCutoffHz);
    resonanceSmoothed.setTargetValue (filterResonance);
}

void SynthEngine::setADSR (float attack, float decay, float sustain, float release)
{
    ampAdsrParams = { attack, decay, sustain, release };
    ampAdsr.setParameters (ampAdsrParams);
}

void SynthEngine::setFilterEnvParams (float attack, float decay, float sustain, float release)
{
    filterAdsrParams = { attack, decay, sustain, release };
    filterAdsr.setParameters (filterAdsrParams);
}

void SynthEngine::setFilterEnvDepth (float depth)
{
    filterEnvDepth = depth;
    filterEnvDepthSmoothed.setTargetValue (filterEnvDepth);
}

void SynthEngine::setGain (float gainLinear)
{
    gain = gainLinear;
    gainSmoothed.setTargetValue (gain);
}
