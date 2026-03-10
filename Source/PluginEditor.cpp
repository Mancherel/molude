#include "PluginEditor.h"

//==============================================================================
// Value formatting helpers
//==============================================================================
static juce::String timeToString (double value)
{
    if (value >= 1.0)
        return juce::String (value, 2) + " s";
    return juce::String (static_cast<int> (value * 1000.0)) + " ms";
}

static double timeFromString (const juce::String& text)
{
    if (text.containsIgnoreCase ("ms"))
        return text.getDoubleValue() / 1000.0;
    return text.getDoubleValue();
}

static juce::String percentToString (double value)
{
    return juce::String (static_cast<int> (value * 100.0)) + "%";
}

static double percentFromString (const juce::String& text)
{
    return text.getDoubleValue() / 100.0;
}

//==============================================================================
// WaveformDisplay — oscilloscope-style waveform visualizer
//==============================================================================
WaveformDisplay::WaveformDisplay()
{
    startTimerHz (24);
}

void WaveformDisplay::setParameters (std::atomic<float>* waveform, std::atomic<float>* pw)
{
    waveformParam = waveform;
    pwParam       = pw;
}

void WaveformDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto cornerSize = 5.0f;

    // === Screen bezel (inset) ===
    g.setColour (juce::Colour (0xff060610));
    g.fillRoundedRectangle (bounds, cornerSize);
    g.setColour (juce::Colour (0xff040408));
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);

    auto screen = bounds.reduced (4.0f);
    g.setColour (juce::Colour (0xff080812));
    g.fillRoundedRectangle (screen, 3.0f);

    // === Scanlines ===
    g.setColour (juce::Colours::white.withAlpha (0.015f));
    for (float y = screen.getY(); y < screen.getBottom(); y += 2.0f)
        g.drawHorizontalLine (static_cast<int> (y), screen.getX(), screen.getRight());

    // === Grid dots (amber tint) ===
    g.setColour (juce::Colour (0xff3a2a1a));
    int cols = 8, rows = 4;
    for (int r = 1; r < rows; ++r)
    {
        for (int c = 1; c < cols; ++c)
        {
            float gx = screen.getX() + screen.getWidth() * (static_cast<float> (c) / static_cast<float> (cols));
            float gy = screen.getY() + screen.getHeight() * (static_cast<float> (r) / static_cast<float> (rows));
            g.fillEllipse (gx - 0.5f, gy - 0.5f, 1.0f, 1.0f);
        }
    }

    // === Center line ===
    auto phosphor = juce::Colour (0xffffaa00); // amber
    auto sx = screen.getX() + 3.0f;
    auto sy = screen.getY() + 3.0f;
    auto sw = screen.getWidth() - 6.0f;
    auto sh = screen.getHeight() - 6.0f;
    auto midY = sy + sh * 0.5f;

    g.setColour (phosphor.withAlpha (0.1f));
    g.drawHorizontalLine (static_cast<int> (midY), sx, sx + sw);

    // === Read parameters ===
    int waveform = waveformParam ? static_cast<int> (waveformParam->load()) : 0;
    float pw = pwParam ? pwParam->load() : 0.5f;

    // === Draw 2 cycles of the waveform ===
    juce::Path wavePath;
    float cycles = 2.0f;
    float amplitude = sh * 0.42f;
    int numPoints = static_cast<int> (sw);

    juce::Random noiseRng (42); // fixed seed for deterministic noise

    for (int i = 0; i <= numPoints; ++i)
    {
        float t = static_cast<float> (i) / static_cast<float> (numPoints);
        float phase = t * cycles;
        phase -= std::floor (phase); // wrap 0–1

        float sample = 0.0f;

        switch (waveform)
        {
            case 0: // Sine
                sample = std::sin (2.0f * juce::MathConstants<float>::pi * phase);
                break;
            case 1: // Saw
                sample = 2.0f * phase - 1.0f;
                break;
            case 2: // Square
                sample = (phase < 0.5f) ? 1.0f : -1.0f;
                break;
            case 3: // Triangle
                sample = 2.0f * std::abs (2.0f * phase - 1.0f) - 1.0f;
                break;
            case 4: // Pulse
                sample = (phase < pw) ? 1.0f : -1.0f;
                break;
            case 5: // Noise
                sample = noiseRng.nextFloat() * 2.0f - 1.0f;
                break;
            default:
                break;
        }

        float px = sx + t * sw;
        float py = midY - sample * amplitude;

        if (i == 0)
            wavePath.startNewSubPath (px, py);
        else
            wavePath.lineTo (px, py);
    }

    // === Glow layers ===
    g.setColour (phosphor.withAlpha (0.08f));
    g.strokePath (wavePath, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    g.setColour (phosphor.withAlpha (0.15f));
    g.strokePath (wavePath, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    g.setColour (phosphor.withAlpha (0.85f));
    g.strokePath (wavePath, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

    // === Vignette ===
    {
        juce::ColourGradient vignette (juce::Colours::transparentBlack, screen.getCentreX(), screen.getCentreY(),
                                        juce::Colours::black.withAlpha (0.3f), screen.getX(), screen.getY(), true);
        g.setGradientFill (vignette);
        g.fillRoundedRectangle (screen, 3.0f);
    }
}

//==============================================================================
// FilterDisplay — oscilloscope-style frequency response
//==============================================================================
FilterDisplay::FilterDisplay()
{
    startTimerHz (24);
}

void FilterDisplay::setParameters (std::atomic<float>* cutoff, std::atomic<float>* reso,
                                     std::atomic<float>* depth)
{
    cutoffParam = cutoff;
    resoParam   = reso;
    depthParam  = depth;
}

float FilterDisplay::getMagnitude (float freq, float cutoffHz, float resonance)
{
    float x = freq / cutoffHz;
    float x2 = x * x;
    float x4 = x2 * x2;

    float base = 1.0f / std::sqrt (1.0f + x4 * x4);

    float logRatio = std::log2 (juce::jmax (x, 0.001f));
    float resPeak = 1.0f + resonance * 12.0f * std::exp (-logRatio * logRatio * 6.0f);

    return base * resPeak;
}

void FilterDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto cornerSize = 5.0f;

    g.setColour (juce::Colour (0xff060610));
    g.fillRoundedRectangle (bounds, cornerSize);
    g.setColour (juce::Colour (0xff040408));
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);

    auto screen = bounds.reduced (4.0f);
    g.setColour (juce::Colour (0xff080812));
    g.fillRoundedRectangle (screen, 3.0f);

    g.setColour (juce::Colours::white.withAlpha (0.015f));
    for (float y = screen.getY(); y < screen.getBottom(); y += 2.0f)
        g.drawHorizontalLine (static_cast<int> (y), screen.getX(), screen.getRight());

    g.setColour (juce::Colour (0xff1a2a3a));
    int cols = 8, rows = 4;
    for (int r = 1; r < rows; ++r)
    {
        for (int c = 1; c < cols; ++c)
        {
            float gx = screen.getX() + screen.getWidth() * (static_cast<float> (c) / static_cast<float> (cols));
            float gy = screen.getY() + screen.getHeight() * (static_cast<float> (r) / static_cast<float> (rows));
            g.fillEllipse (gx - 0.5f, gy - 0.5f, 1.0f, 1.0f);
        }
    }

    float cutoffHz  = cutoffParam ? cutoffParam->load() : 1000.0f;
    float resonance = resoParam   ? resoParam->load()   : 0.1f;
    float depth     = depthParam  ? depthParam->load()   : 0.5f;

    auto sx = screen.getX() + 3.0f;
    auto sy = screen.getY() + 3.0f;
    auto sw = screen.getWidth() - 6.0f;
    auto sh = screen.getHeight() - 6.0f;

    float logMin = std::log2 (20.0f);
    float logMax = std::log2 (20000.0f);
    float dbMin = -36.0f;
    float dbMax = 12.0f;

    auto freqToX = [&] (float freq)
    {
        float logF = std::log2 (juce::jmax (freq, 20.0f));
        return sx + ((logF - logMin) / (logMax - logMin)) * sw;
    };

    auto dbToY = [&] (float db)
    {
        float norm = (db - dbMin) / (dbMax - dbMin);
        return sy + sh - juce::jlimit (0.0f, 1.0f, norm) * sh;
    };

    auto buildResponsePath = [&] (float fc) -> juce::Path
    {
        juce::Path p;
        int numPoints = static_cast<int> (sw);
        for (int i = 0; i <= numPoints; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (numPoints);
            float logF = logMin + t * (logMax - logMin);
            float freq = std::pow (2.0f, logF);
            float mag = getMagnitude (freq, fc, resonance);
            float db = 20.0f * std::log10 (juce::jmax (mag, 0.0001f));
            float px = sx + t * sw;
            float py = dbToY (db);

            if (i == 0)
                p.startNewSubPath (px, py);
            else
                p.lineTo (px, py);
        }
        return p;
    };

    auto phosphor = juce::Colour (0xff00d4ff);

    if (std::abs (depth) > 0.01f)
    {
        float modOctaves = depth * 10.0f;
        float modCutoff = cutoffHz * std::pow (2.0f, modOctaves);
        modCutoff = juce::jlimit (20.0f, 20000.0f, modCutoff);

        auto ghostPath = buildResponsePath (modCutoff);

        auto basePath = buildResponsePath (cutoffHz);
        {
            juce::Path fillPath;
            fillPath.addPath (basePath);

            int numPts = static_cast<int> (sw);
            for (int i = numPts; i >= 0; --i)
            {
                float t = static_cast<float> (i) / static_cast<float> (numPts);
                float logF = logMin + t * (logMax - logMin);
                float freq = std::pow (2.0f, logF);
                float mag = getMagnitude (freq, modCutoff, resonance);
                float db = 20.0f * std::log10 (juce::jmax (mag, 0.0001f));
                float px = sx + t * sw;
                float py = dbToY (db);
                fillPath.lineTo (px, py);
            }
            fillPath.closeSubPath();

            g.setColour (phosphor.withAlpha (0.06f));
            g.fillPath (fillPath);
        }

        g.setColour (phosphor.withAlpha (0.12f));
        g.strokePath (ghostPath, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
    }

    auto mainPath = buildResponsePath (cutoffHz);

    g.setColour (phosphor.withAlpha (0.08f));
    g.strokePath (mainPath, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    g.setColour (phosphor.withAlpha (0.15f));
    g.strokePath (mainPath, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    g.setColour (phosphor.withAlpha (0.85f));
    g.strokePath (mainPath, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

    g.setColour (phosphor.withAlpha (0.12f));
    g.drawHorizontalLine (static_cast<int> (dbToY (0.0f)), sx, sx + sw);

    float cutoffX = freqToX (cutoffHz);
    g.setColour (phosphor.withAlpha (0.2f));
    g.drawVerticalLine (static_cast<int> (cutoffX), sy, sy + sh);

    {
        juce::ColourGradient vignette (juce::Colours::transparentBlack, screen.getCentreX(), screen.getCentreY(),
                                        juce::Colours::black.withAlpha (0.3f), screen.getX(), screen.getY(), true);
        g.setGradientFill (vignette);
        g.fillRoundedRectangle (screen, 3.0f);
    }
}

//==============================================================================
// EnvelopeDisplay — oscilloscope-style ADSR visualizer
//==============================================================================
EnvelopeDisplay::EnvelopeDisplay()
{
    startTimerHz (24);
}

void EnvelopeDisplay::setParameters (std::atomic<float>* a, std::atomic<float>* d,
                                      std::atomic<float>* s, std::atomic<float>* r)
{
    attackParam  = a;
    decayParam   = d;
    sustainParam = s;
    releaseParam = r;
}

void EnvelopeDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto cornerSize = 5.0f;

    g.setColour (juce::Colour (0xff060610));
    g.fillRoundedRectangle (bounds, cornerSize);
    g.setColour (juce::Colour (0xff040408));
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);

    auto screen = bounds.reduced (4.0f);
    g.setColour (juce::Colour (0xff080812));
    g.fillRoundedRectangle (screen, 3.0f);

    g.setColour (juce::Colours::white.withAlpha (0.015f));
    for (float y = screen.getY(); y < screen.getBottom(); y += 2.0f)
        g.drawHorizontalLine (static_cast<int> (y), screen.getX(), screen.getRight());

    g.setColour (juce::Colour (0xff1a3a1a));
    int cols = 8, rows = 4;
    for (int r = 1; r < rows; ++r)
    {
        for (int c = 1; c < cols; ++c)
        {
            float gx = screen.getX() + screen.getWidth() * (static_cast<float> (c) / static_cast<float> (cols));
            float gy = screen.getY() + screen.getHeight() * (static_cast<float> (r) / static_cast<float> (rows));
            g.fillEllipse (gx - 0.5f, gy - 0.5f, 1.0f, 1.0f);
        }
    }

    float attack  = attackParam  ? attackParam->load()  : 0.01f;
    float decay   = decayParam   ? decayParam->load()   : 0.1f;
    float sustain = sustainParam ? sustainParam->load()  : 0.8f;
    float release = releaseParam ? releaseParam->load()  : 0.3f;

    float sustainShow = juce::jmax (0.15f * (attack + decay + release), 0.05f);
    float totalTime = attack + decay + sustainShow + release;

    if (totalTime <= 0.0f)
        return;

    auto sx = screen.getX() + 3.0f;
    auto sy = screen.getY() + 3.0f;
    auto sw = screen.getWidth() - 6.0f;
    auto sh = screen.getHeight() - 6.0f;

    auto timeToX = [&] (float t) { return sx + (t / totalTime) * sw; };
    auto valToY  = [&] (float v) { return sy + sh - v * sh; };

    float tAttack  = attack;
    float tDecay   = tAttack + decay;
    float tSustain = tDecay + sustainShow;
    float tRelease = tSustain + release;

    juce::Path envPath;
    envPath.startNewSubPath (timeToX (0.0f), valToY (0.0f));
    envPath.lineTo (timeToX (tAttack), valToY (1.0f));
    envPath.lineTo (timeToX (tDecay), valToY (sustain));
    envPath.lineTo (timeToX (tSustain), valToY (sustain));
    envPath.lineTo (timeToX (tRelease), valToY (0.0f));

    auto phosphor = juce::Colour (0xff00ff41);

    g.setColour (phosphor.withAlpha (0.08f));
    g.strokePath (envPath, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    g.setColour (phosphor.withAlpha (0.15f));
    g.strokePath (envPath, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    g.setColour (phosphor.withAlpha (0.85f));
    g.strokePath (envPath, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

    g.setColour (phosphor);
    auto dotR = 2.0f;
    g.fillEllipse (timeToX (0.0f) - dotR, valToY (0.0f) - dotR, dotR * 2, dotR * 2);
    g.fillEllipse (timeToX (tAttack) - dotR, valToY (1.0f) - dotR, dotR * 2, dotR * 2);
    g.fillEllipse (timeToX (tDecay) - dotR, valToY (sustain) - dotR, dotR * 2, dotR * 2);
    g.fillEllipse (timeToX (tSustain) - dotR, valToY (sustain) - dotR, dotR * 2, dotR * 2);
    g.fillEllipse (timeToX (tRelease) - dotR, valToY (0.0f) - dotR, dotR * 2, dotR * 2);

    {
        juce::ColourGradient vignette (juce::Colours::transparentBlack, screen.getCentreX(), screen.getCentreY(),
                                        juce::Colours::black.withAlpha (0.3f), screen.getX(), screen.getY(), true);
        g.setGradientFill (vignette);
        g.fillRoundedRectangle (screen, 3.0f);
    }
}

//==============================================================================
MoludeEditor::MoludeEditor (MoludeAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&moludeLnF);
    setSize (640, 710);

    // Displays
    addAndMakeVisible (waveformDisplay);
    addAndMakeVisible (filterDisplay);
    addAndMakeVisible (filtEnvDisplay);
    addAndMakeVisible (ampEnvDisplay);

    waveformDisplay.setParameters (
        processorRef.apvts.getRawParameterValue ("OSC_WAVEFORM"),
        processorRef.apvts.getRawParameterValue ("PULSE_WIDTH"));

    filterDisplay.setParameters (
        processorRef.apvts.getRawParameterValue ("FILTER_CUTOFF"),
        processorRef.apvts.getRawParameterValue ("FILTER_RESONANCE"),
        processorRef.apvts.getRawParameterValue ("FILT_ENV_DEPTH"));

    filtEnvDisplay.setParameters (
        processorRef.apvts.getRawParameterValue ("FILT_ENV_ATTACK"),
        processorRef.apvts.getRawParameterValue ("FILT_ENV_DECAY"),
        processorRef.apvts.getRawParameterValue ("FILT_ENV_SUSTAIN"),
        processorRef.apvts.getRawParameterValue ("FILT_ENV_RELEASE"));

    ampEnvDisplay.setParameters (
        processorRef.apvts.getRawParameterValue ("AMP_ATTACK"),
        processorRef.apvts.getRawParameterValue ("AMP_DECAY"),
        processorRef.apvts.getRawParameterValue ("AMP_SUSTAIN"),
        processorRef.apvts.getRawParameterValue ("AMP_RELEASE"));

    // Oscillator knobs
    setupRotarySlider (waveformSlider, waveformLabel, "Wave");
    setupRotarySlider (pulseWidthSlider, pulseWidthLabel, "PW");

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

    // === Value formatting ===

    waveformSlider.textFromValueFunction = [] (double value)
    {
        static const char* names[] = { "Sine", "Saw", "Sqr", "Tri", "Pulse", "Noise" };
        int idx = juce::jlimit (0, 5, static_cast<int> (std::round (value)));
        return juce::String (names[idx]);
    };
    waveformSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        static const char* names[] = { "Sine", "Saw", "Sqr", "Tri", "Pulse", "Noise" };
        for (int i = 0; i < 6; ++i)
            if (text.containsIgnoreCase (names[i]))
                return static_cast<double> (i);
        return 0.0;
    };

    pulseWidthSlider.textFromValueFunction = percentToString;
    pulseWidthSlider.valueFromTextFunction = percentFromString;

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

    resonanceSlider.textFromValueFunction = percentToString;
    resonanceSlider.valueFromTextFunction = percentFromString;

    filtEnvDepthSlider.textFromValueFunction = [] (double value)
    {
        int pct = static_cast<int> (value * 100.0);
        if (pct > 0) return "+" + juce::String (pct) + "%";
        return juce::String (pct) + "%";
    };
    filtEnvDepthSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        return text.getDoubleValue() / 100.0;
    };

    gainSlider.textFromValueFunction = [] (double value)
    {
        return juce::String (value, 1) + " dB";
    };
    gainSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        return text.getDoubleValue();
    };

    filtEnvAttackSlider.textFromValueFunction = timeToString;
    filtEnvAttackSlider.valueFromTextFunction = timeFromString;
    filtEnvDecaySlider.textFromValueFunction = timeToString;
    filtEnvDecaySlider.valueFromTextFunction = timeFromString;
    filtEnvSustainSlider.textFromValueFunction = percentToString;
    filtEnvSustainSlider.valueFromTextFunction = percentFromString;
    filtEnvReleaseSlider.textFromValueFunction = timeToString;
    filtEnvReleaseSlider.valueFromTextFunction = timeFromString;

    attackSlider.textFromValueFunction = timeToString;
    attackSlider.valueFromTextFunction = timeFromString;
    decaySlider.textFromValueFunction = timeToString;
    decaySlider.valueFromTextFunction = timeFromString;
    sustainSlider.textFromValueFunction = percentToString;
    sustainSlider.valueFromTextFunction = percentFromString;
    releaseSlider.textFromValueFunction = timeToString;
    releaseSlider.valueFromTextFunction = timeFromString;

    // === Attachments ===

    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "OSC_WAVEFORM", waveformSlider);
    pulseWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "PULSE_WIDTH", pulseWidthSlider);
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

