#include "../Source/MixdownRenderer.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (! condition)
        throw std::runtime_error(message);
}

bool near(const double a, const double b, const double tolerance)
{
    return std::abs(a - b) <= tolerance;
}

class Fixture
{
public:
    Fixture()
        : directory(
              juce::File::getSpecialLocation(juce::File::tempDirectory)
                  .getChildFile("sonoforge-mixdown-" + juce::Uuid().toString()))
    {
        directory.createDirectory();
        formatManager.registerBasicFormats();
    }

    ~Fixture()
    {
        directory.deleteRecursively();
    }

    // Mono 32-bit float WAV so the source values are stored exactly.
    juce::File writeSource(
        const juce::String& name,
        const double sampleRate,
        const int numSamples,
        const std::function<float(int)>& value
    )
    {
        auto file = directory.getChildFile(name);
        std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
        require(stream != nullptr, "Cannot create fixture file");

        juce::WavAudioFormat format;
        auto writer = format.createWriterFor(
            stream,
            juce::AudioFormatWriterOptions()
                .withSampleRate(sampleRate)
                .withNumChannels(1)
                .withBitsPerSample(32)
                .withSampleFormat(
                    juce::AudioFormatWriterOptions::SampleFormat::floatingPoint)
        );
        require(writer != nullptr, "Cannot create fixture writer");

        juce::AudioBuffer<float> buffer(1, numSamples);

        for (int sample = 0; sample < numSamples; ++sample)
            buffer.setSample(0, sample, value(sample));

        require(writer->writeFromAudioSampleBuffer(buffer, 0, numSamples),
                "Cannot write fixture");
        return file;
    }

    juce::File dc(const juce::String& name, const double seconds, const float level,
                  const double rate = 48000.0)
    {
        return writeSource(
            name, rate, static_cast<int>(seconds * rate),
            [level](int) { return level; });
    }

    juce::AudioBuffer<float> read(const juce::File& file, double* rate = nullptr)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(file));
        require(reader != nullptr, "Exported file cannot be read back");

        if (rate != nullptr)
            *rate = reader->sampleRate;

        require(reader->numChannels == 2, "Export must be stereo");

        juce::AudioBuffer<float> buffer(2, static_cast<int>(reader->lengthInSamples));
        reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true);
        return buffer;
    }

    juce::File directory;
    juce::AudioFormatManager formatManager;
};

AudioEngine::TrackState makeTrack(
    const juce::File& file,
    const double duration,
    const double offset = 0.0,
    const float gain = 1.0f,
    const float pan = 0.0f
)
{
    AudioEngine::TrackState state;
    state.sourceFile = file;
    state.startOffsetSeconds = offset;
    state.sourceStartSeconds = 0.0;
    state.sourceEndSeconds = duration;
    state.gain = gain;
    state.pan = pan;
    return state;
}

MixdownOptions makeOptions(
    Fixture& fixture,
    const juce::String& name,
    const MixdownOptions::Format format = MixdownOptions::Format::wav32Float
)
{
    MixdownOptions options;
    options.outputFile = fixture.directory.getChildFile(name);
    options.format = format;
    options.sampleRate = 48000.0;
    return options;
}

float at(const juce::AudioBuffer<float>& buffer, const int channel, const double seconds,
         const double rate = 48000.0)
{
    return buffer.getSample(channel, static_cast<int>(seconds * rate));
}

void testSingleClipRoundTrip()
{
    Fixture fixture;
    const auto source = fixture.writeSource(
        "tone.wav", 48000.0, 24000,
        [](int sample)
        {
            return 0.25f * static_cast<float>(
                std::sin(juce::MathConstants<double>::twoPi * 440.0 * sample / 48000.0));
        });

    const auto options = makeOptions(fixture, "out.wav");
    const auto result = MixdownRenderer::render(
        fixture.formatManager, { makeTrack(source, 0.5) }, options);

    require(result.status.wasOk(), "Render failed: " + result.status.getErrorMessage().toStdString());
    require(near(result.durationSeconds, 0.5, 1.0e-6), "Wrong duration");

    double rate = 0.0;
    const auto out = fixture.read(options.outputFile, &rate);
    require(near(rate, 48000.0, 0.1), "Wrong file sample rate");
    require(out.getNumSamples() == 24000, "Wrong length");

    // Mono source plays on both sides, sample for sample.
    float worst = 0.0f;

    for (int sample = 0; sample < 24000; ++sample)
    {
        const auto expected = 0.25f * static_cast<float>(
            std::sin(juce::MathConstants<double>::twoPi * 440.0 * sample / 48000.0));
        worst = juce::jmax(worst, std::abs(out.getSample(0, sample) - expected));
        worst = juce::jmax(worst, std::abs(out.getSample(1, sample) - expected));
    }

    require(worst < 0.001f, "Exported audio differs from source (worst "
                                + std::to_string(worst) + ")");
}

