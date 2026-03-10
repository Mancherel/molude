#include "MoludeLookAndFeel.h"

MoludeLookAndFeel::MoludeLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, bg);
    setColour (juce::Slider::rotarySliderFillColourId, accent);
    setColour (juce::Slider::rotarySliderOutlineColourId, surface);
    setColour (juce::Slider::thumbColourId, text);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, text);
    setColour (juce::ComboBox::backgroundColourId, surface);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, accent.withAlpha (0.4f));
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

    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.38f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();

    // === Drop shadow ===
    {
        juce::ColourGradient shadowGrad (juce::Colours::black.withAlpha (0.4f),
                                          centreX, centreY + radius * 0.15f,
                                          juce::Colours::transparentBlack,
                                          centreX, centreY + radius * 1.4f, true);
        g.setGradientFill (shadowGrad);
        g.fillEllipse (centreX - radius * 1.05f, centreY - radius * 0.9f,
                        radius * 2.1f, radius * 2.3f);
    }

    // === Outer ring (inset bezel) ===
    auto outerRadius = radius * 1.08f;
    {
        juce::ColourGradient bezelGrad (juce::Colour (0xff2a2a4a), centreX, centreY - outerRadius,
                                         juce::Colour (0xff0a0a1a), centreX, centreY + outerRadius, false);
        g.setGradientFill (bezelGrad);
        g.fillEllipse (centreX - outerRadius, centreY - outerRadius,
                        outerRadius * 2.0f, outerRadius * 2.0f);
    }

    // === Knob body (metallic gradient) ===
    {
        juce::ColourGradient knobGrad (juce::Colour (0xff3a3a5e), centreX, centreY - radius,
                                        juce::Colour (0xff18182e), centreX, centreY + radius, false);
        g.setGradientFill (knobGrad);
        g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);
    }

    // === Subtle specular highlight (top-left) ===
    {
        auto highlightRadius = radius * 0.7f;
        auto hlX = centreX - radius * 0.25f;
        auto hlY = centreY - radius * 0.25f;
        juce::ColourGradient specGrad (juce::Colours::white.withAlpha (0.07f), hlX, hlY,
                                        juce::Colours::transparentBlack, hlX + highlightRadius, hlY + highlightRadius, true);
        g.setGradientFill (specGrad);
        g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);
    }

    // === Track arc (background) ===
    auto arcRadius = radius * 1.22f;
    auto lineWidth = 2.5f;
    {
        juce::Path bgArc;
        bgArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                              rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff0a0a1a));
        g.strokePath (bgArc, juce::PathStrokeType (lineWidth, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // === Value arc ===
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    {
        juce::Path valueArc;
        valueArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                                 rotaryStartAngle, toAngle, true);
        g.setColour (accent);
        g.strokePath (valueArc, juce::PathStrokeType (lineWidth + 0.5f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    // === Indicator line ===
    auto lineInner = radius * 0.35f;
    auto lineOuter = radius * 0.82f;
    auto angle = toAngle - juce::MathConstants<float>::halfPi;
    auto cosA = std::cos (angle);
    auto sinA = std::sin (angle);

    g.setColour (juce::Colours::white);
    g.drawLine (centreX + lineInner * cosA, centreY + lineInner * sinA,
                centreX + lineOuter * cosA, centreY + lineOuter * sinA,
                2.0f);
}

void MoludeLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                       int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                       juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat();
    auto cornerSize = 6.0f;

    // Background
    g.setColour (surface);
    g.fillRoundedRectangle (bounds, cornerSize);

    // Border
    g.setColour (accent.withAlpha (0.4f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);

    // Arrow
    auto arrowZone = bounds.removeFromRight (static_cast<float> (height)).reduced (8.0f);
    juce::Path arrow;
    arrow.addTriangle (arrowZone.getX(), arrowZone.getCentreY() - 3.0f,
                        arrowZone.getRight(), arrowZone.getCentreY() - 3.0f,
                        arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);
    g.setColour (box.isEnabled() ? text : dimText);
    g.fillPath (arrow);
}

void MoludeLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (surface);
    g.setColour (accent.withAlpha (0.3f));
    g.drawRect (0, 0, width, height, 1);
}

void MoludeLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                            bool /*isSeparator*/, bool isActive, bool isHighlighted,
                                            bool /*isTicked*/, bool /*hasSubMenu*/,
                                            const juce::String& itemText, const juce::String& /*shortcutKeyText*/,
                                            const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    if (isHighlighted && isActive)
    {
        g.setColour (accent);
        g.fillRect (area);
        g.setColour (juce::Colours::white);
    }
    else
    {
        g.setColour (isActive ? text : dimText);
    }

    auto textArea = area.reduced (10, 0);
    g.setFont (juce::FontOptions (14.0f));
    g.drawFittedText (itemText, textArea, juce::Justification::centredLeft, 1);
}

juce::Label* MoludeLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);

    // Monospace font for numeric values
    label->setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::plain));
    label->setJustificationType (juce::Justification::centred);

    return label;
}

void MoludeLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    // Check if this label belongs to a slider (text box)
    bool isSliderTextBox = (dynamic_cast<juce::Slider*> (label.getParentComponent()) != nullptr);

    if (isSliderTextBox)
    {
        auto bounds = label.getLocalBounds().toFloat();
        auto cornerSize = 4.0f;

        // Inset background
        g.setColour (juce::Colour (0xff0e0e20));
        g.fillRoundedRectangle (bounds, cornerSize);

        // Inset border: darker top, lighter bottom
        {
            juce::Path topBorder;
            topBorder.addArc (bounds.getX() + cornerSize, bounds.getY(),
                               bounds.getWidth() - cornerSize * 2.0f, bounds.getHeight(),
                               -juce::MathConstants<float>::pi, 0.0f);
            g.setColour (juce::Colour (0xff060612));
            g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);
        }

        // Subtle inner highlight at bottom edge
        {
            auto hlRect = bounds.reduced (cornerSize + 1.0f, 0.0f)
                              .withTop (bounds.getBottom() - 1.0f)
                              .withHeight (0.5f);
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.fillRect (hlRect);
        }

        // Draw text in accent colour with monospace
        if (! label.isBeingEdited())
        {
            g.setColour (accent.withAlpha (0.85f));
            g.setFont (label.getFont());
            g.drawFittedText (label.getText(), label.getLocalBounds().reduced (2, 0),
                              label.getJustificationType(), 1);
        }
    }
    else
    {
        // Default rendering for regular labels
        if (! label.isBeingEdited())
        {
            auto textArea = label.getLocalBounds();
            g.setColour (label.findColour (juce::Label::textColourId));
            g.setFont (label.getFont());
            g.drawFittedText (label.getText(), textArea, label.getJustificationType(), 1);
        }
    }
}
