#include "../Source/TrackEffects.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
constexpr double testSampleRate = 48000.0;
constexpr int blockSize = 256;

void require(const bool condition, const std::string& message)
{
    if (! condition)
        throw std::runtime_error(message);
}

// Runs a whole signal through the effects in fixed-size blocks.
void render(TrackEffects& effects, juce::AudioBuffer<float>& signal)
{
    for (int start = 0; start < signal.getNumSamples(); start += blockSize)
    {
        const auto count = juce::jmin(blockSize, signal.getNumSamples() - start);
        effects.process(signal, start, count);
    }
}

juce::AudioBuffer<float> makeImpulse(const int length, const int channels = 2)
{
    juce::AudioBuffer<float> buffer(channels, length);
    buffer.clear();

    for (int channel = 0; channel < channels; ++channel)
        buffer.setSample(channel, 0, 1.0f);

    return buffer;
}

juce::AudioBuffer<float> makeSine(const double frequency, const int length)
{
    juce::AudioBuffer<float> buffer(2, length);

    for (int sample = 0; sample < length; ++sample)
    {
        const auto value = 0.5f * static_cast<float>(std::sin(
            juce::MathConstants<double>::twoPi * frequency * sample / testSampleRate));

        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, value);
    }

    return buffer;
}

float rms(const juce::AudioBuffer<float>& buffer, const int start, const int count)
{
    return buffer.getRMSLevel(0, start, count);
}

bool allFinite(const juce::AudioBuffer<float>& buffer, const float bound)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto value = buffer.getSample(channel, sample);

            if (! std::isfinite(value) || std::abs(value) > bound)
                return false;
        }
    }

    return true;
}

void testDefaultsAreTransparent()
{
    TrackEffects effects;
    effects.prepare(testSampleRate);
    require(! effects.hasActiveEffect(), "Effects must be off by default");

    auto signal = makeSine(440.0, 4800);
    const auto original = signal;
    render(effects, signal);

    for (int sample = 0; sample < signal.getNumSamples(); ++sample)
    {
        require(
            signal.getSample(0, sample) == original.getSample(0, sample),
            "Bypassed chain altered the signal"
        );
    }
}

void testDelayRepeatsAtRequestedTime()
{
    TrackEffects effects;
    effects.prepare(testSampleRate);
    effects.setParam(fx::delayOn, 1.0f);
    effects.setParam(fx::delayTime, 100.0f);
    effects.setParam(fx::delayFeedback, 0.5f);
    effects.setParam(fx::delayMix, 1.0f);

    auto signal = makeImpulse(24000);
    render(effects, signal);

    const auto delaySamples = 4800;
    require(std::abs(signal.getSample(0, 0) - 1.0f) < 0.001f, "Dry impulse lost");
    require(std::abs(signal.getSample(0, delaySamples) - 1.0f) < 0.02f,
            "First delay repeat missing or mistimed");
    require(std::abs(signal.getSample(0, 2 * delaySamples) - 0.5f) < 0.02f,
            "Feedback repeat has wrong level");
    require(std::abs(signal.getSample(0, delaySamples / 2)) < 0.001f,
            "Energy between repeats");
}

void testEchoPingPongsBetweenChannels()
{
    TrackEffects effects;
    effects.prepare(testSampleRate);
    effects.setParam(fx::echoOn, 1.0f);
    effects.setParam(fx::echoTime, 100.0f);
    effects.setParam(fx::echoFeedback, 0.8f);
    effects.setParam(fx::echoTone, 1.0f);
    effects.setParam(fx::echoMix, 1.0f);

    auto signal = makeImpulse(30000);
    render(effects, signal);

    const auto t = 4800;
    const auto firstLeft = std::abs(signal.getSample(0, t));
    const auto firstRight = std::abs(signal.getSample(1, t));
    require(firstLeft > 0.3f && firstRight < 0.01f, "First echo should be on the left");

    // Tone filter spreads the second repeat slightly: look at a small window.
    float secondLeft = 0.0f;
    float secondRight = 0.0f;

    for (int sample = 2 * t - 50; sample < 2 * t + 50; ++sample)
    {
        secondLeft = juce::jmax(secondLeft, std::abs(signal.getSample(0, sample)));
        secondRight = juce::jmax(secondRight, std::abs(signal.getSample(1, sample)));
    }

    require(secondRight > 0.05f, "Second echo should cross to the right");
    require(secondRight > 4.0f * secondLeft, "Second echo should not stay on the left");
}

void testReverbCreatesTail()
{
    TrackEffects effects;
    effects.prepare(testSampleRate);
    effects.setParam(fx::reverbOn, 1.0f);
    effects.setParam(fx::reverbSize, 0.8f);
    effects.setParam(fx::reverbMix, 1.0f);

    auto signal = makeImpulse(48000);
    render(effects, signal);

    require(rms(signal, 4800, 9600) > 0.0005f, "Reverb produced no tail");
    require(allFinite(signal, 4.0f), "Reverb output not finite");
}