void testOffsetGainPanMuteAndMaster()
{
    Fixture fixture;
    const auto dcFile = fixture.dc("dc.wav", 0.5, 0.4f);

    auto muted = makeTrack(dcFile, 0.5, 0.0, 1.0f);
    muted.muted = true;

    auto options = makeOptions(fixture, "mix.wav");
    options.masterGain = 0.5f;

    const auto result = MixdownRenderer::render(
        fixture.formatManager,
        { makeTrack(dcFile, 0.5, 0.0, 0.5f),
          makeTrack(dcFile, 0.5, 0.25, 1.0f, -1.0f),
          muted },
        options);

    require(result.status.wasOk(), "Render failed");
    require(near(result.durationSeconds, 0.75, 1.0e-6), "Length must reach the last clip end");

    const auto out = fixture.read(options.outputFile);

    // Only the first clip: gain 0.5, master 0.5.
    require(near(at(out, 0, 0.1), 0.1, 0.002) && near(at(out, 1, 0.1), 0.1, 0.002),
            "Gain / master wrong before overlap");
    // Both clips; second is hard left.
    require(near(at(out, 0, 0.35), 0.3, 0.002), "Left sum wrong in overlap");
    require(near(at(out, 1, 0.35), 0.1, 0.002), "Hard-left pan leaked to the right");
    // Only the second clip.
    require(near(at(out, 0, 0.6), 0.2, 0.002) && near(at(out, 1, 0.6), 0.0, 0.002),
            "Clip offset or pan wrong after first clip ended");
    require(near(result.peak, 0.3f, 0.002), "Reported peak wrong");
}

void testSoloOnlyRendersSoloedClips()
{
    Fixture fixture;
    const auto quiet = fixture.dc("quiet.wav", 0.2, 0.1f);
    const auto loud = fixture.dc("loud.wav", 0.2, 0.5f);

    auto soloed = makeTrack(loud, 0.2);
    soloed.solo = true;

    const auto options = makeOptions(fixture, "solo.wav");
    const auto result = MixdownRenderer::render(
        fixture.formatManager, { makeTrack(quiet, 0.2), soloed }, options);

    require(result.status.wasOk(), "Solo render failed");
    const auto out = fixture.read(options.outputFile);
    require(near(at(out, 0, 0.1), 0.5, 0.002), "Solo must silence the other clips");
}

void testTrimmedClipUsesTheRightSourceRange()
{
    Fixture fixture;
    // First half 0.1, second half 0.6.
    const auto source = fixture.writeSource(
        "halves.wav", 48000.0, 48000,
        [](int sample) { return sample < 24000 ? 0.1f : 0.6f; });

    auto trimmed = makeTrack(source, 1.0, 0.0);
    trimmed.sourceStartSeconds = 0.5;
    trimmed.sourceEndSeconds = 0.8;

    const auto options = makeOptions(fixture, "trim.wav");
    const auto result = MixdownRenderer::render(
        fixture.formatManager, { trimmed }, options);

    require(result.status.wasOk(), "Trim render failed");
    const auto out = fixture.read(options.outputFile);
    require(near(out.getNumSamples(), 14400, 1), "Trimmed length wrong");
    require(near(at(out, 0, 0.05), 0.6, 0.002), "Trimmed clip played the wrong part");
}

void testSampleRateConversion()
{
    Fixture fixture;
    const auto source = fixture.dc("one-second.wav", 1.0, 0.3f);

    auto options = makeOptions(fixture, "44k.wav");
    options.sampleRate = 44100.0;

    const auto result = MixdownRenderer::render(
        fixture.formatManager, { makeTrack(source, 1.0) }, options);

    require(result.status.wasOk(), "Resampled render failed");

    double rate = 0.0;
    const auto out = fixture.read(options.outputFile, &rate);
    require(near(rate, 44100.0, 0.1), "File sample rate not 44100");
    require(near(out.getNumSamples(), 44100, 1), "Resampled length wrong");
    require(near(at(out, 0, 0.5, 44100.0), 0.3, 0.005), "Resampled level wrong");
}

void testEffectTailIsRendered()
{
    Fixture fixture;
    const auto source = fixture.dc("burst.wav", 0.5, 0.4f);

    auto track = makeTrack(source, 0.5);
    track.effects.values[fx::delayOn] = 1.0f;
    track.effects.values[fx::delayTime] = 600.0f;
    track.effects.values[fx::delayFeedback] = 0.0f;
    track.effects.values[fx::delayMix] = 1.0f;

    const auto options = makeOptions(fixture, "tail.wav");
    const auto result = MixdownRenderer::render(
        fixture.formatManager, { track }, options);

    require(result.status.wasOk(), "Effect render failed");
    require(near(result.durationSeconds, 3.5, 1.0e-6), "Tail not added to the length");

    const auto out = fixture.read(options.outputFile);
    require(near(at(out, 0, 0.3), 0.4, 0.01), "Dry signal lost");
    require(near(at(out, 0, 0.9), 0.4, 0.02), "Delay repeat missing after the clip ended");
    require(near(at(out, 0, 1.5), 0.0, 0.01), "Delay repeat did not end");
}

