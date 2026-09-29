#include "AudioEngine.h"

#include <algorithm>

AudioEngine::AudioEngine()
{
    formatManager.registerBasicFormats();
    recordingThread.startThread();

    auto error = deviceManager.initialise(
        1,
        2,
        nullptr,
        true
    );

    if (error.isNotEmpty())
    {
        error = deviceManager.initialise(
            0,
            2,
            nullptr,
            true
        );
    }

    jassert(error.isEmpty());

    deviceManager.addAudioCallback(this);
}

AudioEngine::~AudioEngine()
{
    deviceManager.removeAudioCallback(this);

    activeWriter.store(nullptr);
    threadedWriter.reset();
    recording.store(false);
    recordingThread.stopThread(2000);

    const juce::ScopedLock lock(trackLock);
    mixer.removeAllInputs();
    tracks.clear();
    midiTracks.clear();
}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    const int numInputChannels,
    float* const* outputChannelData,
    const int numOutputChannels,
    const int numSamples,
    const juce::AudioIODeviceCallbackContext&
)
{
    if (numOutputChannels > 0)
    {
        juce::AudioBuffer<float> outputBuffer(
            outputChannelData,
            numOutputChannels,
            numSamples
        );

        outputBuffer.clear();

        juce::AudioSourceChannelInfo info(
            &outputBuffer,
            0,
            numSamples
        );

        getNextAudioBlock(info);
    }

    if (recording.load())
    {
        if (auto* writer = activeWriter.load())
        {
            if (numInputChannels > 0)
                writer->write(inputChannelData, numSamples);
        }
    }
}

void AudioEngine::audioDeviceAboutToStart(
    juce::AudioIODevice* device
)
{
    if (device == nullptr)
        return;

    inputSampleRate.store(
        device->getCurrentSampleRate()
    );

    activeInputChannels.store(
        device->getActiveInputChannels()
            .countNumberOfSetBits()
    );

    prepareToPlay(
        device->getCurrentBufferSizeSamples(),
        device->getCurrentSampleRate()
    );
}

