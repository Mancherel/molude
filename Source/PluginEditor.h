#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "MoludeLookAndFeel.h"

//==============================================================================
// Waveform oscilloscope display
//==============================================================================
class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    WaveformDisplay();

    void setParameters (std::atomic<float>* waveform, std::atomic<float>* pw);
    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override { repaint(); }

    std::atomic<float>* waveformParam = nullptr;
    std::atomic<float>* pwParam       = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};

//==============================================================================
// ADSR envelope display — oscilloscope style
//==============================================================================
class EnvelopeDisplay : public juce::Component, private juce::Timer
{
public:
    EnvelopeDisplay();

    void setParameters (std::atomic<float>* a, std::atomic<float>* d,
                        std::atomic<float>* s, std::atomic<float>* r);
    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override { repaint(); }

    std::atomic<float>* attackParam  = nullptr;
    std::atomic<float>* decayParam   = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeDisplay)
};

//==============================================================================
// Filter frequency response display — oscilloscope style
//==============================================================================
class FilterDisplay : public juce::Component, private juce::Timer
{
public:
    FilterDisplay();

    void setParameters (std::atomic<float>* cutoff, std::atomic<float>* reso,
                        std::atomic<float>* depth);
    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override { repaint(); }

    static float getMagnitude (float freq, float cutoffHz, float resonance);

    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* resoParam   = nullptr;
    std::atomic<float>* depthParam  = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterDisplay)
};

//==============================================================================
class MoludeEditor : public juce::AudioProcessorEditor
{
public:
    explicit MoludeEditor (MoludeAudioProcessor&);
    ~MoludeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MoludeAudioProcessor& processorRef;
    MoludeLookAndFeel moludeLnF;

    // Displays
    WaveformDisplay waveformDisplay;
    FilterDisplay filterDisplay;
    EnvelopeDisplay filtEnvDisplay;
    EnvelopeDisplay ampEnvDisplay;

    // Oscillator knobs
    juce::Slider waveformSlider;
    juce::Label waveformLabel;
    juce::Slider pulseWidthSlider;
    juce::Label pulseWidthLabel;

    // Filter knobs
    juce::Slider cutoffSlider;
    juce::Label cutoffLabel;
    juce::Slider resonanceSlider;
    juce::Label resonanceLabel;
    juce::Slider filtEnvDepthSlider;
    juce::Label filtEnvDepthLabel;
    juce::Slider gainSlider;
    juce::Label gainLabel;

    // Filter envelope ADSR
    juce::Slider filtEnvAttackSlider;
    juce::Label filtEnvAttackLabel;
    juce::Slider filtEnvDecaySlider;
    juce::Label filtEnvDecayLabel;
    juce::Slider filtEnvSustainSlider;
    juce::Label filtEnvSustainLabel;
    juce::Slider filtEnvReleaseSlider;
    juce::Label filtEnvReleaseLabel;

    // Amp ADSR knobs
    juce::Slider attackSlider;
    juce::Label attackLabel;
    juce::Slider decaySlider;
    juce::Label decayLabel;
    juce::Slider sustainSlider;
    juce::Label sustainLabel;
    juce::Slider releaseSlider;
    juce::Label releaseLabel;

    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> waveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pulseWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filtEnvDepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filtEnvAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filtEnvDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filtEnvSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filtEnvReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;

    void setupRotarySlider (juce::Slider& slider, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoludeEditor)
};
