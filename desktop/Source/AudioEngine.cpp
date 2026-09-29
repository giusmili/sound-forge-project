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

    sourcePlayer.setSource(this);
    deviceManager.addAudioCallback(&sourcePlayer);
}

AudioEngine::~AudioEngine()
{
    deviceManager.removeAudioCallback(&sourcePlayer);
    sourcePlayer.setSource(nullptr);

    mixer.removeAllInputs();
    tracks.clear();
}

void AudioEngine::prepareToPlay(
    const int samplesPerBlockExpected,
    const double sampleRate
)
{
    mixer.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AudioEngine::releaseResources()
{
    mixer.releaseResources();
}

void AudioEngine::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill
)
{
    mixer.getNextAudioBlock(bufferToFill);

    bufferToFill.buffer->applyGain(
        bufferToFill.startSample,
        bufferToFill.numSamples,
        juce::jlimit(0.0f, 1.0f, masterGain.load())
    );
}

juce::Result AudioEngine::addTrackFromFile(
    const juce::File& file,
    AudioTrack*& createdTrack
)
{
    createdTrack = nullptr;

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file)
    );

    if (reader == nullptr)
        return juce::Result::fail(
            "Format audio non pris en charge ou fichier illisible."
        );

    const auto sourceSampleRate = reader->sampleRate;

    auto readerSource = std::make_unique<juce::AudioFormatReaderSource>(
        reader.release(),
        true
    );

    auto track = std::make_unique<AudioTrack>(
        file.getFileNameWithoutExtension(),
        std::move(readerSource),
        sourceSampleRate
    );

    createdTrack = track.get();
    tracks.push_back(std::move(track));

    mixer.addInputSource(createdTrack, false);

    return juce::Result::ok();
}

void AudioEngine::play()
{
    for (auto& track : tracks)
        track->play();
}

void AudioEngine::stop()
{
    for (auto& track : tracks)
        track->stop();
}

void AudioEngine::setMasterGain(const float gain)
{
    masterGain.store(juce::jlimit(0.0f, 1.0f, gain));
}

bool AudioEngine::isPlaying() const
{
    for (const auto& track : tracks)
    {
        if (track->isPlaying())
            return true;
    }

    return false;
}

double AudioEngine::getPositionSeconds() const
{
    double position = 0.0;

    for (const auto& track : tracks)
        position = juce::jmax(position, track->getPositionSeconds());

    return position;
}

double AudioEngine::getLengthSeconds() const
{
    double length = 0.0;

    for (const auto& track : tracks)
        length = juce::jmax(length, track->getLengthSeconds());

    return length;
}

int AudioEngine::getTrackCount() const noexcept
{
    return static_cast<int>(tracks.size());
}

juce::AudioDeviceManager& AudioEngine::getDeviceManager() noexcept
{
    return deviceManager;
}
