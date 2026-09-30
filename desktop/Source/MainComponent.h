#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "AudioEngine.h"
#include "TimelineRulerComponent.h"
#include "MixerComponent.h"
#include "TrackRowComponent.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void openAudioFiles();
    void addTrackRow(AudioTrack& track);
    void layoutTracks();
    void updateTimeline(double position);
    void seekTo(double seconds);
    void showAudioSettings();
    void timerCallback() override;

    static juce::String formatTime(double seconds);

    AudioEngine audioEngine;

    juce::Label titleLabel;
    juce::Label projectLabel;
    juce::Label timeLabel;
    juce::Label masterLabel;
    juce::Label audioStatusLabel;

    juce::TextButton openButton { "Importer pistes" };
    juce::TextButton playButton { "Play" };
    juce::TextButton pauseButton { "Pause" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton audioSettingsButton { "Audio" };
    juce::TextButton mixerButton { "Mixer" };

    juce::Slider masterSlider;

    TimelineRulerComponent timelineRuler;
    juce::Component trackList;
    juce::Viewport trackViewport;
    juce::OwnedArray<TrackRowComponent> trackRows;
    MixerComponent mixer;
    bool mixerVisible = true;

    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::Component::SafePointer<juce::DialogWindow> audioSettingsWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
