#include "AudioEngine.h"
#include "AppServices.h"

#include <algorithm>
#include <cmath>

AudioEngine::AudioEngine(const bool openAudioDevice)
{
    formatManager.registerBasicFormats();
    if (! readAheadThread.startThread())
        AppServices::log("ERROR: unable to start audio file reader thread");

    if (! openAudioDevice)
        return; // Same renderer, deterministic tests without physical hardware.

    deviceManager = std::make_unique<juce::AudioDeviceManager>();
    const auto settings = AppServices::loadSettings();
    const auto xml = juce::parseXML(settings.getProperty("audioDeviceState", "").toString());
    const auto error = deviceManager->initialise(0, 2, xml.get(), true);
    deviceOpened = true;
    sourcePlayer.setSource(this);
    deviceManager->addAudioCallback(&sourcePlayer);
    deviceManager->addChangeListener(this);
    changeListenerCallback(deviceManager.get());
    if (error.isNotEmpty())
    {
        audioStatus = "Audio indisponible : " + error;
        AppServices::log("ERROR initialise: " + error);
    }
}

AudioEngine::~AudioEngine()
{
    if (deviceManager != nullptr)
    {
        deviceManager->removeChangeListener(this);
        if (deviceOpened)
            deviceManager->removeAudioCallback(&sourcePlayer);
    }
    sourcePlayer.setSource(nullptr);
    if (deviceManager != nullptr)
        deviceManager->closeAudioDevice();
    releaseResources();
    // Clients are removed before their thread and readers are destroyed.
    tracks.clear();
    readAheadThread.stopThread(-1);
}

void AudioEngine::prepareToPlay(const int expectedBlockSize, const double sampleRate)
{
    const juce::ScopedLock lock(stateLock);
    playing = false; // Device restart never unexpectedly resumes playback.
    prepared = false;
    preparationError.clear();
    clearPeaksLocked();
    if (! std::isfinite(sampleRate) || sampleRate < 8000.0 || sampleRate > 384000.0
        || expectedBlockSize <= 0)
    {
        preparationError = "Configuration audio invalide.";
        return;
    }

    try
    {
        outputRate = sampleRate;
        // Chunk variable/large device callbacks to keep all render buffers bounded.
        renderBlockSize = juce::jlimit(1, 2048, expectedBlockSize);
        trackBuffer.setSize(2, renderBlockSize);
        for (auto& track : tracks)
        {
            track->prepareToPlay(renderBlockSize, outputRate);
            track->setPositionSeconds(positionSeconds);
        }
        prepared = true;
    }
    catch (const std::exception& exception)
    {
        releaseResources();
        preparationError = "Preparation audio impossible : " + juce::String(exception.what());
        AppServices::log("ERROR: " + preparationError);
        return;
    }
    AppServices::log("Audio prepared: " + juce::String(sampleRate) + " Hz, "
                     + juce::String(expectedBlockSize) + " samples");
}

