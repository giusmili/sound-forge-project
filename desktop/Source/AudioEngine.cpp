#include "AudioEngine.h"
#include "AppServices.h"

#include <algorithm>

AudioEngine::AudioEngine()
{
    AppServices::log("AudioEngine: registering audio formats");
    formatManager.registerBasicFormats();

    AppServices::log("AudioEngine: initialising default output device");
    const auto error = deviceManager.initialise(
        0,
        2,
        nullptr,
        true
    );

    if (error.isNotEmpty())
        AppServices::log("ERROR AudioEngine initialise: " + error);
    else
        AppServices::log("AudioEngine: device initialised");

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

    const auto leftPeak = bufferToFill.buffer->getNumChannels() > 0
        ? bufferToFill.buffer->getMagnitude(0, bufferToFill.startSample, bufferToFill.numSamples)
        : 0.0f;
    const auto rightPeak = bufferToFill.buffer->getNumChannels() > 1
        ? bufferToFill.buffer->getMagnitude(1, bufferToFill.startSample, bufferToFill.numSamples)
        : leftPeak;
    masterPeakLeft.store(leftPeak);
    masterPeakRight.store(rightPeak);
}

juce::Result AudioEngine::addTrackFromFile(
    const juce::File& file,
    AudioTrack*& createdTrack
)
{
    createdTrack = nullptr;
    AppServices::log("Import requested: " + file.getFullPathName());

    if (! file.existsAsFile())
    {
        AppServices::log("ERROR import: file does not exist");
        return juce::Result::fail("Le fichier n'existe pas.");
    }

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file)
    );

    if (reader == nullptr)
        return juce::Result::fail(
            "Format audio non pris en charge ou fichier illisible."
        );

    if (reader->sampleRate <= 0.0
        || reader->numChannels <= 0
        || reader->lengthInSamples <= 0)
    {
        return juce::Result::fail(
            "Le fichier audio ne contient pas de flux exploitable."
        );
    }

    const auto sourceSampleRate = reader->sampleRate;

    // Importing mutates the MixerAudioSource input list. Keep the audio
    // callback detached while the new transport is created and registered.
    // This deliberately favours stability over seamless hot-import.
    stop();
    deviceManager.removeAudioCallback(&sourcePlayer);

    auto readerSource = std::make_unique<juce::AudioFormatReaderSource>(
        reader.release(),
        true
    );

    auto track = std::make_unique<AudioTrack>(
        file,
        file.getFileNameWithoutExtension(),
        std::move(readerSource),
        sourceSampleRate
    );

    createdTrack = track.get();
    tracks.push_back(std::move(track));
    AppServices::log("AudioTrack created: " + createdTrack->getName());

    mixer.addInputSource(createdTrack, false);
    AppServices::log("AudioTrack added to mixer");
    refreshSoloState();

    deviceManager.addAudioCallback(&sourcePlayer);

    return juce::Result::ok();
}

void AudioEngine::play()
{
    const auto length = getLengthSeconds();

    if (length <= 0.0)
        return;

    if (getPositionSeconds() >= length - 0.001)
        setPositionSeconds(0.0);

    for (auto& track : tracks)
        track->play();
}

void AudioEngine::pause()
{
    for (auto& track : tracks)
        track->pause();
}

void AudioEngine::stop()
{
    for (auto& track : tracks)
        track->stop();
}

void AudioEngine::setPositionSeconds(const double seconds)
{
    const auto position = juce::jlimit(
        0.0,
        getLengthSeconds(),
        seconds
    );

    for (auto& track : tracks)
        track->setPositionSeconds(position);
}

void AudioEngine::setMasterGain(const float gain)
{
    masterGain.store(juce::jlimit(0.0f, 1.0f, gain));
}

void AudioEngine::refreshSoloState()
{
    const auto anySolo = std::any_of(
        tracks.begin(),
        tracks.end(),
        [](const auto& track)
        {
            return track->isSolo();
        }
    );

    for (auto& track : tracks)
        track->setSoloMuted(anySolo && ! track->isSolo());
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

float AudioEngine::getMasterPeakLeft() const noexcept
{
    return masterPeakLeft.load();
}

float AudioEngine::getMasterPeakRight() const noexcept
{
    return masterPeakRight.load();
}

juce::AudioDeviceManager& AudioEngine::getDeviceManager() noexcept
{
    return deviceManager;
}

juce::AudioFormatManager& AudioEngine::getFormatManager() noexcept
{
    return formatManager;
}

juce::AudioThumbnailCache& AudioEngine::getThumbnailCache() noexcept
{
    return thumbnailCache;
}
