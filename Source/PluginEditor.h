#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "MoludeLookAndFeel.h"

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

    // Waveform selector
    juce::ComboBox waveformBox;
    juce::Label waveformLabel;

    // Filter knobs
    juce::Slider cutoffSlider;
    juce::Label cutoffLabel;
    juce::Slider resonanceSlider;
    juce::Label resonanceLabel;

    // Filter envelope depth
    juce::Slider filtEnvDepthSlider;
    juce::Label filtEnvDepthLabel;

    // Gain knob
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
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
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
