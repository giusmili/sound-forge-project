#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "AudioTrack.h"
#include "MidiInstrumentTrack.h"

class AudioEngine final : public juce::AudioSource,
                          public juce::AudioIODeviceCallback
{
public:
    struct TrackState
    {
        juce::File sourceFile;
        double startOffsetSeconds = 0.0;
        double sourceStartSeconds = 0.0;
        double sourceEndSeconds = 0.0;
        float gain = 1.0f;
        float pan = 0.0f;
        bool muted = false;
        bool solo = false;
    };

    struct MidiTrackState
    {
        juce::String name;
        float gain = 0.8f;
        bool muted = false;
        int instrumentId = 1;
    };

    AudioEngine();
    ~AudioEngine() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context
    ) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    juce::Result addTrackFromFile(
        const juce::File& file,
        AudioTrack*& createdTrack
    );

    juce::Result duplicateTrack(
        const AudioTrack& sourceTrack,
        AudioTrack*& createdTrack
    );

    juce::Result splitTrackAtProjectPosition(
        AudioTrack& sourceTrack,
        double projectPositionSeconds,
        AudioTrack*& rightTrack
    );

    bool removeTrack(AudioTrack* trackToRemove);

    MidiInstrumentTrack* addMidiInstrumentTrack(
        const juce::String& name
    );
    bool removeMidiInstrumentTrack(
        MidiInstrumentTrack* trackToRemove
    );

    [[nodiscard]] std::vector<TrackState> captureTrackStates() const;
    [[nodiscard]] std::vector<MidiTrackState> captureMidiTrackStates() const;
    juce::Result restoreProjectTracks(
        const std::vector<TrackState>& audioStates,
        const std::vector<MidiTrackState>& midiStates
    );
    juce::Result restoreTrackStates(
        const std::vector<TrackState>& states
    );
    [[nodiscard]] std::vector<AudioTrack*> getTrackPointers() const;
    [[nodiscard]] std::vector<MidiInstrumentTrack*> getMidiTrackPointers() const;

    juce::Result startRecording(const juce::File& file);
    void stopRecording();
    [[nodiscard]] bool isRecording() const noexcept;
    [[nodiscard]] double getRecordingStartPosition() const noexcept;
    [[nodiscard]] bool hasAudioInput() const noexcept;

    void play();
    void pause();
    void stop();
    void setPositionSeconds(double seconds);
    void setMasterGain(float gain);
    void refreshSoloState();
    void refreshTrackAlignment();

    [[nodiscard]] bool isPlaying() const noexcept;
    [[nodiscard]] double getPositionSeconds() const noexcept;
    [[nodiscard]] double getLengthSeconds() const;
    [[nodiscard]] int getTrackCount() const noexcept;

    juce::AudioDeviceManager& getDeviceManager() noexcept;
    juce::AudioFormatManager& getFormatManager() noexcept;
    juce::AudioThumbnailCache& getThumbnailCache() noexcept;

private:
    [[nodiscard]] double getLengthSecondsUnlocked() const;
    void syncTracksUnlocked(double projectPosition, bool projectPlaying);
    void refreshSoloStateUnlocked();

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 64 };
    juce::MixerAudioSource mixer;
    std::vector<std::unique_ptr<AudioTrack>> tracks;
    std::vector<std::unique_ptr<MidiInstrumentTrack>> midiTracks;

    mutable juce::CriticalSection trackLock;

    std::atomic<float> masterGain { 0.8f };
    std::atomic<double> projectPositionSeconds { 0.0 };
    std::atomic<double> outputSampleRate { 44100.0 };
    std::atomic<bool> playing { false };

    juce::AudioDeviceManager deviceManager;

    juce::TimeSliceThread recordingThread { "SonoForge Recorder" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    std::atomic<bool> recording { false };
    std::atomic<double> recordingStartPosition { 0.0 };
    std::atomic<double> inputSampleRate { 0.0 };
    std::atomic<int> activeInputChannels { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
