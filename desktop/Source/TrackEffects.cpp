#include "TrackEffects.h"

#include <algorithm>
#include <cmath>

namespace fx
{
namespace
{
using G = Group;
using K = Kind;

// id, label, min, max, default, step, skew midpoint, group, kind, suffix
constexpr ParamInfo paramTable[paramCount] =
{
    { "filterOn",        "Filtre",     0.0f,     1.0f,     0.0f,   1.0f,   0.0f,    G::filter, K::enable, "" },
    { "filterHighPass",  "Passe-haut", 0.0f,     1.0f,     0.0f,   1.0f,   0.0f,    G::filter, K::toggle, "" },
    { "filterCutoff",    "Frequence",  20.0f,    20000.0f, 1000.0f, 1.0f,  1000.0f, G::filter, K::slider, " Hz" },
    { "filterResonance", "Resonance",  0.0f,     1.0f,     0.2f,   0.01f,  0.0f,    G::filter, K::slider, "" },

    { "driveOn",         "Distorsion", 0.0f,     1.0f,     0.0f,   1.0f,   0.0f,    G::drive,  K::enable, "" },
    { "driveAmount",     "Gain",       0.0f,     1.0f,     0.4f,   0.01f,  0.0f,    G::drive,  K::slider, "" },
    { "driveMix",        "Mix",        0.0f,     1.0f,     1.0f,   0.01f,  0.0f,    G::drive,  K::slider, "" },

    { "chorusOn",        "Chorus",     0.0f,     1.0f,     0.0f,   1.0f,   0.0f,    G::chorus, K::enable, "" },
    { "chorusRate",      "Vitesse",    0.1f,     5.0f,     0.8f,   0.01f,  0.0f,    G::chorus, K::slider, " Hz" },
    { "chorusDepth",     "Profondeur", 0.0f,     1.0f,     0.5f,   0.01f,  0.0f,    G::chorus, K::slider, "" },
    { "chorusMix",       "Mix",        0.0f,     1.0f,     0.5f,   0.01f,  0.0f,    G::chorus, K::slider, "" },

    { "delayOn",         "Delay",      0.0f,     1.0f,     0.0f,   1.0f,   0.0f,    G::delay,  K::enable, "" },
    { "delayTime",       "Temps",      1.0f,     2000.0f,  250.0f, 1.0f,   300.0f,  G::delay,  K::slider, " ms" },
    { "delayFeedback",   "Feedback",   0.0f,     0.95f,    0.35f,  0.01f,  0.0f,    G::delay,  K::slider, "" },
    { "delayMix",        "Mix",        0.0f,     1.0f,     0.35f,  0.01f,  0.0f,    G::delay,  K::slider, "" },

    { "echoOn",          "Echo",       0.0f,     1.0f,     0.0f,   1.0f,   0.0f,    G::echo,   K::enable, "" },
    { "echoTime",        "Temps",      50.0f,    2000.0f,  400.0f, 1.0f,   400.0f,  G::echo,   K::slider, " ms" },
    { "echoFeedback",    "Feedback",   0.0f,     0.95f,    0.45f,  0.01f,  0.0f,    G::echo,   K::slider, "" },
    { "echoTone",        "Tonalite",   0.0f,     1.0f,     0.5f,   0.01f,  0.0f,    G::echo,   K::slider, "" },
    { "echoMix",         "Mix",        0.0f,     1.0f,     0.35f,  0.01f,  0.0f,    G::echo,   K::slider, "" },

    { "reverbOn",        "Reverb",     0.0f,     1.0f,     0.0f,   1.0f,   0.0f,    G::reverb, K::enable, "" },
    { "reverbSize",      "Taille",     0.0f,     1.0f,     0.5f,   0.01f,  0.0f,    G::reverb, K::slider, "" },
    { "reverbDamping",   "Amorti",     0.0f,     1.0f,     0.5f,   0.01f,  0.0f,    G::reverb, K::slider, "" },
    { "reverbMix",       "Mix",        0.0f,     1.0f,     0.3f,   0.01f,  0.0f,    G::reverb, K::slider, "" },
};
} // namespace

const ParamInfo& getParamInfo(const int index) noexcept
{
    return paramTable[juce::jlimit(0, paramCount - 1, index)];
}

const char* getGroupName(const Group group) noexcept
{
    return getParamInfo(getGroupEnableParam(group)).label;
}

Param getGroupEnableParam(const Group group) noexcept
{
    switch (group)
    {
        case Group::filter: return filterOn;
        case Group::drive:  return driveOn;
        case Group::chorus: return chorusOn;
        case Group::delay:  return delayOn;
        case Group::echo:   return echoOn;
        case Group::reverb: return reverbOn;
    }

    return filterOn;
}
} // namespace fx

