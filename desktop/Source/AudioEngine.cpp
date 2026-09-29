#include "AudioEngine.h"

#include <algorithm>

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

    const juce::ScopedLock lock(trackLock);
    mixer.removeAllInputs();
    tracks.clear();
}

void AudioEngine::prepareToPlay(
    const int samplesPerBlockExpected,
    const double sampleRate
)
{
    outputSampleRate.store(sampleRate);

    const juce::ScopedLock lock(trackLock);
    mixer.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AudioEngine::releaseResources()
{
    const juce::ScopedLock lock(trackLock);
    mixer.releaseResources();
}

void AudioEngine::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill
)
{
    const juce::ScopedLock lock(trackLock);

    const auto currentPosition =
        projectPositionSeconds.load();

    const auto projectPlaying =
        playing.load();

    syncTracksUnlocked(
        currentPosition,
        projectPlaying
    );

    mixer.getNextAudioBlock(bufferToFill);

    bufferToFill.buffer->applyGain(
        bufferToFill.startSample,
        bufferToFill.numSamples,
        juce::jlimit(0.0f, 1.0f, masterGain.load())
    );

    if (! projectPlaying)
        return;

    const auto sampleRate = outputSampleRate.load();

    if (sampleRate <= 0.0)
        return;

    const auto nextPosition =
        currentPosition
        + static_cast<double>(bufferToFill.numSamples)
            / sampleRate;

    const auto projectLength =
        getLengthSecondsUnlocked();

    if (nextPosition >= projectLength)
    {
        projectPositionSeconds.store(projectLength);
        playing.store(false);
        syncTracksUnlocked(projectLength, false);
    }
    else
    {
        projectPositionSeconds.store(nextPosition);
    }
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
    {
        return juce::Result::fail(
            "Format audio non pris en charge ou fichier illisible."
        );
    }

    const auto sourceSampleRate = reader->sampleRate;

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

    const juce::ScopedLock lock(trackLock);

    createdTrack = track.get();
    tracks.push_back(std::move(track));

    mixer.addInputSource(createdTrack, false);
    refreshSoloStateUnlocked();

    createdTrack->syncToProjectPosition(
        projectPositionSeconds.load(),
        playing.load()
    );

    return juce::Result::ok();
}

juce::Result AudioEngine::duplicateTrack(
    const AudioTrack& sourceTrack,
    AudioTrack*& createdTrack
)
{
    const auto result = addTrackFromFile(
        sourceTrack.getSourceFile(),
        createdTrack
    );

    if (result.failed() || createdTrack == nullptr)
        return result;

    createdTrack->setStartOffsetSeconds(
        sourceTrack.getStartOffsetSeconds()
    );

    createdTrack->setSourceRange(
        sourceTrack.getSourceStartSeconds(),
        sourceTrack.getSourceEndSeconds()
    );

    createdTrack->setGain(sourceTrack.getGain());
    createdTrack->setPan(sourceTrack.getPan());
    createdTrack->setMuted(sourceTrack.isMuted());
    createdTrack->setSolo(sourceTrack.isSolo());

    refreshSoloState();
    refreshTrackAlignment();

    return juce::Result::ok();
}

bool AudioEngine::removeTrack(AudioTrack* trackToRemove)
{
    if (trackToRemove == nullptr)
        return false;

    const juce::ScopedLock lock(trackLock);

    const auto iterator = std::find_if(
        tracks.begin(),
        tracks.end(),
        [trackToRemove](const auto& track)
        {
            return track.get() == trackToRemove;
        }
    );

    if (iterator == tracks.end())
        return false;

    mixer.removeInputSource(trackToRemove);
    tracks.erase(iterator);

    refreshSoloStateUnlocked();

    const auto newLength = getLengthSecondsUnlocked();

    if (tracks.empty())
    {
        playing.store(false);
        projectPositionSeconds.store(0.0);
    }
    else if (projectPositionSeconds.load() > newLength)
    {
        projectPositionSeconds.store(newLength);
    }

    syncTracksUnlocked(
        projectPositionSeconds.load(),
        playing.load()
    );

    return true;
}

void AudioEngine::play()
{
    const juce::ScopedLock lock(trackLock);

    const auto length =
        getLengthSecondsUnlocked();

    if (length <= 0.0)
        return;

    auto position =
        projectPositionSeconds.load();

    if (position >= length - 0.001)
    {
        position = 0.0;
        projectPositionSeconds.store(position);
    }

    playing.store(true);
    syncTracksUnlocked(position, true);
}

void AudioEngine::pause()
{
    playing.store(false);

    const juce::ScopedLock lock(trackLock);
    syncTracksUnlocked(
        projectPositionSeconds.load(),
        false
    );
}

void AudioEngine::stop()
{
    playing.store(false);
    projectPositionSeconds.store(0.0);

    const juce::ScopedLock lock(trackLock);
    syncTracksUnlocked(0.0, false);
}

void AudioEngine::setPositionSeconds(const double seconds)
{
    const juce::ScopedLock lock(trackLock);

    const auto position = juce::jlimit(
        0.0,
        getLengthSecondsUnlocked(),
        seconds
    );

    projectPositionSeconds.store(position);

    syncTracksUnlocked(
        position,
        playing.load()
    );
}

void AudioEngine::setMasterGain(const float gain)
{
    masterGain.store(
        juce::jlimit(0.0f, 1.0f, gain)
    );
}

void AudioEngine::refreshSoloState()
{
    const juce::ScopedLock lock(trackLock);
    refreshSoloStateUnlocked();
}

void AudioEngine::refreshTrackAlignment()
{
    const juce::ScopedLock lock(trackLock);

    syncTracksUnlocked(
        projectPositionSeconds.load(),
        playing.load()
    );
}

bool AudioEngine::isPlaying() const noexcept
{
    return playing.load();
}

double AudioEngine::getPositionSeconds() const noexcept
{
    return projectPositionSeconds.load();
}

double AudioEngine::getLengthSeconds() const
{
    const juce::ScopedLock lock(trackLock);
    return getLengthSecondsUnlocked();
}

int AudioEngine::getTrackCount() const noexcept
{
    const juce::ScopedLock lock(trackLock);
    return static_cast<int>(tracks.size());
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

double AudioEngine::getLengthSecondsUnlocked() const
{
    double length = 0.0;

    for (const auto& track : tracks)
    {
        length = juce::jmax(
            length,
            track->getProjectEndSeconds()
        );
    }

    return length;
}

void AudioEngine::syncTracksUnlocked(
    const double projectPosition,
    const bool projectPlaying
)
{
    for (auto& track : tracks)
    {
        track->syncToProjectPosition(
            projectPosition,
            projectPlaying
        );
    }
}

void AudioEngine::refreshSoloStateUnlocked()
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