void testLowPassAttenuatesHighFrequencies()
{
    TrackEffects effects;
    effects.prepare(testSampleRate);
    effects.setParam(fx::filterOn, 1.0f);
    effects.setParam(fx::filterCutoff, 200.0f);
    effects.setParam(fx::filterResonance, 0.0f);

    auto high = makeSine(5000.0, 24000);
    const auto before = rms(high, 12000, 6000);
    render(effects, high);
    const auto after = rms(high, 12000, 6000);

    require(after < before * 0.1f, "Low-pass did not attenuate 5 kHz by 20 dB");

    TrackEffects highPass;
    highPass.prepare(testSampleRate);
    highPass.setParam(fx::filterOn, 1.0f);
    highPass.setParam(fx::filterHighPass, 1.0f);
    highPass.setParam(fx::filterCutoff, 5000.0f);
    highPass.setParam(fx::filterResonance, 0.0f);

    auto low = makeSine(100.0, 24000);
    const auto lowBefore = rms(low, 12000, 6000);
    render(highPass, low);
    const auto lowAfter = rms(low, 12000, 6000);

    require(lowAfter < lowBefore * 0.1f, "High-pass did not attenuate 100 Hz");
}

void testDriveStaysBounded()
{
    TrackEffects effects;
    effects.prepare(testSampleRate);
    effects.setParam(fx::driveOn, 1.0f);
    effects.setParam(fx::driveAmount, 1.0f);
    effects.setParam(fx::driveMix, 1.0f);

    auto signal = makeSine(220.0, 9600);

    for (int channel = 0; channel < 2; ++channel)
        signal.applyGain(channel, 0, signal.getNumSamples(), 8.0f);

    render(effects, signal);
    require(allFinite(signal, 1.0f), "Drive output exceeded full scale");
    require(rms(signal, 0, 9600) > 0.01f, "Drive produced silence");
}

void testChorusModifiesSignal()
{
    TrackEffects effects;
    effects.prepare(testSampleRate);
    effects.setParam(fx::chorusOn, 1.0f);
    effects.setParam(fx::chorusMix, 1.0f);

    auto signal = makeSine(440.0, 9600);
    const auto original = signal;
    render(effects, signal);

    float difference = 0.0f;

    for (int sample = 4800; sample < 9600; ++sample)
    {
        difference = juce::jmax(
            difference,
            std::abs(signal.getSample(0, sample) - original.getSample(0, sample))
        );
    }

    require(difference > 0.05f, "Chorus left the signal untouched");
    require(allFinite(signal, 2.0f), "Chorus output not finite");
}

void testMonoBufferIsSupported()
{
    TrackEffects effects;
    effects.prepare(testSampleRate);

    for (const auto group : { fx::filterOn, fx::driveOn, fx::chorusOn, fx::delayOn,
                              fx::echoOn, fx::reverbOn })
    {
        effects.setParam(group, 1.0f);
    }

    juce::AudioBuffer<float> mono(1, 9600);
    mono.clear();
    mono.setSample(0, 0, 1.0f);
    render(effects, mono);
    require(allFinite(mono, 8.0f), "Mono processing produced invalid samples");
}

void testReenablingStartsClean()
{
    TrackEffects effects;
    effects.prepare(testSampleRate);
    effects.setParam(fx::delayOn, 1.0f);
    effects.setParam(fx::delayTime, 50.0f);
    effects.setParam(fx::delayMix, 1.0f);
    effects.setParam(fx::delayFeedback, 0.9f);

    auto burst = makeSine(440.0, 2400);
    render(effects, burst);

    effects.setParam(fx::delayOn, 0.0f);
    auto silence = juce::AudioBuffer<float>(2, 1024);
    silence.clear();
    render(effects, silence);

    effects.setParam(fx::delayOn, 1.0f);
    silence.clear();
    render(effects, silence);

    require(silence.getMagnitude(0, 1024) < 0.00001f,
            "Re-enabled delay replayed stale audio");
}

void testSerialisationRoundTrip()
{
    EffectsSettings settings;
    settings.values[fx::reverbOn] = 1.0f;
    settings.values[fx::reverbSize] = 0.83f;
    settings.values[fx::delayTime] = 612.0f;
    settings.values[fx::filterHighPass] = 1.0f;

    const auto json = juce::JSON::toString(settings.toVar());
    const auto restored = EffectsSettings::fromVar(juce::JSON::parse(json));

    for (int index = 0; index < fx::paramCount; ++index)
    {
        require(
            std::abs(restored.values[static_cast<size_t>(index)]
                     - settings.values[static_cast<size_t>(index)]) < 0.0001f,
            std::string("Round trip changed ") + fx::getParamInfo(index).id
        );
    }

    require(restored.hasActiveEffect(), "Reverb enable flag lost");
    require(restored.fingerprint() == settings.fingerprint(), "Fingerprint mismatch");
}

