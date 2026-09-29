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

    void play();
    void pause();
    void stop();
    void setPositionSeconds(double seconds);
    void setMasterGain(float gain);
    void refreshSoloState();

    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] double getPositionSeconds() const;
    [[nodiscard]] double getLengthSeconds() const;
    [[nodiscard]] int getTrackCount() const noexcept;
    [[nodiscard]] float getMasterPeakLeft() const noexcept;
    [[nodiscard]] float getMasterPeakRight() const noexcept;

    juce::AudioDeviceManager& getDeviceManager() noexcept;
    juce::AudioFormatManager& getFormatManager() noexcept;
    juce::AudioThumbnailCache& getThumbnailCache() noexcept;

private:
    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 64 };
    juce::MixerAudioSource mixer;
    std::vector<std::unique_ptr<AudioTrack>> tracks;

    std::atomic<float> masterGain { 0.8f };
    std::atomic<float> masterPeakLeft { 0.0f };
    std::atomic<float> masterPeakRight { 0.0f };

    juce::AudioSourcePlayer sourcePlayer;
    juce::AudioDeviceManager deviceManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
