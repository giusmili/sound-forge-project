#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioTrack.h"
#include "TimelineRulerComponent.h"

class TrackRowComponent final
    : public juce::Component,
      private juce::ChangeListener
{
public:
    TrackRowComponent(
        AudioTrack& trackToControl,
        juce::AudioFormatManager& formatManager,
        juce::AudioThumbnailCache& thumbnailCache
    );

    ~TrackRowComponent() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    void setTimelineState(
        double projectLengthSeconds,
        double playheadSeconds
    );

    std::function<void(double)> onSeek;
    std::function<void()> onSoloChanged;

private:
    void changeListenerCallback(
        juce::ChangeBroadcaster* source
    ) override;

    void refreshMuteButton();
    void refreshSoloButton();

    [[nodiscard]] juce::Rectangle<int> getWaveformBounds() const;
    [[nodiscard]] juce::Rectangle<int> getClipBounds() const;

    AudioTrack& track;
    juce::AudioThumbnail thumbnail;

    double projectLength = 0.0;
    double playheadPosition = 0.0;

    juce::Label nameLabel;
    juce::Label volumeLabel;
    juce::Label panLabel;

    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };
    juce::Slider volumeSlider;
    juce::Slider panSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackRowComponent)
};