void AudioEngine::audioDeviceStopped()
{
    inputSampleRate.store(0.0);
    activeInputChannels.store(0);
    releaseResources();
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

    float masterPeak = 0.0f;

    for (int channel = 0;
         channel < bufferToFill.buffer->getNumChannels();
         ++channel)
    {
        masterPeak = juce::jmax(
            masterPeak,
            bufferToFill.buffer->getMagnitude(
                channel,
                bufferToFill.startSample,
                bufferToFill.numSamples
            )
        );
    }

    masterPeakLevel.store(
        juce::jlimit(0.0f, 1.5f, masterPeak)
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

    if (nextPosition >= projectLength
        && ! recording.load())
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

juce::Result AudioEngine::splitTrackAtProjectPosition(
    AudioTrack& sourceTrack,
    const double projectPositionSeconds,
    AudioTrack*& rightTrack
)
{
    rightTrack = nullptr;

    const auto clipStart =
        sourceTrack.getStartOffsetSeconds();

    const auto clipDuration =
        sourceTrack.getClipDurationSeconds();

    const auto clipEnd =
        clipStart + clipDuration;

    constexpr double minimumFragmentSeconds = 0.01;

    if (projectPositionSeconds
            <= clipStart + minimumFragmentSeconds
        || projectPositionSeconds
            >= clipEnd - minimumFragmentSeconds)
    {
        return juce::Result::fail(
            "Placez le playhead a l'interieur du clip, loin de ses bords."
        );
    }

    const auto sourceSplit =
        sourceTrack.getSourceStartSeconds()
        + (projectPositionSeconds - clipStart);

    const auto originalSourceEnd =
        sourceTrack.getSourceEndSeconds();

    const auto result = addTrackFromFile(
        sourceTrack.getSourceFile(),
        rightTrack
    );

    if (result.failed() || rightTrack == nullptr)
        return result;

    rightTrack->setStartOffsetSeconds(
        projectPositionSeconds
    );

    rightTrack->setSourceRange(
        sourceSplit,
        originalSourceEnd
    );

    rightTrack->setGain(sourceTrack.getGain());
    rightTrack->setPan(sourceTrack.getPan());
    rightTrack->setMuted(sourceTrack.isMuted());
    rightTrack->setSolo(sourceTrack.isSolo());

    sourceTrack.setSourceRange(
        sourceTrack.getSourceStartSeconds(),
        sourceSplit
    );

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

MidiInstrumentTrack*
AudioEngine::addMidiInstrumentTrack(
    const juce::String& name
)
{
    auto track =
        std::make_unique<MidiInstrumentTrack>(
            name.isNotEmpty()
                ? name
                : "Instrument"
        );

    const juce::ScopedLock lock(trackLock);

    auto* createdTrack = track.get();
    midiTracks.push_back(std::move(track));
    mixer.addInputSource(createdTrack, false);

    return createdTrack;
}

bool AudioEngine::removeMidiInstrumentTrack(
    MidiInstrumentTrack* trackToRemove
)
{
    if (trackToRemove == nullptr)
        return false;

    const juce::ScopedLock lock(trackLock);

    const auto iterator = std::find_if(
        midiTracks.begin(),
        midiTracks.end(),
        [trackToRemove](const auto& track)
        {
            return track.get() == trackToRemove;
        }
    );

    if (iterator == midiTracks.end())
        return false;

    mixer.removeInputSource(trackToRemove);
    midiTracks.erase(iterator);

    if (tracks.empty()
        && midiTracks.empty())
    {
        playing.store(false);
        projectPositionSeconds.store(0.0);
    }

    return true;
}

std::vector<AudioEngine::TrackState>
AudioEngine::captureTrackStates() const
{
    const juce::ScopedLock lock(trackLock);

    std::vector<TrackState> states;
    states.reserve(tracks.size());

    for (const auto& track : tracks)
    {
        TrackState state;
        state.sourceFile = track->getSourceFile();
        state.startOffsetSeconds = track->getStartOffsetSeconds();
        state.sourceStartSeconds = track->getSourceStartSeconds();
        state.sourceEndSeconds = track->getSourceEndSeconds();
        state.gain = track->getGain();
        state.pan = track->getPan();
        state.muted = track->isMuted();
        state.solo = track->isSolo();

        states.push_back(std::move(state));
    }

    return states;
}

std::vector<AudioEngine::MidiTrackState>
AudioEngine::captureMidiTrackStates() const
{
    const juce::ScopedLock lock(trackLock);

    std::vector<MidiTrackState> states;
    states.reserve(midiTracks.size());

    for (const auto& track : midiTracks)
    {
        MidiTrackState state;
        state.name = track->getName();
        state.gain = track->getGain();
        state.muted = track->isMuted();
        state.instrumentId =
            track->getInstrumentId();

        states.push_back(std::move(state));
    }

    return states;
}

juce::Result AudioEngine::restoreProjectTracks(
    const std::vector<TrackState>& audioStates,
    const std::vector<MidiTrackState>& midiStates
)
{
    std::vector<std::unique_ptr<AudioTrack>> rebuiltTracks;
    rebuiltTracks.reserve(audioStates.size());

    for (const auto& state : audioStates)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(state.sourceFile)
        );

        if (reader == nullptr)
        {
            return juce::Result::fail(
                "Impossible de restaurer le fichier audio : "
                + state.sourceFile.getFullPathName()
            );
        }

        const auto sourceSampleRate = reader->sampleRate;

        auto readerSource =
            std::make_unique<juce::AudioFormatReaderSource>(
                reader.release(),
                true
            );

        auto track = std::make_unique<AudioTrack>(
            state.sourceFile,
            state.sourceFile.getFileNameWithoutExtension(),
            std::move(readerSource),
            sourceSampleRate
        );

        track->setStartOffsetSeconds(
            state.startOffsetSeconds
        );
        track->setSourceRange(
            state.sourceStartSeconds,
            state.sourceEndSeconds
        );
        track->setGain(state.gain);
        track->setPan(state.pan);
        track->setMuted(state.muted);
        track->setSolo(state.solo);

        rebuiltTracks.push_back(std::move(track));
    }

    std::vector<std::unique_ptr<MidiInstrumentTrack>>
        rebuiltMidiTracks;

    rebuiltMidiTracks.reserve(midiStates.size());

    for (const auto& state : midiStates)
    {
        auto track =
            std::make_unique<MidiInstrumentTrack>(
                state.name
            );

        track->setGain(state.gain);
        track->setMuted(state.muted);

        rebuiltMidiTracks.push_back(
            std::move(track)
        );
    }

    const juce::ScopedLock lock(trackLock);

    playing.store(false);
    mixer.removeAllInputs();
    tracks.clear();
    midiTracks.clear();

    tracks = std::move(rebuiltTracks);
    midiTracks = std::move(rebuiltMidiTracks);

    for (auto& track : tracks)
        mixer.addInputSource(track.get(), false);

    for (auto& track : midiTracks)
        mixer.addInputSource(track.get(), false);

    refreshSoloStateUnlocked();

    const auto length = getLengthSecondsUnlocked();
    const auto position = juce::jlimit(
        0.0,
        length,
        projectPositionSeconds.load()
    );

    projectPositionSeconds.store(position);
    syncTracksUnlocked(position, false);

    return juce::Result::ok();
}

juce::Result AudioEngine::restoreTrackStates(
    const std::vector<TrackState>& states
)
{
    return restoreProjectTracks(
        states,
        {}
    );
}

std::vector<AudioTrack*> AudioEngine::getTrackPointers() const
{
    const juce::ScopedLock lock(trackLock);

    std::vector<AudioTrack*> result;
    result.reserve(tracks.size());

    for (const auto& track : tracks)
        result.push_back(track.get());

    return result;
}

std::vector<MidiInstrumentTrack*>
AudioEngine::getMidiTrackPointers() const
{
    const juce::ScopedLock lock(trackLock);

    std::vector<MidiInstrumentTrack*> result;
    result.reserve(midiTracks.size());

    for (const auto& track : midiTracks)
        result.push_back(track.get());

    return result;
}

juce::Result AudioEngine::startRecording(
    const juce::File& file
)
{
    if (recording.load())
    {
        return juce::Result::fail(
            "Un enregistrement est deja en cours."
        );
    }

    const auto sampleRate =
        inputSampleRate.load();

    const auto channels =
        activeInputChannels.load();

    if (sampleRate <= 0.0 || channels <= 0)
    {
        return juce::Result::fail(
            "Aucune entree audio active. Configurez une entree dans Audio."
        );
    }

    const auto parent = file.getParentDirectory();

    if (! parent.exists()
        && ! parent.createDirectory())
    {
        return juce::Result::fail(
            "Impossible de creer le dossier d'enregistrement."
        );
    }

    if (file.existsAsFile()
        && ! file.deleteFile())
    {
        return juce::Result::fail(
            "Impossible de remplacer le fichier d'enregistrement."
        );
    }

    auto stream = file.createOutputStream();

    if (stream == nullptr)
    {
        return juce::Result::fail(
            "Impossible de creer le fichier WAV."
        );
    }

    juce::WavAudioFormat wavFormat;

    auto* writer = wavFormat.createWriterFor(
        stream.get(),
        sampleRate,
        static_cast<unsigned int>(channels),
        24,
        {},
        0
    );

    if (writer == nullptr)
    {
        return juce::Result::fail(
            "Impossible d'initialiser l'ecriture WAV."
        );
    }

    stream.release();

    threadedWriter =
        std::make_unique<
            juce::AudioFormatWriter::ThreadedWriter
        >(
            writer,
            recordingThread,
            32768
        );

    recordingStartPosition.store(
        projectPositionSeconds.load()
    );

    activeWriter.store(threadedWriter.get());
    recording.store(true);
    playing.store(true);

    const juce::ScopedLock lock(trackLock);

    syncTracksUnlocked(
        projectPositionSeconds.load(),
        true
    );

    return juce::Result::ok();
}

void AudioEngine::stopRecording()
{
    activeWriter.store(nullptr);
    recording.store(false);
    threadedWriter.reset();
    playing.store(false);

    const juce::ScopedLock lock(trackLock);

    syncTracksUnlocked(
        projectPositionSeconds.load(),
        false
    );
}

bool AudioEngine::isRecording() const noexcept
{
    return recording.load();
}

double AudioEngine::getRecordingStartPosition() const noexcept
{
    return recordingStartPosition.load();
}

bool AudioEngine::hasAudioInput() const noexcept
{
    return inputSampleRate.load() > 0.0
        && activeInputChannels.load() > 0;
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
    if (recording.load())
        return;

    playing.store(false);

    const juce::ScopedLock lock(trackLock);
    syncTracksUnlocked(
        projectPositionSeconds.load(),
        false
    );
}

void AudioEngine::stop()
{
    if (recording.load())
        stopRecording();

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

float AudioEngine::getMasterGain() const noexcept
{
    return masterGain.load();
}

float AudioEngine::getMasterPeakLevel() const noexcept
{
    return masterPeakLevel.load();
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

    return static_cast<int>(
        tracks.size() + midiTracks.size()
    );
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
