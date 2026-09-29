#include "MidiInstrumentTrack.h"

#include <cmath>

MidiInstrumentTrack::MidiInstrumentTrack(
    juce::String trackName
)
    : name(std::move(trackName))
{
    for (int index = 0; index < 8; ++index)
        synthesiser.addVoice(new BasicSynthVoice());

    synthesiser.addSound(new BasicSynthSound());
}

void MidiInstrumentTrack::prepareToPlay(
    const int,
    const double sampleRate
)
{
    synthesiser.setCurrentPlaybackSampleRate(
        sampleRate
    );
}

void MidiInstrumentTrack::releaseResources()
{
    allNotesOff();
}

void MidiInstrumentTrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill
)
{
    if (bufferToFill.buffer == nullptr)
        return;

    if (muted.load())
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    synthesiser.renderNextBlock(
        *bufferToFill.buffer,
        emptyMidiBuffer,
        bufferToFill.startSample,
        bufferToFill.numSamples
    );

    bufferToFill.buffer->applyGain(
        bufferToFill.startSample,
        bufferToFill.numSamples,
        juce::jlimit(0.0f, 1.5f, gain.load())
    );
}

void MidiInstrumentTrack::noteOn(
    const int midiNote,
    const float velocity
)
{
    synthesiser.noteOn(
        1,
        juce::jlimit(0, 127, midiNote),
        juce::jlimit(0.0f, 1.0f, velocity)
    );
}

void MidiInstrumentTrack::noteOff(
    const int midiNote
)
{
    synthesiser.noteOff(
        1,
        juce::jlimit(0, 127, midiNote),
        0.0f,
        true
    );
}

void MidiInstrumentTrack::allNotesOff()
{
    synthesiser.allNotesOff(
        1,
        true
    );
}

void MidiInstrumentTrack::setGain(
    const float newGain
)
{
    gain.store(
        juce::jlimit(0.0f, 1.5f, newGain)
    );
}

void MidiInstrumentTrack::setMuted(
    const bool shouldBeMuted
)
{
    muted.store(shouldBeMuted);

    if (shouldBeMuted)
        allNotesOff();
}

float MidiInstrumentTrack::getGain() const noexcept
{
    return gain.load();
}

bool MidiInstrumentTrack::isMuted() const noexcept
{
    return muted.load();
}

const juce::String&
MidiInstrumentTrack::getName() const noexcept
{
    return name;
}

int MidiInstrumentTrack::getInstrumentId() const noexcept
{
    return 1;
}

bool MidiInstrumentTrack::BasicSynthVoice::canPlaySound(
    juce::SynthesiserSound* sound
)
{
    return dynamic_cast<BasicSynthSound*>(sound)
        != nullptr;
}

void MidiInstrumentTrack::BasicSynthVoice::startNote(
    const int midiNoteNumber,
    const float velocity,
    juce::SynthesiserSound*,
    const int
)
{
    currentAngle = 0.0;
    level = velocity * 0.18f;
    tailOff = 0.0f;

    const auto cyclesPerSecond =
        juce::MidiMessage::getMidiNoteInHertz(
            midiNoteNumber
        );

    const auto cyclesPerSample =
        cyclesPerSecond / getSampleRate();

    angleDelta =
        cyclesPerSample
        * juce::MathConstants<double>::twoPi;
}

void MidiInstrumentTrack::BasicSynthVoice::stopNote(
    const float,
    const bool allowTailOff
)
{
    if (allowTailOff)
    {
        if (tailOff == 0.0f)
            tailOff = 1.0f;
    }
    else
    {
        clearCurrentNote();
        angleDelta = 0.0;
    }
}

void MidiInstrumentTrack::BasicSynthVoice::renderNextBlock(
    juce::AudioBuffer<float>& outputBuffer,
    int startSample,
    int numSamples
)
{
    if (angleDelta == 0.0)
        return;

    while (--numSamples >= 0)
    {
        auto currentSample =
            static_cast<float>(
                std::sin(currentAngle)
            )
            * level;

        if (tailOff > 0.0f)
            currentSample *= tailOff;

        for (int channel = 0;
             channel < outputBuffer.getNumChannels();
             ++channel)
        {
            outputBuffer.addSample(
                channel,
                startSample,
                currentSample
            );
        }

        currentAngle += angleDelta;
        ++startSample;

        if (tailOff > 0.0f)
        {
            tailOff *= 0.99f;

            if (tailOff <= 0.005f)
            {
                clearCurrentNote();
                angleDelta = 0.0;
                break;
            }
        }
    }
}
