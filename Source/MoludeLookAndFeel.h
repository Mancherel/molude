#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class MoludeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MoludeLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override;

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    juce::Label* createSliderTextBox (juce::Slider& slider) override;
    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    // Shared palette
    static inline const juce::Colour bg      { 0xff12122a };
    static inline const juce::Colour bgLight { 0xff1a1a3e };
    static inline const juce::Colour surface { 0xff16213e };
    static inline const juce::Colour panel   { 0xff1c1c3a };
    static inline const juce::Colour accent  { 0xffe94560 };
    static inline const juce::Colour text    { 0xffeaeaea };
    static inline const juce::Colour dimText { 0xff888899 };
};