// ---------------------------------------------------------------------------
// EffectsSettings

EffectsSettings::EffectsSettings() noexcept
{
    for (int index = 0; index < fx::paramCount; ++index)
        values[static_cast<size_t>(index)] = fx::getParamInfo(index).defaultValue;
}

bool EffectsSettings::operator==(const EffectsSettings& other) const noexcept
{
    return values == other.values;
}

bool EffectsSettings::operator!=(const EffectsSettings& other) const noexcept
{
    return ! (*this == other);
}

bool EffectsSettings::isGroupEnabled(const fx::Group group) const noexcept
{
    return values[static_cast<size_t>(fx::getGroupEnableParam(group))] >= 0.5f;
}

bool EffectsSettings::hasActiveEffect() const noexcept
{
    for (int group = 0; group < fx::groupCount; ++group)
    {
        if (isGroupEnabled(static_cast<fx::Group>(group)))
            return true;
    }

    return false;
}

juce::var EffectsSettings::toVar() const
{
    auto* object = new juce::DynamicObject();

    for (int index = 0; index < fx::paramCount; ++index)
    {
        object->setProperty(
            juce::Identifier(fx::getParamInfo(index).id),
            static_cast<double>(values[static_cast<size_t>(index)])
        );
    }

    return juce::var(object);
}

EffectsSettings EffectsSettings::fromVar(const juce::var& data)
{
    EffectsSettings settings;

    const auto* object = data.getDynamicObject();

    if (object == nullptr)
        return settings;

    for (int index = 0; index < fx::paramCount; ++index)
    {
        const auto& info = fx::getParamInfo(index);
        const juce::Identifier id(info.id);

        if (! object->hasProperty(id))
            continue;

        const auto value = static_cast<double>(object->getProperty(id));

        if (! std::isfinite(value))
            continue;

        settings.values[static_cast<size_t>(index)] = juce::jlimit(
            info.minimum,
            info.maximum,
            static_cast<float>(value)
        );
    }

    return settings;
}

juce::String EffectsSettings::fingerprint() const
{
    juce::String text;

    for (const auto value : values)
        text << juce::String(value, 4) << ',';

    return text;
}

// ---------------------------------------------------------------------------
// TrackEffects

void TrackEffects::DelayBuffer::resize(const int length)
{
    data.assign(static_cast<size_t>(juce::jmax(8, length)), 0.0f);
    writeIndex = 0;
}

void TrackEffects::DelayBuffer::clear() noexcept
{
    std::fill(data.begin(), data.end(), 0.0f);
    writeIndex = 0;
}

float TrackEffects::DelayBuffer::read(const float delaySamples) const noexcept
{
    const auto size = static_cast<int>(data.size());
    auto position = static_cast<float>(writeIndex) - delaySamples;

    while (position < 0.0f)
        position += static_cast<float>(size);

    auto index = static_cast<int>(position);
    const auto fraction = position - static_cast<float>(index);

    if (index >= size)
        index -= size;

    const auto next = index + 1 >= size ? 0 : index + 1;

    return data[static_cast<size_t>(index)]
        + fraction
            * (data[static_cast<size_t>(next)] - data[static_cast<size_t>(index)]);
}

void TrackEffects::DelayBuffer::write(const float value) noexcept
{
    data[static_cast<size_t>(writeIndex)] = value;

    if (++writeIndex >= static_cast<int>(data.size()))
        writeIndex = 0;
}

float TrackEffects::StateVariableFilter::process(
    const float input,
    const float a1,
    const float a2,
    const float a3,
    const float k,
    const bool highPass
) noexcept
{
    const auto v3 = input - ic2;
    const auto v1 = a1 * ic1 + a2 * v3;
    const auto v2 = ic2 + a2 * ic1 + a3 * v3;

    ic1 = 2.0f * v1 - ic1;
    ic2 = 2.0f * v2 - ic2;

    return highPass ? input - k * v1 - v2 : v2;
}

