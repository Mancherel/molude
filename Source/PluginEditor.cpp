#include "PluginEditor.h"

MoludeEditor::MoludeEditor (MoludeAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&moludeLnF);
    setSize (520, 450);

    // Waveform combo box
    waveformBox.addItem ("Sine", 1);
    waveformBox.addItem ("Saw", 2);
    addAndMakeVisible (waveformBox);

    waveformLabel.setText ("Waveform", juce::dontSendNotification);
    waveformLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (waveformLabel);

    // Filter knobs
    setupRotarySlider (cutoffSlider, cutoffLabel, "Cutoff");
    setupRotarySlider (resonanceSlider, resonanceLabel, "Reso");
    setupRotarySlider (filtEnvDepthSlider, filtEnvDepthLabel, "Depth");
    setupRotarySlider (gainSlider, gainLabel, "Gain");

    // Filter envelope ADSR
    setupRotarySlider (filtEnvAttackSlider, filtEnvAttackLabel, "Attack");
    setupRotarySlider (filtEnvDecaySlider, filtEnvDecayLabel, "Decay");
    setupRotarySlider (filtEnvSustainSlider, filtEnvSustainLabel, "Sustain");
    setupRotarySlider (filtEnvReleaseSlider, filtEnvReleaseLabel, "Release");

    // Amp ADSR knobs
    setupRotarySlider (attackSlider, attackLabel, "Attack");
    setupRotarySlider (decaySlider, decayLabel, "Decay");
    setupRotarySlider (sustainSlider, sustainLabel, "Sustain");
    setupRotarySlider (releaseSlider, releaseLabel, "Release");

    // Compact cutoff display
    cutoffSlider.textFromValueFunction = [] (double value)
    {
        if (value >= 1000.0)
            return juce::String (value / 1000.0, 1) + " kHz";
        return juce::String (static_cast<int> (value)) + " Hz";
    };
    cutoffSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        if (text.containsIgnoreCase ("kHz"))
            return text.getDoubleValue() * 1000.0;
        return text.getDoubleValue();
    };

    // Attachments
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processorRef.apvts, "OSC_WAVEFORM", waveformBox);
    cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "FILTER_CUTOFF", cutoffSlider);
    resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "FILTER_RESONANCE", resonanceSlider);
    filtEnvDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "FILT_ENV_DEPTH", filtEnvDepthSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "MASTER_GAIN", gainSlider);
    filtEnvAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "FILT_ENV_ATTACK", filtEnvAttackSlider);
    filtEnvDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "FILT_ENV_DECAY", filtEnvDecaySlider);
    filtEnvSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "FILT_ENV_SUSTAIN", filtEnvSustainSlider);
    filtEnvReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "FILT_ENV_RELEASE", filtEnvReleaseSlider);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "AMP_ATTACK", attackSlider);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "AMP_DECAY", decaySlider);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "AMP_SUSTAIN", sustainSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "AMP_RELEASE", releaseSlider);
}

MoludeEditor::~MoludeEditor()
{
    setLookAndFeel (nullptr);
}

void MoludeEditor::setupRotarySlider (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

void MoludeEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Title
    g.setColour (juce::Colour (0xffe94560));
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    g.drawText ("MOLUDE", getLocalBounds().removeFromTop (50), juce::Justification::centred);

    // Section labels
    auto area = getLocalBounds().reduced (15);
    area.removeFromTop (45);
    area.removeFromTop (110); // skip top row

    g.setColour (juce::Colour (0x99e94560));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));

    auto filtEnvSection = area.removeFromTop (110);
    g.drawText ("FILTER ENV", filtEnvSection.removeFromLeft (15).withWidth (100),
                juce::Justification::centredLeft);

    area.removeFromTop (5);
    auto ampEnvSection = area;
    g.drawText ("AMP ENV", ampEnvSection.removeFromLeft (15).withWidth (100),
                juce::Justification::centredLeft);
}

void MoludeEditor::resized()
{
    auto area = getLocalBounds().reduced (15);

    // Title area
    area.removeFromTop (45);

    // Top row: waveform + cutoff + reso + depth + gain
    auto topRow = area.removeFromTop (110);
    auto colW = topRow.getWidth() / 5;

    auto waveformArea = topRow.removeFromLeft (colW);
    auto cutoffArea   = topRow.removeFromLeft (colW);
    auto resoArea     = topRow.removeFromLeft (colW);
    auto depthArea    = topRow.removeFromLeft (colW);
    auto gainArea     = topRow;

    waveformLabel.setBounds (waveformArea.removeFromTop (20));
    auto comboArea = waveformArea.reduced (8, 18);
    waveformBox.setBounds (comboArea.removeFromTop (28));

    cutoffLabel.setBounds (cutoffArea.removeFromTop (20));
    cutoffSlider.setBounds (cutoffArea);

    resonanceLabel.setBounds (resoArea.removeFromTop (20));
    resonanceSlider.setBounds (resoArea);

    filtEnvDepthLabel.setBounds (depthArea.removeFromTop (20));
    filtEnvDepthSlider.setBounds (depthArea);

    gainLabel.setBounds (gainArea.removeFromTop (20));
    gainSlider.setBounds (gainArea);

    // Filter envelope row
    auto filtEnvRow = area.removeFromTop (110);
    auto feKnobW = filtEnvRow.getWidth() / 4;

    auto feAtkArea = filtEnvRow.removeFromLeft (feKnobW);
    auto feDecArea = filtEnvRow.removeFromLeft (feKnobW);
    auto feSusArea = filtEnvRow.removeFromLeft (feKnobW);
    auto feRelArea = filtEnvRow;

    filtEnvAttackLabel.setBounds (feAtkArea.removeFromTop (20));
    filtEnvAttackSlider.setBounds (feAtkArea);

    filtEnvDecayLabel.setBounds (feDecArea.removeFromTop (20));
    filtEnvDecaySlider.setBounds (feDecArea);

    filtEnvSustainLabel.setBounds (feSusArea.removeFromTop (20));
    filtEnvSustainSlider.setBounds (feSusArea);

    filtEnvReleaseLabel.setBounds (feRelArea.removeFromTop (20));
    filtEnvReleaseSlider.setBounds (feRelArea);

    // Spacer
    area.removeFromTop (5);

    // Amp envelope row
    auto ampEnvRow = area;
    auto aeKnobW = ampEnvRow.getWidth() / 4;

    auto aeAtkArea = ampEnvRow.removeFromLeft (aeKnobW);
    auto aeDecArea = ampEnvRow.removeFromLeft (aeKnobW);
    auto aeSusArea = ampEnvRow.removeFromLeft (aeKnobW);
    auto aeRelArea = ampEnvRow;

    attackLabel.setBounds (aeAtkArea.removeFromTop (20));
    attackSlider.setBounds (aeAtkArea);

    decayLabel.setBounds (aeDecArea.removeFromTop (20));
    decaySlider.setBounds (aeDecArea);

    sustainLabel.setBounds (aeSusArea.removeFromTop (20));
    sustainSlider.setBounds (aeSusArea);

    releaseLabel.setBounds (aeRelArea.removeFromTop (20));
    releaseSlider.setBounds (aeRelArea);
}
