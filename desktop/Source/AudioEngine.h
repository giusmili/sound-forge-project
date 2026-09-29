#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "AudioTrack.h"

class AudioEngine final : public juce::AudioSource
{
public:
    AudioEngine();
    ~AudioEngine() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    juce::Result addTrackFromFile(
        const juce::File& file,
        AudioTrack*& createdTrack
    );

    juce::Result duplicateTrack(
        const AudioTrack& sourceTrack,
        AudioTrack*& createdTrack
    );

    juce::Result splitTrackAtProjectPosition(
        AudioTrack& sourceTrack,
        double projectPositionSeconds,
        AudioTrack*& rightTrack
    );

    bool removeTrack(AudioTrack* trackToRemove);

    void play();
    void pause();
    void stop();
    void setPositionSeconds(double seconds);
    void setMasterGain(float gain);
    void refreshSoloState();
    void refreshTrackAlignment();

    [[nodiscard]] bool isPlaying() const noexcept;
    [[nodiscard]] double getPositionSeconds() const noexcept;
    [[nodiscard]] double getLengthSeconds() const;
    [[nodiscard]] int getTrackCount() const noexcept;

    juce::AudioDeviceManager& getDeviceManager() noexcept;
    juce::AudioFormatManager& getFormatManager() noexcept;
    juce::AudioThumbnailCache& getThumbnailCache() noexcept;

private:
    [[nodiscard]] double getLengthSecondsUnlocked() const;
    void syncTracksUnlocked(double projectPosition, bool projectPlaying);
    void refreshSoloStateUnlocked();

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 64 };
    juce::MixerAudioSource mixer;
    std::vector<std::unique_ptr<AudioTrack>> tracks;

    mutable juce::CriticalSection trackLock;

    std::atomic<float> masterGain { 0.8f };
    std::atomic<double> projectPositionSeconds { 0.0 };
    std::atomic<double> outputSampleRate { 44100.0 };
    std::atomic<bool> playing { false };

    juce::AudioSourcePlayer sourcePlayer;
    juce::AudioDeviceManager deviceManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
