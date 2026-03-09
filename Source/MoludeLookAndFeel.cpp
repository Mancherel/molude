#include "MoludeLookAndFeel.h"

MoludeLookAndFeel::MoludeLookAndFeel()
{
    // Dark theme colours
    auto bg      = juce::Colour (0xff1a1a2e);
    auto surface = juce::Colour (0xff16213e);
    auto accent  = juce::Colour (0xffe94560);
    auto text    = juce::Colour (0xffeaeaea);

    setColour (juce::ResizableWindow::backgroundColourId, bg);
    setColour (juce::Slider::rotarySliderFillColourId, accent);
    setColour (juce::Slider::rotarySliderOutlineColourId, surface);
    setColour (juce::Slider::thumbColourId, text);
    setColour (juce::Label::textColourId, text);
    setColour (juce::ComboBox::backgroundColourId, surface);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, accent);
    setColour (juce::ComboBox::arrowColourId, text);
    setColour (juce::PopupMenu::backgroundColourId, surface);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

void MoludeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider&)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                           static_cast<float> (width), static_cast<float> (height));

    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.4f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto arcRadius = radius;

    auto accent  = juce::Colour (0xffe94560);
    auto surface = juce::Colour (0xff16213e);

    // Background circle
    g.setColour (surface);
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Track arc (background)
    auto lineWidth = 3.0f;
    juce::Path bgArc;
    bgArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                          rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff0a0a1a));
    g.strokePath (bgArc, juce::PathStrokeType (lineWidth, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    // Value arc
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path valueArc;
    valueArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, toAngle, true);
    g.setColour (accent);
    g.strokePath (valueArc, juce::PathStrokeType (lineWidth, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

    // Pointer dot
    auto pointerLength = radius * 0.6f;
    auto pointerX = centreX + pointerLength * std::cos (toAngle - juce::MathConstants<float>::halfPi);
    auto pointerY = centreY + pointerLength * std::sin (toAngle - juce::MathConstants<float>::halfPi);
    auto dotSize = 5.0f;
    g.setColour (juce::Colours::white);
    g.fillEllipse (pointerX - dotSize * 0.5f, pointerY - dotSize * 0.5f, dotSize, dotSize);
}