TrackEffects::TrackEffects()
{
    for (int index = 0; index < fx::paramCount; ++index)
    {
        params[static_cast<size_t>(index)].store(
            fx::getParamInfo(index).defaultValue
        );
    }
}

void TrackEffects::prepare(const double newSampleRate)
{
    sampleRate = newSampleRate;

    if (sampleRate <= 0.0)
        return;

    const auto maxDelay = static_cast<int>(std::ceil(2.0 * sampleRate)) + 8;
    const auto maxChorus = static_cast<int>(std::ceil(0.03 * sampleRate)) + 8;

    delayLeft.resize(maxDelay);
    delayRight.resize(maxDelay);
    echoLeft.resize(maxDelay);
    echoRight.resize(maxDelay);
    chorusLeft.resize(maxChorus);
    chorusRight.resize(maxChorus);

    reverb.setSampleRate(sampleRate);
    reverb.reset();

    timeSmoothCoefficient = static_cast<float>(
        1.0 - std::exp(-1.0 / (0.04 * sampleRate))
    );

    // Smoothed values are expressed in samples: re-initialise on next block.
    wasActive.fill(false);
}

void TrackEffects::reset()
{
    for (int group = 0; group < fx::groupCount; ++group)
        clearEffectState(group);

    wasActive.fill(false);
}

void TrackEffects::clearEffectState(const int group) noexcept
{
    switch (static_cast<fx::Group>(group))
    {
        case fx::Group::filter:
            filterLeft = {};
            filterRight = {};
            break;
        case fx::Group::drive:
            break;
        case fx::Group::chorus:
            chorusLeft.clear();
            chorusRight.clear();
            chorusPhase = 0.0f;
            break;
        case fx::Group::delay:
            delayLeft.clear();
            delayRight.clear();
            break;
        case fx::Group::echo:
            echoLeft.clear();
            echoRight.clear();
            echoToneLeft = 0.0f;
            echoToneRight = 0.0f;
            break;
        case fx::Group::reverb:
            reverb.reset();
            break;
    }
}

void TrackEffects::setParam(const int index, const float value) noexcept
{
    if (index < 0 || index >= fx::paramCount)
        return;

    const auto& info = fx::getParamInfo(index);

    if (! std::isfinite(value))
        return;

    params[static_cast<size_t>(index)].store(
        juce::jlimit(info.minimum, info.maximum, value),
        std::memory_order_relaxed
    );
}

float TrackEffects::getParam(const int index) const noexcept
{
    if (index < 0 || index >= fx::paramCount)
        return 0.0f;

    return params[static_cast<size_t>(index)].load(std::memory_order_relaxed);
}

void TrackEffects::apply(const EffectsSettings& settings) noexcept
{
    for (int index = 0; index < fx::paramCount; ++index)
        setParam(index, settings.values[static_cast<size_t>(index)]);
}

EffectsSettings TrackEffects::getSettings() const noexcept
{
    EffectsSettings settings;

    for (int index = 0; index < fx::paramCount; ++index)
        settings.values[static_cast<size_t>(index)] = getParam(index);

    return settings;
}

bool TrackEffects::hasActiveEffect() const noexcept
{
    for (int group = 0; group < fx::groupCount; ++group)
    {
        if (getParam(fx::getGroupEnableParam(static_cast<fx::Group>(group))) >= 0.5f)
            return true;
    }

    return false;
}