void MoludeEditor::setupRotarySlider (juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (label);
}

//==============================================================================
static void drawSectionPanel (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
{
    auto panelBounds = bounds.toFloat();
    auto cornerSize = 6.0f;

    g.setColour (MoludeLookAndFeel::panel);
    g.fillRoundedRectangle (panelBounds, cornerSize);

    g.setColour (juce::Colour (0xff2a2a50));
    g.drawRoundedRectangle (panelBounds.reduced (0.5f), cornerSize, 1.0f);

    {
        auto highlightRect = panelBounds.reduced (1.0f).withHeight (1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRect (highlightRect.reduced (cornerSize, 0.0f));
    }

    auto labelArea = bounds.withHeight (20).translated (0, 4);
    g.setColour (MoludeLookAndFeel::accent.withAlpha (0.7f));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));

    auto textLeft = static_cast<int> (panelBounds.getX()) + 12;
    auto textWidth = static_cast<int> (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title)) + 8;

    auto lineY = static_cast<float> (labelArea.getCentreY());
    g.setColour (MoludeLookAndFeel::accent.withAlpha (0.2f));
    g.drawLine (static_cast<float> (textLeft + textWidth + 4), lineY,
                panelBounds.getRight() - 12.0f, lineY, 1.0f);

    g.setColour (MoludeLookAndFeel::accent.withAlpha (0.7f));
    g.drawText (title, labelArea.withLeft (textLeft).withWidth (textWidth),
                juce::Justification::centredLeft);
}