void testNormaliseAndClippingReport()
{
    Fixture fixture;
    const auto quiet = fixture.dc("quiet.wav", 0.3, 0.1f);

    auto options = makeOptions(fixture, "norm.wav");
    options.normalise = true;

    auto result = MixdownRenderer::render(
        fixture.formatManager, { makeTrack(quiet, 0.3) }, options);

    require(result.status.wasOk(), "Normalised render failed");
    const auto target = std::pow(10.0, -1.0 / 20.0);
    require(near(result.peak, target, 0.002), "Normalised peak not -1 dBFS");

    const auto out = fixture.read(options.outputFile);
    require(near(at(out, 0, 0.1), target, 0.002), "Normalised file level wrong");

    // Two loud clips sum above full scale: report it instead of hiding it.
    const auto hot = fixture.dc("hot.wav", 0.2, 0.9f);
    auto clipping = makeOptions(fixture, "clip.wav", MixdownOptions::Format::wav16);
    result = MixdownRenderer::render(
        fixture.formatManager, { makeTrack(hot, 0.2), makeTrack(hot, 0.2) }, clipping);

    require(result.status.wasOk(), "Clipping render failed");
    require(result.clippedSamples > 0, "Clipping not reported");
    require(result.peak > 1.5f, "Peak must show the real overshoot");
}

void testFlacIsLossless()
{
    Fixture fixture;
    const auto source = fixture.dc("flac-source.wav", 0.3, 0.25f);

    const auto options = makeOptions(
        fixture, "out.flac", MixdownOptions::Format::flac24);
    const auto result = MixdownRenderer::render(
        fixture.formatManager, { makeTrack(source, 0.3) }, options);

    require(result.status.wasOk(), "FLAC render failed");
    const auto out = fixture.read(options.outputFile);
    require(near(at(out, 0, 0.1), 0.25, 0.0001), "FLAC level wrong");
}

void testCancelKeepsExistingFile()
{
    Fixture fixture;
    const auto source = fixture.dc("long.wav", 2.0, 0.2f);
    const auto options = makeOptions(fixture, "keep.wav");

    options.outputFile.replaceWithText("previous export");

    int calls = 0;
    const auto result = MixdownRenderer::render(
        fixture.formatManager, { makeTrack(source, 2.0) }, options,
        [&calls](double)
        {
            return ++calls < 3;
        });

    require(result.cancelled, "Cancel not reported");
    require(options.outputFile.loadFileAsString() == "previous export",
            "Cancelled export damaged the previous file");

    // No stray temporary file may be left behind.
    require(fixture.directory.findChildFiles(juce::File::findFiles, false, "*.wav").size() == 2,
            "Temporary file left after cancel");
}

void testErrorsAreReported()
{
    Fixture fixture;

    auto options = makeOptions(fixture, "none.wav");
    auto result = MixdownRenderer::render(fixture.formatManager, {}, options);
    require(result.status.failed(), "Empty project must not export");
    require(! options.outputFile.existsAsFile(), "File created for an empty project");

    const auto missing = fixture.directory.getChildFile("does-not-exist.wav");
    result = MixdownRenderer::render(
        fixture.formatManager, { makeTrack(missing, 1.0) }, options);
    require(result.status.failed(), "Missing source must fail");
    require(! options.outputFile.existsAsFile(), "File created despite a missing source");

    auto allMuted = makeTrack(fixture.dc("m.wav", 0.2, 0.2f), 0.2);
    allMuted.muted = true;
    result = MixdownRenderer::render(fixture.formatManager, { allMuted }, options);
    require(result.status.failed(), "Fully muted project must not export");
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
        { "single clip round trip", testSingleClipRoundTrip },
        { "offset, gain, pan, mute, master", testOffsetGainPanMuteAndMaster },
        { "solo", testSoloOnlyRendersSoloedClips },
        { "trimmed clip source range", testTrimmedClipUsesTheRightSourceRange },
        { "sample rate conversion", testSampleRateConversion },
        { "effect tail", testEffectTailIsRendered },
        { "normalise and clipping report", testNormaliseAndClippingReport },
        { "flac lossless", testFlacIsLossless },
        { "cancel keeps existing file", testCancelKeepsExistingFile },
        { "errors", testErrorsAreReported },
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
