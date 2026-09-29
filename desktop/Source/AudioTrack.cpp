#include "AudioTrack.h"

#include <cmath>

AudioTrack::AudioTrack(
    juce::File sourceFile,
    juce::String trackName,
    std::unique_ptr<juce::AudioFormatReaderSource> source,
    const double sourceSampleRate
)
    : file(std::move(sourceFile)),
      name(std::move(trackName)),
      readerSource(std::move(source))
{
    transport.setSource(
        readerSource.get(),
        32768,
        nullptr,
        sourceSampleRate
    );

    sourceEndSeconds.store(
        transport.getLengthInSeconds()
    );
}

AudioTrack::~AudioTrack()
{
    transport.stop();
    transport.setSource(nullptr);
}

void AudioTrack::prepareToPlay(
    const int samplesPerBlockExpected,
    const double sampleRate
)
{
    transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AudioTrack::releaseResources()
{
    transport.releaseResources();
}

void AudioTrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill
)
{
    transport.getNextAudioBlock(bufferToFill);

    if (muted.load() || soloMuted.load())
    {
        peakLevel.store(0.0f);
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    const auto currentGain = juce::jmax(0.0f, gain.load());
    const auto currentPan = juce::jlimit(-1.0f, 1.0f, pan.load());

    auto leftGain = currentGain;
    auto rightGain = currentGain;

    if (currentPan < 0.0f)
        rightGain *= 1.0f + currentPan;
    else if (currentPan > 0.0f)
        leftGain *= 1.0f - currentPan;

    if (bufferToFill.buffer->getNumChannels() > 0)
    {
        bufferToFill.buffer->applyGain(
            0,
            bufferToFill.startSample,
            bufferToFill.numSamples,
            leftGain
        );
    }

    if (bufferToFill.buffer->getNumChannels() > 1)
    {
        bufferToFill.buffer->applyGain(
            1,
            bufferToFill.startSample,
            bufferToFill.numSamples,
            rightGain
        );
    }

    for (int channel = 2;
         channel < bufferToFill.buffer->getNumChannels();
         ++channel)
    {
        bufferToFill.buffer->applyGain(
            channel,
            bufferToFill.startSample,
            bufferToFill.numSamples,
            currentGain
        );
    }

    float peak = 0.0f;

    for (int channel = 0;
         channel < bufferToFill.buffer->getNumChannels();
         ++channel)
    {
        peak = juce::jmax(
            peak,
            bufferToFill.buffer->getMagnitude(
                channel,
                bufferToFill.startSample,
                bufferToFill.numSamples
            )
        );
    }

    peakLevel.store(
        juce::jlimit(0.0f, 1.5f, peak)
    );
}

void AudioTrack::syncToProjectPosition(
    const double projectPositionSeconds,
    const bool projectPlaying
)
{
    const auto clipLocalPosition =
        projectPositionSeconds - startOffsetSeconds.load();

    const auto clipDuration = getClipDurationSeconds();
    const auto sourceStart = sourceStartSeconds.load();
    const auto sourceEnd = sourceEndSeconds.load();

    if (clipLocalPosition < 0.0
        || clipLocalPosition >= clipDuration)
    {
        if (transport.isPlaying())
            transport.stop();

        transport.setPosition(
            clipLocalPosition < 0.0
                ? sourceStart
                : sourceEnd
        );

        return;
    }

    const auto sourcePosition =
        sourceStart + clipLocalPosition;

    const auto currentPosition =
        transport.getCurrentPosition();

    if (std::abs(currentPosition - sourcePosition) > 0.05)
        transport.setPosition(sourcePosition);

    if (projectPlaying)
    {
        if (! transport.isPlaying())
            transport.start();
    }
    else if (transport.isPlaying())
    {
        transport.stop();
    }
}

void AudioTrack::setGain(const float newGain)
{
    gain.store(juce::jlimit(0.0f, 1.5f, newGain));
}

void AudioTrack::setPan(const float newPan)
{
    pan.store(juce::jlimit(-1.0f, 1.0f, newPan));
}

void AudioTrack::setMuted(const bool shouldBeMuted)
{
    muted.store(shouldBeMuted);
}

void AudioTrack::setSolo(const bool shouldBeSolo)
{
    solo.store(shouldBeSolo);
}

void AudioTrack::setSoloMuted(const bool shouldBeSoloMuted)
{
    soloMuted.store(shouldBeSoloMuted);
}

void AudioTrack::setStartOffsetSeconds(const double seconds)
{
    startOffsetSeconds.store(
        juce::jlimit(0.0, 36000.0, seconds)
    );
}

void AudioTrack::setSourceRange(
    const double startSeconds,
    const double endSeconds
)
{
    const auto sourceLength = getLengthSeconds();

    const auto safeStart = juce::jlimit(
        0.0,
        sourceLength,
        startSeconds
    );

    const auto safeEnd = juce::jlimit(
        safeStart,
        sourceLength,
        endSeconds
    );

    sourceStartSeconds.store(safeStart);
    sourceEndSeconds.store(safeEnd);
}

float AudioTrack::getGain() const noexcept
{
    return gain.load();
}

float AudioTrack::getPan() const noexcept
{
    return pan.load();
}

float AudioTrack::getPeakLevel() const noexcept
{
    return peakLevel.load();
}

bool AudioTrack::isMuted() const noexcept
{
    return muted.load();
}

bool AudioTrack::isSolo() const noexcept
{
    return solo.load();
}

bool AudioTrack::isPlaying() const
{
    return transport.isPlaying();
}

double AudioTrack::getPositionSeconds() const
{
    return transport.getCurrentPosition();
}

double AudioTrack::getLengthSeconds() const
{
    return transport.getLengthInSeconds();
}

double AudioTrack::getStartOffsetSeconds() const noexcept
{
    return startOffsetSeconds.load();
}

double AudioTrack::getSourceStartSeconds() const noexcept
{
    return sourceStartSeconds.load();
}

double AudioTrack::getSourceEndSeconds() const noexcept
{
    return sourceEndSeconds.load();
}

double AudioTrack::getClipDurationSeconds() const noexcept
{
    return juce::jmax(
        0.0,
        sourceEndSeconds.load()
            - sourceStartSeconds.load()
    );
}

double AudioTrack::getProjectEndSeconds() const noexcept
{
    return getStartOffsetSeconds()
        + getClipDurationSeconds();
}

const juce::String& AudioTrack::getName() const noexcept
{
    return name;
}

const juce::File& AudioTrack::getSourceFile() const noexcept
{
    return file;
}
