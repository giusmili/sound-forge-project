#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class AudioEngine final
{
public:
    AudioEngine();
    ~AudioEngine();

    juce::Result loadFile(const juce::File& file);

    void play();
    void stop();
    void setGain(float gain);
    void setPositionSeconds(double seconds);

    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] double getPositionSeconds() const;
    [[nodiscard]] double getLengthSeconds() const;

    juce::AudioDeviceManager& getDeviceManager() noexcept;

private:
    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transport;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    juce::AudioSourcePlayer sourcePlayer;
    juce::AudioDeviceManager deviceManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
