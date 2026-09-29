#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "AudioEngine.h"
#include "TimelineRulerComponent.h"
#include "TrackRowComponent.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void openAudioFiles();
    TrackRowComponent* addTrackRow(AudioTrack& track);
    void layoutTracks();
    void updateTimeline(double position);
    void updateProjectState();
    void seekTo(double seconds);
    void clipMoved();
    void selectRow(TrackRowComponent* row);
    void duplicateSelectedClip();
    void deleteSelectedClip();
    void updateGridSettings();
    void showAudioSettings();
    void timerCallback() override;

    [[nodiscard]] double getViewDuration(double projectLength) const;
    [[nodiscard]] double getViewStart(double projectLength, double viewDuration) const;
    [[nodiscard]] double getGridIntervalSeconds() const;

    static juce::String formatTime(double seconds);

    AudioEngine audioEngine;

    juce::Label titleLabel;
    juce::Label projectLabel;
    juce::Label timeLabel;
    juce::Label masterLabel;
    juce::Label bpmLabel;
    juce::Label gridLabel;
    juce::Label zoomLabel;
    juce::Label viewLabel;

    juce::TextButton openButton { "Importer pistes" };
    juce::TextButton duplicateButton { "Dupliquer" };
    juce::TextButton deleteButton { "Supprimer" };
    juce::TextButton playButton { "Play" };
    juce::TextButton pauseButton { "Pause" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton audioSettingsButton { "Audio" };
    juce::TextButton snapButton { "Snap Grid" };

    juce::ComboBox gridCombo;

    juce::Slider masterSlider;
    juce::Slider bpmSlider;
    juce::Slider zoomSlider;
    juce::Slider viewSlider;

    TimelineRulerComponent timelineRuler;
    juce::Component trackList;
    juce::Viewport trackViewport;
    juce::OwnedArray<TrackRowComponent> trackRows;
    TrackRowComponent* selectedRow = nullptr;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
