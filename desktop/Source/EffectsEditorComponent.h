#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "TrackEffects.h"

// Data-driven editor: one row per effect, built from the fx::ParamInfo table.
class EffectsEditorComponent final : public juce::Component
{
public:
    EffectsEditorComponent(
        TrackEffects& effectsToEdit,
        juce::String trackName,
        std::function<void()> changedCallback
    );

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    struct Control
    {
        int param = 0;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::Label> label;
    };

    void notifyChanged();

    TrackEffects& effects;
    juce::String title;
    std::function<void()> changed;
    std::vector<std::unique_ptr<Control>> controls;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsEditorComponent)
};
