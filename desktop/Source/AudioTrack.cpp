#include "AudioTrack.h"

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

    if (muted.load())
    {
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
}

void AudioTrack::play()
{
    if (getLengthSeconds() > 0.0
        && getPositionSeconds() < getLengthSeconds())
    {
        transport.start();
    }
}

void AudioTrack::stop()
{
    transport.stop();
    transport.setPosition(0.0);
}

void AudioTrack::setPositionSeconds(const double seconds)
{
    transport.setPosition(
        juce::jlimit(0.0, getLengthSeconds(), seconds)
    );
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

const juce::String& AudioTrack::getName() const noexcept
{
    return name;
}

const juce::File& AudioTrack::getSourceFile() const noexcept
{
    return file;
}
