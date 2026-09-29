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
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    void setTimelineState(
        double projectLengthSeconds,
        double playheadSeconds,
        double viewStartSeconds,
        double viewDurationSeconds
    );

    void setSnapSettings(
        bool enabled,
        double intervalSeconds
    );

    void setSelected(bool shouldBeSelected);
    void syncControlsFromTrack();

    [[nodiscard]] bool isSelected() const noexcept;
    [[nodiscard]] AudioTrack& getTrack() noexcept;

    std::function<void(double)> onSeek;
    std::function<void()> onSoloChanged;
    std::function<void()> onClipMoved;
    std::function<void()> onEditBegin;
    std::function<void(TrackRowComponent*)> onSelectionRequested;

private:
    enum class DragMode
    {
        none,
        move,
        trimLeft,
        trimRight
    };

    void changeListenerCallback(
        juce::ChangeBroadcaster* source
    ) override;

    void refreshMuteButton();
    void refreshSoloButton();
    void updateMouseCursor(juce::Point<float> position);

    [[nodiscard]] juce::Rectangle<int> getWaveformBounds() const;
    [[nodiscard]] juce::Rectangle<int> getClipBounds() const;

    AudioTrack& track;
    juce::AudioThumbnail thumbnail;

    double projectLength = 0.0;
    double playheadPosition = 0.0;
    double viewStart = 0.0;
    double viewDuration = 0.0;

    bool snapEnabled = true;
    double snapInterval = 1.0;

    bool selected = false;
    bool editSnapshotSent = false;
    DragMode dragMode = DragMode::none;
    float dragStartX = 0.0f;
    double dragStartOffset = 0.0;
    double dragSourceStart = 0.0;
    double dragSourceEnd = 0.0;
    double dragViewDuration = 0.0;

    juce::Label nameLabel;
    juce::Label volumeLabel;
    juce::Label panLabel;

    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };
    juce::Slider volumeSlider;
    juce::Slider panSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackRowComponent)
};
