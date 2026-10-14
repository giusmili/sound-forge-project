#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <array>
#include <atomic>
#include <vector>

namespace fx
{
// Order of the processing chain: filter -> drive -> chorus -> delay -> echo -> reverb.
enum Param : int
{
    filterOn,
    filterHighPass,
    filterCutoff,
    filterResonance,
    driveOn,
    driveAmount,
    driveMix,
    chorusOn,
    chorusRate,
    chorusDepth,
    chorusMix,
    delayOn,
    delayTime,
    delayFeedback,
    delayMix,
    echoOn,
    echoTime,
    echoFeedback,
    echoTone,
    echoMix,
    reverbOn,
    reverbSize,
    reverbDamping,
    reverbMix,
    paramCount
};

enum class Group : int
{
    filter,
    drive,
    chorus,
    delay,
    echo,
    reverb
};

constexpr int groupCount = 6;

enum class Kind
{
    enable,
    toggle,
    slider
};

struct ParamInfo
{
    const char* id;
    const char* label;
    float minimum;
    float maximum;
    float defaultValue;
    float step;
    float skewMidpoint; // 0 = linear slider
    Group group;
    Kind kind;
    const char* suffix;
};

const ParamInfo& getParamInfo(int index) noexcept;
const char* getGroupName(Group group) noexcept;
Param getGroupEnableParam(Group group) noexcept;
} // namespace fx

// Plain, copyable values of every effect parameter. Used for project
// snapshots, undo/redo, serialisation and duplicating tracks.
struct EffectsSettings
{
    EffectsSettings() noexcept;

    std::array<float, fx::paramCount> values;

    [[nodiscard]] bool operator==(const EffectsSettings& other) const noexcept;
    [[nodiscard]] bool operator!=(const EffectsSettings& other) const noexcept;

    [[nodiscard]] bool isGroupEnabled(fx::Group group) const noexcept;
    [[nodiscard]] bool hasActiveEffect() const noexcept;

    [[nodiscard]] juce::var toVar() const;
    // Missing or invalid properties fall back to defaults, so projects saved
    // before effects existed load unchanged.
    [[nodiscard]] static EffectsSettings fromVar(const juce::var& data);

    [[nodiscard]] juce::String fingerprint() const;
};

// Per-track insert chain. Parameters are lock-free and may be changed from the
// message thread while process() runs on the audio thread.
class TrackEffects final
{
public:
    TrackEffects();

    // Allocates the delay lines. Must not run concurrently with process().
    void prepare(double sampleRate);
    void reset();

    void setParam(int index, float value) noexcept;
    [[nodiscard]] float getParam(int index) const noexcept;

    void apply(const EffectsSettings& settings) noexcept;
    [[nodiscard]] EffectsSettings getSettings() const noexcept;
    [[nodiscard]] bool hasActiveEffect() const noexcept;

    void process(
        juce::AudioBuffer<float>& buffer,
        int startSample,
        int numSamples
    ) noexcept;

private:
    struct DelayBuffer
    {
        std::vector<float> data;
        int writeIndex = 0;

        void resize(int length);
        void clear() noexcept;
        [[nodiscard]] float read(float delaySamples) const noexcept;
        void write(float value) noexcept;
    };

    struct StateVariableFilter
    {
        float ic1 = 0.0f;
        float ic2 = 0.0f;

        float process(
            float input,
            float a1,
            float a2,
            float a3,
            float k,
            bool highPass
        ) noexcept;
    };

    void clearEffectState(int group) noexcept;

    std::array<std::atomic<float>, fx::paramCount> params;

    double sampleRate = 0.0;
    float timeSmoothCoefficient = 0.001f;

    std::array<bool, fx::groupCount> wasActive {};

    StateVariableFilter filterLeft;
    StateVariableFilter filterRight;
    float smoothedCutoff = 1000.0f;

    DelayBuffer chorusLeft;
    DelayBuffer chorusRight;
    float chorusPhase = 0.0f;

    DelayBuffer delayLeft;
    DelayBuffer delayRight;
    float smoothedDelaySamples = 0.0f;

    DelayBuffer echoLeft;
    DelayBuffer echoRight;
    float smoothedEchoSamples = 0.0f;
    float echoToneLeft = 0.0f;
    float echoToneRight = 0.0f;

    juce::Reverb reverb;

    // Block-start values for the per-sample linear ramps (avoids zipper noise).
    float previousDriveMix = 0.0f;
    float previousChorusMix = 0.0f;
    float previousDelayMix = 0.0f;
    float previousDelayFeedback = 0.0f;
    float previousEchoMix = 0.0f;
    float previousEchoFeedback = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackEffects)
};