void TrackEffects::process(
    juce::AudioBuffer<float>& buffer,
    const int startSample,
    const int numSamples
) noexcept
{
    if (sampleRate <= 0.0
        || numSamples <= 0
        || buffer.getNumChannels() <= 0
        || delayLeft.data.empty())
    {
        return;
    }

    float p[fx::paramCount];

    for (int index = 0; index < fx::paramCount; ++index)
        p[index] = getParam(index);

    const auto sr = static_cast<float>(sampleRate);

    const auto delayLimit = static_cast<float>(delayLeft.data.size()) - 4.0f;
    const auto delayTarget = juce::jlimit(1.0f, delayLimit, p[fx::delayTime] * 0.001f * sr);
    const auto echoTarget = juce::jlimit(1.0f, delayLimit, p[fx::echoTime] * 0.001f * sr);

    bool anyActive = false;
    std::array<bool, fx::groupCount> active {};

    for (int group = 0; group < fx::groupCount; ++group)
    {
        const auto enableParam = fx::getGroupEnableParam(static_cast<fx::Group>(group));
        const auto index = static_cast<size_t>(group);

        active[index] = p[enableParam] >= 0.5f;

        if (active[index] && ! wasActive[index])
        {
            // Start from silence so stale audio never bursts out of a
            // re-enabled effect, and ramps begin at the current setting.
            clearEffectState(group);

            switch (static_cast<fx::Group>(group))
            {
                case fx::Group::filter:
                    smoothedCutoff = p[fx::filterCutoff];
                    break;
                case fx::Group::drive:
                    previousDriveMix = p[fx::driveMix];
                    break;
                case fx::Group::chorus:
                    previousChorusMix = p[fx::chorusMix];
                    break;
                case fx::Group::delay:
                    smoothedDelaySamples = delayTarget;
                    previousDelayMix = p[fx::delayMix];
                    previousDelayFeedback = p[fx::delayFeedback];
                    break;
                case fx::Group::echo:
                    smoothedEchoSamples = echoTarget;
                    previousEchoMix = p[fx::echoMix];
                    previousEchoFeedback = p[fx::echoFeedback];
                    break;
                case fx::Group::reverb:
                    break;
            }
        }

        wasActive[index] = active[index];
        anyActive = anyActive || active[index];
    }

    if (! anyActive)
        return;

    juce::ScopedNoDenormals noDenormals;

    auto* left = buffer.getWritePointer(0, startSample);
    auto* right = buffer.getNumChannels() > 1
        ? buffer.getWritePointer(1, startSample)
        : nullptr;

    const auto inverseLength = 1.0f / static_cast<float>(numSamples);
    constexpr auto twoPi = juce::MathConstants<float>::twoPi;

    const auto filterActive = active[static_cast<size_t>(fx::Group::filter)];
    const auto driveActive = active[static_cast<size_t>(fx::Group::drive)];
    const auto chorusActive = active[static_cast<size_t>(fx::Group::chorus)];
    const auto delayActive = active[static_cast<size_t>(fx::Group::delay)];
    const auto echoActive = active[static_cast<size_t>(fx::Group::echo)];
    const auto reverbActive = active[static_cast<size_t>(fx::Group::reverb)];

    // Filter coefficients (zero-delay-feedback state variable filter).
    float a1 = 0.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;
    float k = 1.0f;
    const auto highPass = p[fx::filterHighPass] >= 0.5f;

    if (filterActive)
    {
        smoothedCutoff += (p[fx::filterCutoff] - smoothedCutoff) * 0.5f;

        const auto cutoff = juce::jlimit(20.0f, 0.45f * sr, smoothedCutoff);
        const auto g = std::tan(juce::MathConstants<float>::pi * cutoff / sr);

        k = juce::jmap(p[fx::filterResonance], 0.0f, 1.0f, 1.4142f, 0.12f);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    // Drive: tanh saturation, level-compensated so loudness stays comparable.
    const auto driveGain = juce::Decibels::decibelsToGain(p[fx::driveAmount] * 30.0f);
    const auto driveCompensation = 1.0f / std::sqrt(driveGain);
    auto driveMix = previousDriveMix;
    const auto driveMixStep = (p[fx::driveMix] - previousDriveMix) * inverseLength;

    // Chorus: two modulated taps, quadrature LFO between channels.
    const auto chorusIncrement = p[fx::chorusRate] / sr;
    const auto chorusBase = 0.010f * sr;
    const auto chorusDepthSamples = p[fx::chorusDepth] * 0.004f * sr;
    auto chorusMix = previousChorusMix;
    const auto chorusMixStep = (p[fx::chorusMix] - previousChorusMix) * inverseLength;

    // Delay: clean stereo feedback delay.
    auto delayMix = previousDelayMix;
    const auto delayMixStep = (p[fx::delayMix] - previousDelayMix) * inverseLength;
    auto delayFeedback = previousDelayFeedback;
    const auto delayFeedbackStep =
        (juce::jlimit(0.0f, 0.95f, p[fx::delayFeedback]) - previousDelayFeedback) * inverseLength;

    // Echo: ping-pong repeats that darken with each pass.
    auto echoMix = previousEchoMix;
    const auto echoMixStep = (p[fx::echoMix] - previousEchoMix) * inverseLength;
    auto echoFeedback = previousEchoFeedback;
    const auto echoFeedbackStep =
        (juce::jlimit(0.0f, 0.95f, p[fx::echoFeedback]) - previousEchoFeedback) * inverseLength;
    const auto echoToneCutoff = juce::jmap(p[fx::echoTone], 0.0f, 1.0f, 800.0f, 12000.0f);
    const auto echoToneCoefficient = 1.0f - std::exp(-twoPi * echoToneCutoff / sr);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        auto l = left[sample];
        auto r = right != nullptr ? right[sample] : l;

        if (filterActive)
        {
            l = filterLeft.process(l, a1, a2, a3, k, highPass);
            r = filterRight.process(r, a1, a2, a3, k, highPass);
        }

        if (driveActive)
        {
            driveMix += driveMixStep;

            const auto wetL = std::tanh(l * driveGain) * driveCompensation;
            const auto wetR = std::tanh(r * driveGain) * driveCompensation;

            l += (wetL - l) * driveMix;
            r += (wetR - r) * driveMix;
        }

        if (chorusActive)
        {
            chorusMix += chorusMixStep;

            chorusLeft.write(l);
            chorusRight.write(r);

            const auto modLeft = std::sin(twoPi * chorusPhase);
            const auto modRight = std::sin(twoPi * (chorusPhase + 0.25f));

            const auto wetL = chorusLeft.read(chorusBase + chorusDepthSamples * modLeft);
            const auto wetR = chorusRight.read(chorusBase + chorusDepthSamples * modRight);

            chorusPhase += chorusIncrement;

            if (chorusPhase >= 1.0f)
                chorusPhase -= 1.0f;

            l += wetL * chorusMix;
            r += wetR * chorusMix;
        }

        if (delayActive)
        {
            delayMix += delayMixStep;
            delayFeedback += delayFeedbackStep;
            smoothedDelaySamples +=
                (delayTarget - smoothedDelaySamples) * timeSmoothCoefficient;

            const auto wetL = delayLeft.read(smoothedDelaySamples);
            const auto wetR = delayRight.read(smoothedDelaySamples);

            delayLeft.write(l + wetL * delayFeedback);
            delayRight.write(r + wetR * delayFeedback);

            l += wetL * delayMix;
            r += wetR * delayMix;
        }

        if (echoActive)
        {
            echoMix += echoMixStep;
            echoFeedback += echoFeedbackStep;
            smoothedEchoSamples +=
                (echoTarget - smoothedEchoSamples) * timeSmoothCoefficient;

            const auto wetL = echoLeft.read(smoothedEchoSamples);
            const auto wetR = echoRight.read(smoothedEchoSamples);

            echoToneLeft += echoToneCoefficient * (wetL - echoToneLeft);
            echoToneRight += echoToneCoefficient * (wetR - echoToneRight);

            // Input enters the left line; each repeat crosses to the other side.
            echoLeft.write(0.5f * (l + r) + echoToneRight * echoFeedback);
            echoRight.write(echoToneLeft * echoFeedback);

            l += wetL * echoMix;
            r += wetR * echoMix;
        }

        left[sample] = l;

        if (right != nullptr)
            right[sample] = r;
    }

    previousDriveMix = p[fx::driveMix];
    previousChorusMix = p[fx::chorusMix];
    previousDelayMix = p[fx::delayMix];
    previousDelayFeedback = juce::jlimit(0.0f, 0.95f, p[fx::delayFeedback]);
    previousEchoMix = p[fx::echoMix];
    previousEchoFeedback = juce::jlimit(0.0f, 0.95f, p[fx::echoFeedback]);

    if (reverbActive)
    {
        juce::Reverb::Parameters reverbParameters;
        reverbParameters.roomSize = p[fx::reverbSize];
        reverbParameters.damping = p[fx::reverbDamping];
        // juce::Reverb scales wet by 3 and dry by 2: keep dry at unity.
        reverbParameters.wetLevel = p[fx::reverbMix] * 0.33f;
        reverbParameters.dryLevel = 0.5f;
        reverbParameters.width = 1.0f;
        reverbParameters.freezeMode = 0.0f;
        reverb.setParameters(reverbParameters);

        if (right != nullptr)
            reverb.processStereo(left, right, numSamples);
        else
            reverb.processMono(left, numSamples);
    }
}