void AudioEngine::releaseResources()
{
    const juce::ScopedLock lock(stateLock);
    playing = false;
    prepared = false;
    for (auto& track : tracks)
        track->releaseResources();
    clearPeaksLocked();
    outputRate = 0.0;
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    if (info.buffer == nullptr || info.numSamples <= 0)
        return;
    info.clearActiveBufferRegion();
    if (info.buffer->getNumChannels() == 0)
        return;

    // UI edits may prepare a reader or seek. Emit silence instead of waiting
    // for those operations on the real-time thread.
    const juce::ScopedTryLock lock(stateLock);
    if (! lock.isLocked())
        return;
    if (! prepared || ! playing)
    {
        clearPeaksLocked();
        return;
    }

    for (int offset = 0; offset < info.numSamples;)
    {
        const auto remaining = static_cast<int>(juce::jmin(
            static_cast<double>(renderBlockSize),
            std::ceil(juce::jmax(0.0, lengthSeconds - positionSeconds) * outputRate)));
        const auto count = juce::jmin(info.numSamples - offset, remaining);
        if (count <= 0)
        {
            playing = false;
            positionSeconds = lengthSeconds;
            break;
        }

        const auto ready = std::all_of(tracks.begin(), tracks.end(), [this, count](auto& track)
        {
            return positionSeconds >= track->getLengthSeconds() || track->isReady(count);
        });
        if (! ready)
        {
            ++readAheadUnderruns;
            clearPeaksLocked();
            break; // No track or project clock advances on a cache miss.
        }

        for (auto& track : tracks)
        {
            if (positionSeconds >= track->getLengthSeconds())
            {
                track->clearPeaks();
                continue;
            }
            track->getNextAudioBlock({ &trackBuffer, 0, count });
            if (info.buffer->getNumChannels() == 1)
            {
                info.buffer->addFrom(0, info.startSample + offset, trackBuffer, 0, 0, count, 0.5f);
                info.buffer->addFrom(0, info.startSample + offset, trackBuffer, 1, 0, count, 0.5f);
            }
            else
            {
                for (int channel = 0; channel < 2; ++channel)
                    info.buffer->addFrom(channel, info.startSample + offset, trackBuffer, channel, 0, count);
            }
        }
        positionSeconds = juce::jmin(lengthSeconds, positionSeconds + count / outputRate);
        offset += count;
        if (positionSeconds >= lengthSeconds - 0.5 / outputRate)
        {
            positionSeconds = lengthSeconds;
            playing = false;
            break;
        }
    }

    const auto gain = masterGain.load();
    // Reject non-finite decoder samples and prevent an over-range sum reaching
    // the device. There are deliberately no allocations, logs or file reads here.
    for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel)
    {
        auto* samples = info.buffer->getWritePointer(channel, info.startSample);
        for (int i = 0; i < info.numSamples; ++i)
        {
            const auto value = samples[i] * gain;
            samples[i] = std::isfinite(value) ? juce::jlimit(-1.0f, 1.0f, value) : 0.0f;
        }
    }
    masterPeakLeft.store(info.buffer->getMagnitude(0, info.startSample, info.numSamples));
    masterPeakRight.store(info.buffer->getNumChannels() > 1
        ? info.buffer->getMagnitude(1, info.startSample, info.numSamples) : masterPeakLeft.load());
}

juce::Result AudioEngine::addTrackFromFile(const juce::File& file, AudioTrack*& createdTrack)
{
    createdTrack = nullptr;
    AppServices::log("Import requested: " + file.getFullPathName());
    const auto fail = [](const juce::String& message)
    {
        AppServices::log("ERROR import: " + message);
        return juce::Result::fail(message);
    };
    if (! readAheadThread.isThreadRunning())
        return fail("Le thread de lecture audio est indisponible.");
    if (! file.existsAsFile())
        return fail("Le fichier n'existe pas.");

    try
    {
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr)
            return fail("Format audio non pris en charge ou fichier illisible.");
        if (! std::isfinite(reader->sampleRate) || reader->sampleRate < 8000.0
            || reader->sampleRate > 384000.0 || reader->numChannels == 0
            || reader->numChannels > 2 || reader->lengthInSamples <= 0
            || reader->lengthInSamples / reader->sampleRate > 86400.0)
            return fail("Flux audio invalide ou hors limites : mono/stereo, 8 a 384 kHz, 24 h maximum.");

        const auto sourceRate = reader->sampleRate;
        AppServices::log("Import metadata: " + juce::String(sourceRate) + " Hz, "
                         + juce::String(reader->numChannels) + " channels, "
                         + juce::String(reader->lengthInSamples) + " samples");
        // Allocation/decoder failures occur before the live session is mutated.
        auto source = std::make_unique<juce::AudioFormatReaderSource>(reader.get(), true);
        reader.release(); // Ownership transfers only after allocation succeeds.
        auto track = std::make_unique<AudioTrack>(file, file.getFileNameWithoutExtension(),
                                                std::move(source), sourceRate, readAheadThread);
        const juce::ScopedLock lock(stateLock);
        tracks.reserve(tracks.size() + 1);
        if (prepared)
            track->prepareToPlay(renderBlockSize, outputRate);
        track->setPositionSeconds(0.0);
        // Import deliberately stops and rewinds the session, without detaching
        // the device callback or waiting for it to acknowledge stop().
        playing = false;
        seekLocked(0.0);
        auto* result = track.get();
        tracks.push_back(std::move(track));
        lengthSeconds = juce::jmax(lengthSeconds, result->getLengthSeconds());
        refreshSoloState();
        createdTrack = result;
        AppServices::log("Import complete: " + result->getName());
        return juce::Result::ok();
    }
    catch (const std::exception& exception)
    {
        return fail("Impossible de charger la piste : " + juce::String(exception.what()));
    }
}

void AudioEngine::play()
{
    const juce::ScopedLock lock(stateLock);
    if (! canPlay())
        return;
    if (positionSeconds >= lengthSeconds)
        seekLocked(0.0);
    playing = true;
}

