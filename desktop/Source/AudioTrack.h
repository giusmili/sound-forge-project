#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class AudioTrack final : public juce::AudioSource
{
public:
    AudioTrack(
        juce::File sourceFile,
        juce::String trackName,
        std::unique_ptr<juce::AudioFormatReaderSource> source,
        double sourceSampleRate,
        juce::TimeSliceThread& readAheadThread
    );

    ~AudioTrack() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void setPositionSeconds(double seconds);
    [[nodiscard]] bool isReady(int outputSamples);
    void clearPeaks() noexcept;

    void setGain(float newGain);
    void setPan(float newPan);
    void setMuted(bool shouldBeMuted);
    void setSolo(bool shouldBeSolo);
    void setSoloMuted(bool shouldBeSoloMuted);

    [[nodiscard]] float getGain() const noexcept;
    [[nodiscard]] float getPan() const noexcept;
    [[nodiscard]] bool isMuted() const noexcept;
    [[nodiscard]] bool isSolo() const noexcept;
    [[nodiscard]] float getPeakLeft() const noexcept;
    [[nodiscard]] float getPeakRight() const noexcept;
    [[nodiscard]] double getLengthSeconds() const;
    [[nodiscard]] const juce::String& getName() const noexcept;
    [[nodiscard]] const juce::File& getSourceFile() const noexcept;

private:
    juce::File file;
    juce::String name;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    const double sourceRate;
    const double lengthSeconds;
    // Declaration order is ownership order: the resampler and buffer must be
    // destroyed before the reader and the engine's read-ahead thread.
    juce::BufferingAudioSource bufferedSource;
    juce::ResamplingAudioSource resampler;
    double outputRate = 0.0;
    bool prepared = false;

    std::atomic<float> gain { 1.0f };
    std::atomic<float> pan { 0.0f };
    std::atomic<bool> muted { false };
    std::atomic<bool> solo { false };
    std::atomic<bool> soloMuted { false };
    std::atomic<float> peakLeft { 0.0f };
    std::atomic<float> peakRight { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioTrack)
};
