#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class MidiInstrumentTrack final : public juce::AudioSource
{
public:
    explicit MidiInstrumentTrack(juce::String trackName);
    ~MidiInstrumentTrack() override = default;

    void prepareToPlay(
        int samplesPerBlockExpected,
        double sampleRate
    ) override;

    void releaseResources() override;

    void getNextAudioBlock(
        const juce::AudioSourceChannelInfo& bufferToFill
    ) override;

    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);
    void allNotesOff();

    void setGain(float newGain);
    void setMuted(bool shouldBeMuted);

    [[nodiscard]] float getGain() const noexcept;
    [[nodiscard]] bool isMuted() const noexcept;
    [[nodiscard]] const juce::String& getName() const noexcept;
    [[nodiscard]] int getInstrumentId() const noexcept;

private:
    class BasicSynthSound final
        : public juce::SynthesiserSound
    {
    public:
        bool appliesToNote(int) override
        {
            return true;
        }

        bool appliesToChannel(int) override
        {
            return true;
        }
    };

    class BasicSynthVoice final
        : public juce::SynthesiserVoice
    {
    public:
        bool canPlaySound(
            juce::SynthesiserSound* sound
        ) override;

        void startNote(
            int midiNoteNumber,
            float velocity,
            juce::SynthesiserSound*,
            int
        ) override;

        void stopNote(
            float velocity,
            bool allowTailOff
        ) override;

        void pitchWheelMoved(int) override {}
        void controllerMoved(int, int) override {}

        void renderNextBlock(
            juce::AudioBuffer<float>& outputBuffer,
            int startSample,
            int numSamples
        ) override;

    private:
        double currentAngle = 0.0;
        double angleDelta = 0.0;
        float level = 0.0f;
        float tailOff = 0.0f;
    };

    juce::String name;
    juce::Synthesiser synthesiser;
    juce::MidiBuffer emptyMidiBuffer;

    std::atomic<float> gain { 0.8f };
    std::atomic<bool> muted { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        MidiInstrumentTrack
    )
};
