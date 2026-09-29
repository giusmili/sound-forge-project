#include "AudioEngine.h"

AudioEngine::AudioEngine()
{
    formatManager.registerBasicFormats();

    const auto error = deviceManager.initialise(
        0,
        2,
        nullptr,
        true
    );

    jassert(error.isEmpty());

    sourcePlayer.setSource(&transport);
    deviceManager.addAudioCallback(&sourcePlayer);
}

AudioEngine::~AudioEngine()
{
    transport.stop();
    transport.setSource(nullptr);

    deviceManager.removeAudioCallback(&sourcePlayer);
    sourcePlayer.setSource(nullptr);

    readerSource.reset();
}

juce::Result AudioEngine::loadFile(const juce::File& file)
{
    stop();
    transport.setSource(nullptr);
    readerSource.reset();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader == nullptr)
        return juce::Result::fail("Format audio non pris en charge ou fichier illisible.");

    const auto sourceSampleRate = reader->sampleRate;
    auto newReaderSource = std::make_unique<juce::AudioFormatReaderSource>(
        reader.release(),
        true
    );

    transport.setSource(
        newReaderSource.get(),
        32768,
        nullptr,
        sourceSampleRate
    );

    readerSource = std::move(newReaderSource);
    transport.setPosition(0.0);

    return juce::Result::ok();
}

void AudioEngine::play()
{
    if (readerSource != nullptr)
        transport.start();
}

void AudioEngine::stop()
{
    transport.stop();
    transport.setPosition(0.0);
}

void AudioEngine::setGain(const float gain)
{
    transport.setGain(juce::jlimit(0.0f, 1.0f, gain));
}

void AudioEngine::setPositionSeconds(const double seconds)
{
    transport.setPosition(juce::jlimit(0.0, getLengthSeconds(), seconds));
}

bool AudioEngine::isPlaying() const
{
    return transport.isPlaying();
}

double AudioEngine::getPositionSeconds() const
{
    return transport.getCurrentPosition();
}

double AudioEngine::getLengthSeconds() const
{
    return transport.getLengthInSeconds();
}

juce::AudioDeviceManager& AudioEngine::getDeviceManager() noexcept
{
    return deviceManager;
}
