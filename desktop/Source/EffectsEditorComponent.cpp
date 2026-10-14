#include "EffectsEditorComponent.h"

namespace
{
constexpr auto panelColour = 0xff15191f;
constexpr auto rowColour = 0xff1c2129;
constexpr auto textColour = 0xffe8edf3;
constexpr auto accentColour = 0xff5aa9ff;
constexpr int editorWidth = 600;
constexpr int titleHeight = 34;
constexpr int rowHeight = 96;
constexpr int rowGap = 4;
constexpr int enableColumnWidth = 130;
constexpr int padding = 8;
}

EffectsEditorComponent::EffectsEditorComponent(
    TrackEffects& effectsToEdit,
    juce::String trackName,
    std::function<void()> changedCallback
)
    : effects(effectsToEdit),
      title(std::move(trackName)),
      changed(std::move(changedCallback))
{
    for (int index = 0; index < fx::paramCount; ++index)
    {
        const auto& info = fx::getParamInfo(index);
        auto control = std::make_unique<Control>();
        control->param = index;

        if (info.kind == fx::Kind::slider)
        {
            auto slider = std::make_unique<juce::Slider>(
                juce::Slider::RotaryHorizontalVerticalDrag,
                juce::Slider::TextBoxBelow
            );
            slider->setRange(info.minimum, info.maximum, info.step);

            if (info.skewMidpoint > 0.0f)
                slider->setSkewFactorFromMidPoint(info.skewMidpoint);

            slider->setValue(
                effects.getParam(index),
                juce::dontSendNotification
            );
            slider->setDoubleClickReturnValue(true, info.defaultValue);
            slider->setTextValueSuffix(info.suffix);
            slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
            slider->setColour(
                juce::Slider::rotarySliderFillColourId,
                juce::Colour(accentColour)
            );
            slider->onValueChange = [this, index, slider = slider.get()]
            {
                effects.setParam(
                    index,
                    static_cast<float>(slider->getValue())
                );
                notifyChanged();
            };
            addAndMakeVisible(*slider);

            auto label = std::make_unique<juce::Label>(
                juce::String(),
                info.label
            );
            label->setJustificationType(juce::Justification::centred);
            label->setColour(
                juce::Label::textColourId,
                juce::Colour(textColour).withAlpha(0.75f)
            );
            label->setFont(juce::FontOptions(11.0f));
            addAndMakeVisible(*label);

            control->slider = std::move(slider);
            control->label = std::move(label);
        }
        else
        {
            auto toggle = std::make_unique<juce::ToggleButton>(info.label);
            toggle->setToggleState(
                effects.getParam(index) >= 0.5f,
                juce::dontSendNotification
            );
            toggle->setColour(
                juce::ToggleButton::textColourId,
                juce::Colour(textColour)
            );
            toggle->setColour(
                juce::ToggleButton::tickColourId,
                juce::Colour(accentColour)
            );
            toggle->onClick = [this, index, toggle = toggle.get()]
            {
                effects.setParam(
                    index,
                    toggle->getToggleState() ? 1.0f : 0.0f
                );
                notifyChanged();
            };
            addAndMakeVisible(*toggle);

            control->toggle = std::move(toggle);
        }

        controls.push_back(std::move(control));
    }

    setSize(
        editorWidth,
        titleHeight + fx::groupCount * (rowHeight + rowGap) + padding
    );
}

void EffectsEditorComponent::notifyChanged()
{
    if (changed)
        changed();
}

void EffectsEditorComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(panelColour));

    graphics.setColour(juce::Colour(textColour));
    graphics.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    graphics.drawText(
        "Effets - " + title,
        padding,
        0,
        getWidth() - 2 * padding,
        titleHeight,
        juce::Justification::centredLeft
    );

    for (int group = 0; group < fx::groupCount; ++group)
    {
        const auto top = titleHeight + group * (rowHeight + rowGap);

        graphics.setColour(juce::Colour(rowColour));
        graphics.fillRoundedRectangle(
            juce::Rectangle<int>(
                padding,
                top,
                getWidth() - 2 * padding,
                rowHeight
            ).toFloat(),
            5.0f
        );
    }
}

void EffectsEditorComponent::resized()
{
    for (int group = 0; group < fx::groupCount; ++group)
    {
        auto row = juce::Rectangle<int>(
            padding,
            titleHeight + group * (rowHeight + rowGap),
            getWidth() - 2 * padding,
            rowHeight
        ).reduced(6, 4);

        auto enableColumn = row.removeFromLeft(enableColumnWidth);

        std::vector<Control*> sliders;

        for (auto& control : controls)
        {
            const auto& info = fx::getParamInfo(control->param);

            if (static_cast<int>(info.group) != group)
                continue;

            if (info.kind == fx::Kind::enable)
                control->toggle->setBounds(enableColumn.removeFromTop(28));
            else if (info.kind == fx::Kind::toggle)
                control->toggle->setBounds(enableColumn.removeFromTop(28));
            else
                sliders.push_back(control.get());
        }

        if (sliders.empty())
            continue;

        const auto width = row.getWidth() / static_cast<int>(sliders.size());

        for (auto* control : sliders)
        {
            auto cell = row.removeFromLeft(width);
            control->label->setBounds(cell.removeFromTop(16));
            control->slider->setBounds(cell);
        }
    }
}
