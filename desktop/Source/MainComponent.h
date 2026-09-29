#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "AudioEngine.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void openAudioFile();
    void showAudioSettings();
    void timerCallback() override;

    static juce::String formatTime(double seconds);

    AudioEngine audioEngine;

    juce::Label titleLabel;
    juce::Label fileLabel;
    juce::Label timeLabel;
    juce::Label volumeLabel;

    juce::TextButton openButton { "Importer audio" };
    juce::TextButton playButton { "Play" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton audioSettingsButton { "Audio" };

    juce::Slider volumeSlider;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
