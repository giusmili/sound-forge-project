#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <functional>
#include <vector>

#include "AudioEngine.h"

struct MixdownOptions
{
    enum class Format
    {
        wav16,
        wav24,
        wav32Float,
        flac16,
        flac24
    };

    juce::File outputFile;
    Format format = Format::wav24;
    double sampleRate = 48000.0;
    // Scale the finished mix so its peak sits at -1 dBFS.
    bool normalise = false;
    float masterGain = 1.0f;
    // Extra time rendered after the last clip so reverb and delay tails ring
    // out. Negative = automatic (a few seconds, only when an effect is on).
    double tailSeconds = -1.0;
};

struct MixdownResult
{
    juce::Result status = juce::Result::ok();
    bool cancelled = false;
    double durationSeconds = 0.0;
    // Peak of the exported signal (linear, after normalisation).
    float peak = 0.0f;
    // Samples that exceeded 0 dBFS and were clipped by the file format.
    juce::int64 clippedSamples = 0;
};

// Offline bounce of the audio tracks, independent of the audio device: it
// reads the source files itself, so it is faster than real time and does not
// disturb playback. Applies the same effects, fader, pan, mute/solo and
// master gain as the live engine.
class MixdownRenderer
{
public:
    // Called regularly with the completed fraction (0..1); return false to cancel.
    using ProgressCallback = std::function<bool(double)>;

    [[nodiscard]] static MixdownResult render(
        juce::AudioFormatManager& formatManager,
        const std::vector<AudioEngine::TrackState>& tracks,
        const MixdownOptions& options,
        const ProgressCallback& progress = {}
    );

    // Length of the file render() would produce, 0 when nothing is audible.
    [[nodiscard]] static double calculateLengthSeconds(
        const std::vector<AudioEngine::TrackState>& tracks,
        const MixdownOptions& options
    );

    [[nodiscard]] static juce::String getFileExtension(
        MixdownOptions::Format format
    );
    [[nodiscard]] static juce::String getFormatDescription(
        MixdownOptions::Format format
    );

private:
    static MixdownResult runPass(
        juce::AudioFormatManager& formatManager,
        const std::vector<AudioEngine::TrackState>& tracks,
        const MixdownOptions& options,
        float extraGain,
        juce::AudioFormatWriter* writer,
        const ProgressCallback& progress,
        double progressStart,
        double progressSpan
    );
};