void testLegacyAndCorruptDataFallBackToDefaults()
{
    require(EffectsSettings::fromVar(juce::var()) == EffectsSettings(),
            "Missing effects data must give defaults");

    auto* object = new juce::DynamicObject();
    object->setProperty("delayTime", 999999.0);
    object->setProperty("reverbSize", -4.0);
    object->setProperty("echoMix", "not a number");
    const auto parsed = EffectsSettings::fromVar(juce::var(object));

    require(parsed.values[fx::delayTime] == fx::getParamInfo(fx::delayTime).maximum,
            "Out-of-range value not clamped to maximum");
    require(parsed.values[fx::reverbSize] == fx::getParamInfo(fx::reverbSize).minimum,
            "Out-of-range value not clamped to minimum");
}

void testParameterTableIsComplete()
{
    for (int index = 0; index < fx::paramCount; ++index)
    {
        const auto& info = fx::getParamInfo(index);
        require(info.id != nullptr && info.label != nullptr, "Parameter table entry missing");
        require(info.minimum < info.maximum, std::string("Bad range for ") + info.id);
        require(info.defaultValue >= info.minimum && info.defaultValue <= info.maximum,
                std::string("Default outside range for ") + info.id);
    }

    for (int group = 0; group < fx::groupCount; ++group)
    {
        const auto enable = fx::getGroupEnableParam(static_cast<fx::Group>(group));
        require(fx::getParamInfo(enable).kind == fx::Kind::enable, "Enable parameter mismatch");
    }
}

void testExtremeSettingsStayStable()
{
    TrackEffects effects;
    effects.prepare(testSampleRate);

    juce::Random random(1234);

    // Sweep random, extreme parameter values while audio runs.
    juce::AudioBuffer<float> block(2, blockSize);
    float peak = 0.0f;

    for (int iteration = 0; iteration < 2000; ++iteration)
    {
        if (iteration % 20 == 0)
        {
            for (int index = 0; index < fx::paramCount; ++index)
            {
                const auto& info = fx::getParamInfo(index);

                if (info.kind == fx::Kind::slider)
                {
                    const auto extreme = random.nextBool();
                    effects.setParam(index, extreme ? info.maximum : info.minimum);
                }
                else
                {
                    effects.setParam(index, random.nextFloat() > 0.3f ? 1.0f : 0.0f);
                }
            }
        }

        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < blockSize; ++sample)
                block.setSample(channel, sample, random.nextFloat() * 2.0f - 1.0f);

        effects.process(block, 0, blockSize);

        for (int channel = 0; channel < 2; ++channel)
        {
            for (int sample = 0; sample < blockSize; ++sample)
            {
                const auto value = block.getSample(channel, sample);
                require(std::isfinite(value), "Non-finite sample under extreme settings");
                peak = juce::jmax(peak, std::abs(value));
            }
        }
    }

    require(peak < 100.0f, "Output level ran away under extreme settings");
}

void testPrepareCanBeRepeatedAtNewRate()
{
    TrackEffects effects;
    effects.prepare(44100.0);
    effects.setParam(fx::delayOn, 1.0f);
    effects.setParam(fx::reverbOn, 1.0f);

    auto first = makeImpulse(4096);
    render(effects, first);

    effects.prepare(96000.0);
    auto second = makeImpulse(4096);
    render(effects, second);

    require(allFinite(second, 8.0f), "Output invalid after sample rate change");
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;

    struct TestCase
    {
        const char* name;
        void (*run)();
    };

    const TestCase tests[] =
    {
        { "defaults are transparent", testDefaultsAreTransparent },
        { "delay timing and feedback", testDelayRepeatsAtRequestedTime },
        { "echo ping-pong", testEchoPingPongsBetweenChannels },
        { "reverb tail", testReverbCreatesTail },
        { "filter low/high pass", testLowPassAttenuatesHighFrequencies },
        { "drive bounded", testDriveStaysBounded },
        { "chorus modulates", testChorusModifiesSignal },
        { "mono buffers", testMonoBufferIsSupported },
        { "re-enable starts clean", testReenablingStartsClean },
        { "serialisation round trip", testSerialisationRoundTrip },
        { "legacy/corrupt data", testLegacyAndCorruptDataFallBackToDefaults },
        { "parameter table", testParameterTableIsComplete },
        { "extreme settings stable", testExtremeSettingsStayStable },
        { "sample rate change", testPrepareCanBeRepeatedAtNewRate },
    };

    int failures = 0;

    for (const auto& test : tests)
    {
        try
        {
            test.run();
            std::cout << "PASS: " << test.name << '\n';
        }
        catch (const std::exception& error)
        {
            ++failures;
            std::cerr << "FAIL: " << test.name << " - " << error.what() << '\n';
        }
    }

    return failures == 0 ? 0 : 1;
}
