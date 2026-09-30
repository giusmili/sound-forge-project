#include "AudioTrack.h"

#include <cmath>

AudioTrack::AudioTrack(
    juce::File sourceFile,
    juce::String trackName,
    std::unique_ptr<juce::AudioFormatReaderSource> source,
    const double sourceSampleRate,
    juce::TimeSliceThread& readAheadThread
)
    : file(std::move(sourceFile)),
      name(std::move(trackName)),
      readerSource(std::move(source)),
      sourceRate(sourceSampleRate),
      lengthSeconds(static_cast<double>(readerSource->getTotalLength()) / sourceRate),
      bufferedSource(readerSource.get(), readAheadThread, false, 131072, 2, false),
      resampler(&bufferedSource, false, 2)
{
}

AudioTrack::~AudioTrack()
{
    releaseResources();
}

void AudioTrack::prepareToPlay(const int blockSize, const double sampleRate)
{
    releaseResources();
    outputRate = sampleRate;
    resampler.setResamplingRatio(sourceRate / outputRate);
    prepared = true;
    try
    {
        resampler.prepareToPlay(blockSize, outputRate);
    }
    catch (...)
    {
        releaseResources();
        throw;
    }
}

void AudioTrack::releaseResources()
{
    if (prepared)
        resampler.releaseResources();
    prepared = false;
    outputRate = 0.0;
    clearPeaks();
}

bool AudioTrack::isReady(const int outputSamples)
{
    if (! prepared)
        return false;

    // JUCE's resampler needs up to round(n * ratio) + 3 source samples.
    // A zero timeout only probes availability; the audio callback never waits
    // for disk I/O. The engine advances all tracks together, or none of them.
    const auto required = static_cast<int>(std::ceil(outputSamples * sourceRate / outputRate)) + 4;
    return bufferedSource.waitForNextAudioBlockReady({ nullptr, 0, required }, 0);
}

void AudioTrack::clearPeaks() noexcept
{
    peakLeft.store(0.0f);
    peakRight.store(0.0f);
}

void AudioTrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill
)
{
    bufferToFill.clearActiveBufferRegion();
    if (! prepared || bufferToFill.numSamples <= 0)
        return;
    resampler.getNextAudioBlock(bufferToFill);

    for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
    {
        auto* samples = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);
        for (int i = 0; i < bufferToFill.numSamples; ++i)
            if (! std::isfinite(samples[i])) samples[i] = 0.0f;
    }

    if (muted.load() || soloMuted.load())
    {
        bufferToFill.clearActiveBufferRegion();
        peakLeft.store(0.0f);
        peakRight.store(0.0f);
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

    const auto leftPeak = bufferToFill.buffer->getNumChannels() > 0
        ? bufferToFill.buffer->getMagnitude(0, bufferToFill.startSample, bufferToFill.numSamples)
        : 0.0f;
    const auto rightPeak = bufferToFill.buffer->getNumChannels() > 1
        ? bufferToFill.buffer->getMagnitude(1, bufferToFill.startSample, bufferToFill.numSamples)
        : leftPeak;
    peakLeft.store(juce::jlimit(0.0f, 1.0f, leftPeak));
    peakRight.store(juce::jlimit(0.0f, 1.0f, rightPeak));

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
}

void AudioTrack::setPositionSeconds(const double seconds)
{
    const auto position = std::isfinite(seconds)
        ? juce::jlimit(0.0, lengthSeconds, seconds) : 0.0;
    bufferedSource.setNextReadPosition(static_cast<juce::int64>(position * sourceRate));
    resampler.flushBuffers();
    clearPeaks();
}

void AudioTrack::setGain(const float newGain)
{
    gain.store(std::isfinite(newGain) ? juce::jlimit(0.0f, 1.5f, newGain) : 1.0f);
}

void AudioTrack::setPan(const float newPan)
{
    pan.store(std::isfinite(newPan) ? juce::jlimit(-1.0f, 1.0f, newPan) : 0.0f);
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

float AudioTrack::getGain() const noexcept
{
    return gain.load();
}

float AudioTrack::getPan() const noexcept
{
    return pan.load();
}

bool AudioTrack::isMuted() const noexcept
{
    return muted.load();
}

bool AudioTrack::isSolo() const noexcept
{
    return solo.load();
}

float AudioTrack::getPeakLeft() const noexcept
{
    return peakLeft.load();
}

float AudioTrack::getPeakRight() const noexcept
{
    return peakRight.load();
}

double AudioTrack::getLengthSeconds() const
{
    return lengthSeconds;
}

const juce::String& AudioTrack::getName() const noexcept
{
    return name;
}

const juce::File& AudioTrack::getSourceFile() const noexcept
{
    return file;
}
