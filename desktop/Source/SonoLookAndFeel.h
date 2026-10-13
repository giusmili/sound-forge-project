#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Dark theme shared by menus, combo boxes, popups, sliders and buttons.
class SonoLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SonoLookAndFeel()
    {
        const juce::Colour panel(0xff1b1f26);
        const juce::Colour raised(0xff262c36);
        const juce::Colour text(0xffe8edf3);
        const juce::Colour accent(0xff5aa9ff);

        setColour(juce::PopupMenu::backgroundColourId, panel);
        setColour(juce::PopupMenu::textColourId, text);
        setColour(juce::PopupMenu::headerTextColourId, accent);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, accent.darker(0.35f));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        setColour(juce::ComboBox::backgroundColourId, raised);
        setColour(juce::ComboBox::textColourId, text);
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a4350));
        setColour(juce::ComboBox::arrowColourId, accent);

        setColour(juce::TextButton::buttonColourId, raised);
        setColour(juce::TextButton::textColourOffId, text);
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);

        setColour(juce::Slider::trackColourId, accent);
        setColour(juce::Slider::thumbColourId, juce::Colour(0xffdbe9f8));
        setColour(juce::Slider::backgroundColourId, juce::Colour(0xff0f1217));
        setColour(juce::Slider::textBoxTextColourId, text);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff12161b));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3a4350));

        setColour(juce::Label::textColourId, text);

        setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff4a5566));

        setColour(juce::TooltipWindow::backgroundColourId, raised);
        setColour(juce::TooltipWindow::textColourId, text);
        setColour(juce::TooltipWindow::outlineColourId, juce::Colour(0xff3a4350));
    }

    // Taller, easier to hit popup rows.
    void getIdealPopupMenuItemSize(
        const juce::String& text,
        const bool isSeparator,
        const int standardMenuItemHeight,
        int& idealWidth,
        int& idealHeight
    ) override
    {
        juce::LookAndFeel_V4::getIdealPopupMenuItemSize(
            text, isSeparator, standardMenuItemHeight, idealWidth, idealHeight
        );

        if (! isSeparator)
        {
            idealHeight = juce::jmax(idealHeight, 30);
            idealWidth += 24;
        }
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::Font(juce::FontOptions(15.0f));
    }

    // Slightly thicker linear slider track so the pan/volume thumb is easy to grab.
    int getSliderThumbRadius(juce::Slider& slider) override
    {
        return slider.isHorizontal() ? 8 : juce::LookAndFeel_V4::getSliderThumbRadius(slider);
    }
};
