#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class AudioTrack final : public juce::AudioSource
{
public:
    AudioTrack(
        juce::File sourceFile,
        juce::String trackName,
        std::unique_ptr<juce::AudioFormatReaderSource> source,
        double sourceSampleRate
    );

    ~AudioTrack() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void syncToProjectPosition(double projectPositionSeconds, bool projectPlaying);

    void setGain(float newGain);
    void setPan(float newPan);
    void setMuted(bool shouldBeMuted);
    void setSolo(bool shouldBeSolo);
    void setSoloMuted(bool shouldBeSoloMuted);
    void setStartOffsetSeconds(double seconds);
    void setSourceRange(double startSeconds, double endSeconds);

    [[nodiscard]] float getGain() const noexcept;
    [[nodiscard]] float getPan() const noexcept;
    [[nodiscard]] float getPeakLevel() const noexcept;
    [[nodiscard]] bool isMuted() const noexcept;
    [[nodiscard]] bool isSolo() const noexcept;
    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] double getPositionSeconds() const;
    [[nodiscard]] double getLengthSeconds() const;
    [[nodiscard]] double getStartOffsetSeconds() const noexcept;
    [[nodiscard]] double getSourceStartSeconds() const noexcept;
    [[nodiscard]] double getSourceEndSeconds() const noexcept;
    [[nodiscard]] double getClipDurationSeconds() const noexcept;
    [[nodiscard]] double getProjectEndSeconds() const noexcept;
    [[nodiscard]] const juce::String& getName() const noexcept;
    [[nodiscard]] const juce::File& getSourceFile() const noexcept;

private:
    juce::File file;
    juce::String name;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transport;

    std::atomic<float> gain { 1.0f };
    std::atomic<float> pan { 0.0f };
    std::atomic<float> peakLevel { 0.0f };
    std::atomic<bool> muted { false };
    std::atomic<bool> solo { false };
    std::atomic<bool> soloMuted { false };
    std::atomic<double> startOffsetSeconds { 0.0 };
    std::atomic<double> sourceStartSeconds { 0.0 };
    std::atomic<double> sourceEndSeconds { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioTrack)
};