void AudioEngine::pause()
{
    const juce::ScopedLock lock(stateLock);
    playing = false;
    clearPeaksLocked();
}

void AudioEngine::stop()
{
    const juce::ScopedLock lock(stateLock);
    playing = false;
    seekLocked(0.0);
}

void AudioEngine::seekLocked(const double seconds)
{
    positionSeconds = std::isfinite(seconds) ? juce::jlimit(0.0, lengthSeconds, seconds) : 0.0;
    for (auto& track : tracks)
        track->setPositionSeconds(positionSeconds);
    clearPeaksLocked();
}

void AudioEngine::setPositionSeconds(const double seconds)
{
    const juce::ScopedLock lock(stateLock);
    seekLocked(seconds);
}

void AudioEngine::clearPeaksLocked()
{
    masterPeakLeft.store(0.0f);
    masterPeakRight.store(0.0f);
    for (auto& track : tracks)
        track->clearPeaks();
}

void AudioEngine::setMasterGain(const float gain)
{
    masterGain.store(std::isfinite(gain) ? juce::jlimit(0.0f, 1.0f, gain) : 0.8f);
}

void AudioEngine::refreshSoloState()
{
    const juce::ScopedLock lock(stateLock);
    const auto anySolo = std::any_of(tracks.begin(), tracks.end(), [](const auto& track)
    {
        return track->isSolo();
    });
    for (auto& track : tracks)
        track->setSoloMuted(anySolo && ! track->isSolo());
}

bool AudioEngine::isPlaying() const
{
    const juce::ScopedLock lock(stateLock);
    return playing;
}

bool AudioEngine::canPlay() const
{
    const juce::ScopedLock lock(stateLock);
    return prepared && lengthSeconds > 0.0 && (! deviceOpened || deviceAvailable);
}

double AudioEngine::getPositionSeconds() const
{
    const juce::ScopedLock lock(stateLock);
    return positionSeconds;
}

double AudioEngine::getLengthSeconds() const
{
    const juce::ScopedLock lock(stateLock);
    return lengthSeconds;
}

int AudioEngine::getTrackCount() const noexcept
{
    const juce::ScopedLock lock(stateLock);
    return static_cast<int>(tracks.size());
}

float AudioEngine::getMasterPeakLeft() const noexcept { return masterPeakLeft.load(); }
float AudioEngine::getMasterPeakRight() const noexcept { return masterPeakRight.load(); }
unsigned int AudioEngine::getReadAheadUnderruns() const noexcept { return readAheadUnderruns.load(); }
juce::AudioDeviceManager& AudioEngine::getDeviceManager() noexcept
{
    // The device-free test path deliberately never creates platform services.
    jassert(deviceManager != nullptr);
    return *deviceManager;
}
juce::AudioFormatManager& AudioEngine::getFormatManager() noexcept { return formatManager; }
juce::AudioThumbnailCache& AudioEngine::getThumbnailCache() noexcept { return thumbnailCache; }
juce::String AudioEngine::getAudioStatus() const
{
    const juce::ScopedLock lock(stateLock);
    return preparationError.isNotEmpty() ? preparationError : audioStatus;
}

void AudioEngine::changeListenerCallback(juce::ChangeBroadcaster*)
{
    // AudioDeviceManager change notifications run on the message thread.
    auto* device = deviceManager->getCurrentAudioDevice();
    const auto available = device != nullptr && device->isOpen() && device->isPlaying()
        && ! device->getActiveOutputChannels().isZero();
    {
        const juce::ScopedLock lock(stateLock);
        deviceAvailable = available;
    }
    if (! available)
    {
        pause();
        audioStatus = "Audio indisponible. Ouvrez Configuration audio.";
        AppServices::log(audioStatus);
        return; // Keep the last usable device setup for the next launch.
    }
    audioStatus = device->getName() + " | " + juce::String(device->getCurrentSampleRate())
                  + " Hz | " + juce::String(device->getCurrentBufferSizeSamples()) + " samples";
    AppServices::log("Device: " + audioStatus);
    auto settings = AppServices::loadSettings();
    if (auto* object = settings.getDynamicObject())
    {
        if (auto xml = deviceManager->createStateXml())
            object->setProperty("audioDeviceState", xml->toString());
        object->setProperty("audioDevice", device->getName());
        object->setProperty("sampleRate", device->getCurrentSampleRate());
        object->setProperty("bufferSize", device->getCurrentBufferSizeSamples());
        AppServices::saveSettings(settings);
    }
}
