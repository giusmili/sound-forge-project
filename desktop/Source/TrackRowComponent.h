#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioTrack.h"

class TrackRowComponent final : public juce::Component
{
public:
    explicit TrackRowComponent(AudioTrack& trackToControl);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void refreshMuteButton();

    AudioTrack& track;

    juce::Label nameLabel;
    juce::Label volumeLabel;
    juce::Label panLabel;

    juce::TextButton muteButton { "M" };
    juce::Slider volumeSlider;
    juce::Slider panSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackRowComponent)
};
