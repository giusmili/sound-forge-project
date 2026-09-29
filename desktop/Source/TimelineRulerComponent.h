#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class TimelineRulerComponent final : public juce::Component
{
public:
    TimelineRulerComponent() = default;

    void paint(juce::Graphics& graphics) override;
    void mouseDown(const juce::MouseEvent& event) override;

    void setView(
        double projectLengthSeconds,
        double viewStartSeconds,
        double viewDurationSeconds,
        double positionSeconds,
        double gridIntervalSeconds
    );

    std::function<void(double)> onSeek;

    static constexpr int controlsWidth = 250;

private:
    static juce::String formatTick(double seconds);
    [[nodiscard]] double chooseTickInterval() const;

    double projectLength = 0.0;
    double viewStart = 0.0;
    double viewDuration = 0.0;
    double position = 0.0;
    double gridInterval = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineRulerComponent)
};
