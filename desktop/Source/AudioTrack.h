#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class AudioTrack final : public juce::AudioSource
{
public:
    AudioTrack(
        juce::String trackName,
        std::unique_ptr<juce::AudioFormatReaderSource> source,
        double sourceSampleRate
    );

    ~AudioTrack() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void play();
    void stop();

    void setGain(float newGain);
    void setPan(float newPan);
    void setMuted(bool shouldBeMuted);

    [[nodiscard]] float getGain() const noexcept;
    [[nodiscard]] float getPan() const noexcept;
    [[nodiscard]] bool isMuted() const noexcept;
    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] double getPositionSeconds() const;
    [[nodiscard]] double getLengthSeconds() const;
    [[nodiscard]] const juce::String& getName() const noexcept;

private:
    juce::String name;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transport;

    std::atomic<float> gain { 1.0f };
    std::atomic<float> pan { 0.0f };
    std::atomic<bool> muted { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioTrack)
};