//==============================================================================
void MoludeEditor::paint (juce::Graphics& g)
{
    {
        juce::ColourGradient bgGrad (MoludeLookAndFeel::bg.darker (0.15f), 0.0f, 0.0f,
                                      MoludeLookAndFeel::bg.brighter (0.05f), 0.0f, static_cast<float> (getHeight()), false);
        g.setGradientFill (bgGrad);
        g.fillRect (getLocalBounds());
    }

    auto area = getLocalBounds().reduced (14);

    // Title with glow
    {
        auto titleArea = area.removeFromTop (48);
        juce::String titleText = "M O L U D E";

        g.setColour (MoludeLookAndFeel::accent.withAlpha (0.25f));
        g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
        g.drawText (titleText, titleArea.translated (0, 1), juce::Justification::centred);
        g.drawText (titleText, titleArea.translated (1, 0), juce::Justification::centred);
        g.drawText (titleText, titleArea.translated (-1, 0), juce::Justification::centred);
        g.drawText (titleText, titleArea.translated (0, -1), juce::Justification::centred);

        g.setColour (MoludeLookAndFeel::accent);
        g.drawText (titleText, titleArea, juce::Justification::centred);
    }

    area.removeFromTop (4);

    auto panelMargin = 10;
    auto sectionH = 150;

    auto oscPanel = area.removeFromTop (sectionH);
    drawSectionPanel (g, oscPanel, "OSCILLATOR");

    area.removeFromTop (panelMargin);

    auto filterPanel = area.removeFromTop (sectionH);
    drawSectionPanel (g, filterPanel, "FILTER");

    area.removeFromTop (panelMargin);

    auto filtEnvPanel = area.removeFromTop (sectionH);
    drawSectionPanel (g, filtEnvPanel, "FILTER ENVELOPE");

    area.removeFromTop (panelMargin);

    auto ampPanel = area.removeFromTop (sectionH);
    drawSectionPanel (g, ampPanel, "AMP ENVELOPE");
}

