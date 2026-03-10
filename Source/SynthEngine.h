#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class SynthEngine
{
public:
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    void setWaveform (int type);
    void setPulseWidth (float width);
    void setFilterParams (float cutoffHz, float resonance);
    void setADSR (float attack, float decay, float sustain, float release);
    void setFilterEnvParams (float attack, float decay, float sustain, float release);
    void setFilterEnvDepth (float depth);
    void setGain (float gainLinear);

    // Waveform indices
    enum Waveform { Sine = 0, Saw, Square, Triangle, Pulse, Noise, NumWaveforms };

private:
    float generateSample();
    void handleMidiEvent (const juce::MidiMessage& msg);
    void updateFilterCutoff (float envValue);

    // Oscillator
    double phase = 0.0;
    double phaseIncrement = 0.0;
    int currentWaveform = 0;
    float pulseWidth = 0.5f;
    int currentMidiNote = -1;
    float currentVelocity = 0.0f;

    // Filter
    juce::dsp::LadderFilter<float> ladderFilter;
    float baseCutoffHz = 1000.0f;
    float filterResonance = 0.1f;

    // Amp envelope
    juce::ADSR ampAdsr;
    juce::ADSR::Parameters ampAdsrParams { 0.01f, 0.1f, 0.8f, 0.3f };

    // Filter envelope
    juce::ADSR filterAdsr;
    juce::ADSR::Parameters filterAdsrParams { 0.01f, 0.3f, 0.0f, 0.3f };
    float filterEnvDepth = 0.5f;

    // Output gain
    float gain = 1.0f;

    double sampleRate = 44100.0;
    int preparedChannels = 2;

    // Noise RNG
    juce::Random noiseRng;

    // Sub-block update rate for filter cutoff
    static constexpr int filterUpdateInterval = 32;
};
