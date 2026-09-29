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
    void stop();
    void setMasterGain(float gain);

    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] double getPositionSeconds() const;
    [[nodiscard]] double getLengthSeconds() const;
    [[nodiscard]] int getTrackCount() const noexcept;

    juce::AudioDeviceManager& getDeviceManager() noexcept;

private:
    juce::AudioFormatManager formatManager;
    juce::MixerAudioSource mixer;
    std::vector<std::unique_ptr<AudioTrack>> tracks;

    std::atomic<float> masterGain { 0.8f };

    juce::AudioSourcePlayer sourcePlayer;
    juce::AudioDeviceManager deviceManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