//==============================================================================
void MoludeEditor::resized()
{
    auto area = getLocalBounds().reduced (14);

    area.removeFromTop (48);
    area.removeFromTop (4);

    auto panelMargin = 10;
    auto panelPadding = 8;
    auto sectionH = 150;
    auto labelH = 18;
    auto headerH = 22;
    auto bottomPad = 6;
    auto displayW = 130;

    // === Oscillator section: display + Wave + PW ===
    {
        auto oscPanel = area.removeFromTop (sectionH).reduced (panelPadding);
        oscPanel.removeFromTop (headerH);
        oscPanel.removeFromBottom (bottomPad);

        auto displayArea = oscPanel.removeFromLeft (displayW);
        waveformDisplay.setBounds (displayArea.reduced (2));

        oscPanel.removeFromLeft (6);

        auto colW = oscPanel.getWidth() / 2;

        auto waveArea = oscPanel.removeFromLeft (colW);
        auto pwArea   = oscPanel;

        waveformLabel.setBounds (waveArea.removeFromTop (labelH));
        waveformSlider.setBounds (waveArea);

        pulseWidthLabel.setBounds (pwArea.removeFromTop (labelH));
        pulseWidthSlider.setBounds (pwArea);
    }

    area.removeFromTop (panelMargin);

    // === Filter section: display + Cutoff, Reso, Depth, Gain ===
    {
        auto filterPanel = area.removeFromTop (sectionH).reduced (panelPadding);
        filterPanel.removeFromTop (headerH);
        filterPanel.removeFromBottom (bottomPad);

        auto displayArea = filterPanel.removeFromLeft (displayW);
        filterDisplay.setBounds (displayArea.reduced (2));

        filterPanel.removeFromLeft (6);

        auto colW = filterPanel.getWidth() / 4;

        auto cutoffArea = filterPanel.removeFromLeft (colW);
        auto resoArea   = filterPanel.removeFromLeft (colW);
        auto depthArea  = filterPanel.removeFromLeft (colW);
        auto gainArea   = filterPanel;

        cutoffLabel.setBounds (cutoffArea.removeFromTop (labelH));
        cutoffSlider.setBounds (cutoffArea);

        resonanceLabel.setBounds (resoArea.removeFromTop (labelH));
        resonanceSlider.setBounds (resoArea);

        filtEnvDepthLabel.setBounds (depthArea.removeFromTop (labelH));
        filtEnvDepthSlider.setBounds (depthArea);

        gainLabel.setBounds (gainArea.removeFromTop (labelH));
        gainSlider.setBounds (gainArea);
    }

    area.removeFromTop (panelMargin);

    // === Filter envelope: display + ADSR ===
    {
        auto filtPanel = area.removeFromTop (sectionH).reduced (panelPadding);
        filtPanel.removeFromTop (headerH);
        filtPanel.removeFromBottom (bottomPad);

        auto displayArea = filtPanel.removeFromLeft (displayW);
        filtEnvDisplay.setBounds (displayArea.reduced (2));

        filtPanel.removeFromLeft (6);

        auto knobW = filtPanel.getWidth() / 4;

        auto atkArea = filtPanel.removeFromLeft (knobW);
        auto decArea = filtPanel.removeFromLeft (knobW);
        auto susArea = filtPanel.removeFromLeft (knobW);
        auto relArea = filtPanel;

        filtEnvAttackLabel.setBounds (atkArea.removeFromTop (labelH));
        filtEnvAttackSlider.setBounds (atkArea);

        filtEnvDecayLabel.setBounds (decArea.removeFromTop (labelH));
        filtEnvDecaySlider.setBounds (decArea);

        filtEnvSustainLabel.setBounds (susArea.removeFromTop (labelH));
        filtEnvSustainSlider.setBounds (susArea);

        filtEnvReleaseLabel.setBounds (relArea.removeFromTop (labelH));
        filtEnvReleaseSlider.setBounds (relArea);
    }

    area.removeFromTop (panelMargin);

    // === Amp envelope: display + ADSR ===
    {
        auto ampPanel = area.removeFromTop (sectionH).reduced (panelPadding);
        ampPanel.removeFromTop (headerH);
        ampPanel.removeFromBottom (bottomPad);

        auto displayArea = ampPanel.removeFromLeft (displayW);
        ampEnvDisplay.setBounds (displayArea.reduced (2));

        ampPanel.removeFromLeft (6);

        auto knobW = ampPanel.getWidth() / 4;

        auto atkArea = ampPanel.removeFromLeft (knobW);
        auto decArea = ampPanel.removeFromLeft (knobW);
        auto susArea = ampPanel.removeFromLeft (knobW);
        auto relArea = ampPanel;

        attackLabel.setBounds (atkArea.removeFromTop (labelH));
        attackSlider.setBounds (atkArea);

        decayLabel.setBounds (decArea.removeFromTop (labelH));
        decaySlider.setBounds (decArea);

        sustainLabel.setBounds (susArea.removeFromTop (labelH));
        sustainSlider.setBounds (susArea);

        releaseLabel.setBounds (relArea.removeFromTop (labelH));
        releaseSlider.setBounds (relArea);
    }
}
