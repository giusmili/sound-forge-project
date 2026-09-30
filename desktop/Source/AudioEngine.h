#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "AudioTrack.h"

// Control operations run on the message thread. Device callbacks may run on
// another thread. No control operation waits for an audio callback to complete
// a transport command; one clock controls the entire session.
class AudioEngine final : public juce::AudioSource,
                          private juce::ChangeListener
{
public:
    explicit AudioEngine(bool openAudioDevice = true);
    ~AudioEngine() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;
    juce::Result addTrackFromFile(const juce::File&, AudioTrack*& createdTrack);

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
    [[nodiscard]] juce::String getAudioStatus() const;
    [[nodiscard]] unsigned int getReadAheadUnderruns() const noexcept;

    juce::AudioDeviceManager& getDeviceManager() noexcept;
    juce::AudioFormatManager& getFormatManager() noexcept;
    juce::AudioThumbnailCache& getThumbnailCache() noexcept;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void seekLocked(double seconds);
    void clearPeaksLocked();

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 64 };
    juce::TimeSliceThread readAheadThread { "SonoForge audio file reader" };
    mutable juce::CriticalSection stateLock;
    std::vector<std::unique_ptr<AudioTrack>> tracks;
    juce::AudioBuffer<float> trackBuffer;
    double outputRate = 0.0;
    double positionSeconds = 0.0;
    double lengthSeconds = 0.0;
    int renderBlockSize = 0;
    bool prepared = false;
    bool playing = false;
    bool deviceOpened = false;
    juce::String audioStatus;
    juce::String preparationError;

    std::atomic<float> masterGain { 0.8f };
    std::atomic<float> masterPeakLeft { 0.0f };
    std::atomic<float> masterPeakRight { 0.0f };
    std::atomic<unsigned int> readAheadUnderruns { 0 };
    juce::AudioSourcePlayer sourcePlayer;
    std::unique_ptr<juce::AudioDeviceManager> deviceManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
