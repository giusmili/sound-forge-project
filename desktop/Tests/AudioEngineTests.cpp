#include "AudioEngine.h"
#include "AppServices.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace
{
int checks = 0;
void check(const bool condition, const char* description)
{
    ++checks;
    if (! condition) throw std::runtime_error(description);
}
void near(const double actual, const double expected, const double tolerance, const char* description)
{
    check(std::isfinite(actual) && std::abs(actual - expected) <= tolerance, description);
}

juce::File writeFixture(const juce::File& directory, const juce::String& name,
                       const double rate, const int channels, const int samples,
                       const float amplitude = 0.25f, juce::AudioFormat* formatOverride = nullptr)
{
    auto file = directory.getChildFile(name);
    juce::WavAudioFormat format;
    const auto options = juce::AudioFormatWriterOptions {}
        .withSampleRate(rate).withNumChannels(channels).withBitsPerSample(24);
    std::unique_ptr<juce::OutputStream> stream(file.createOutputStream());
    auto writer = (formatOverride != nullptr ? *formatOverride : format).createWriterFor(stream, options);
    check(writer != nullptr, "WAV fixture writer");
    juce::AudioBuffer<float> data(channels, samples);
    for (int c = 0; c < channels; ++c)
        for (int i = 0; i < samples; ++i)
            data.setSample(c, i, amplitude);
    check(writer->writeFromAudioSampleBuffer(data, 0, samples), "WAV fixture write");
    return file;
}

AudioTrack& import(AudioEngine& engine, const juce::File& file)
{
    AudioTrack* track = nullptr;
    const auto result = engine.addTrackFromFile(file, track);
    if (result.failed()) throw std::runtime_error(result.getErrorMessage().toStdString());
    check(track != nullptr, "import returns a track");
    return *track;
}

// The test driver is allowed to wait for disk. The real audio callback is not.
void render(AudioEngine& engine, juce::AudioBuffer<float>& buffer, const int offset = 0, const int count = 256)
{
    const auto before = engine.getPositionSeconds();
    for (int attempt = 0; attempt < 1000; ++attempt)
    {
        engine.getNextAudioBlock({ &buffer, offset, count });
        if (! engine.isPlaying() || engine.getPositionSeconds() > before)
            return;
        juce::Thread::sleep(1);
    }
    throw std::runtime_error("read-ahead did not become ready within one second");
}

void testImportAndTransport(const juce::File& mono, const juce::File& stereo, const juce::File& directory)
{
    AudioEngine engine(false);
    auto& first = import(engine, mono); // Import without a device must work.
    near(first.getLengthSeconds(), 2.0, 1e-9, "duration available without a device");
    engine.play();
    check(! engine.isPlaying(), "play without a prepared device stays stopped");
    engine.prepareToPlay(256, 48000.0); // Used to crash: null read-ahead thread.
    auto& second = import(engine, stereo);
    check(engine.getTrackCount() == 2, "multiple imports");
    engine.setMasterGain(1.0f);
    juce::AudioBuffer<float> output(2, 1024);
    engine.play();
    render(engine, output);
    near(engine.getPositionSeconds(), 256.0 / 48000.0, 1e-9, "one global clock");
    near(output.getSample(0, 200), 0.5, 0.002, "mixed-rate tracks sum on left");
    near(output.getSample(1, 200), 0.5, 0.002, "mono file duplicates to right");

    engine.pause();
    const auto paused = engine.getPositionSeconds();
    engine.getNextAudioBlock({ &output, 0, 256 });
    near(output.getMagnitude(0, 256), 0.0, 0.0, "pause emits silence");
    near(engine.getPositionSeconds(), paused, 0.0, "pause freezes time");
    engine.play();
    render(engine, output);
    check(engine.getPositionSeconds() > paused, "resume advances time");

    engine.setPositionSeconds(0.8);
    near(engine.getPositionSeconds(), 0.8, 1e-9, "seek while playing");
    render(engine, output);
    near(output.getSample(0, 200), 0.5, 0.002, "seek flushes resampler and reads both tracks");
    engine.stop();
    check(! engine.isPlaying(), "stop is immediate without a callback thread");
    near(engine.getPositionSeconds(), 0.0, 0.0, "stop rewinds");

    engine.play();
    render(engine, output);
    const auto previous = engine.getPositionSeconds();
    AudioTrack* rejected = &first;
    check(engine.addTrackFromFile(directory.getChildFile("missing.wav"), rejected).failed(), "missing file rejected");
    check(rejected == nullptr && engine.getTrackCount() == 2, "failure does not publish a track");
    check(engine.isPlaying(), "failed import preserves playing state");
    near(engine.getPositionSeconds(), previous, 0.0, "failed import preserves position");
    auto invalid = directory.getChildFile("broken.wav");
    check(invalid.replaceWithText("RIFF truncated invalid content"), "invalid fixture written");
    check(engine.addTrackFromFile(invalid, rejected).failed(), "corrupt file rejected");
    import(engine, mono);
    check(! engine.isPlaying(), "successful hot import stops");
    near(engine.getPositionSeconds(), 0.0, 0.0, "successful hot import rewinds");

    first.setGain(std::numeric_limits<float>::quiet_NaN());
    second.setPan(std::numeric_limits<float>::infinity());
    engine.setMasterGain(std::numeric_limits<float>::quiet_NaN());
    engine.setPositionSeconds(std::numeric_limits<double>::infinity());
    near(first.getGain(), 1.0, 0.0, "invalid gain defaults");
    near(second.getPan(), 0.0, 0.0, "invalid pan defaults");
    near(engine.getPositionSeconds(), 0.0, 0.0, "invalid seek defaults");
    std::cout << "PASS import, mixed sample rates, transport, failed import recovery\n";
}

void testMixer(const juce::File& mono)
{
    AudioEngine engine(false);
    engine.prepareToPlay(256, 48000.0);
    auto& a = import(engine, mono);
    auto& b = import(engine, mono);
    engine.setMasterGain(1.0f);
    juce::AudioBuffer<float> output(2, 1024);
    a.setPan(-1.0f);
    b.setPan(1.0f);
    a.setGain(0.5f);
    engine.play();
    render(engine, output);
    near(output.getSample(0, 200), 0.125, 0.001, "left pan and gain");
    near(output.getSample(1, 200), 0.25, 0.001, "right pan and gain");
    a.setSolo(true);
    engine.refreshSoloState();
    render(engine, output);
    near(output.getSample(1, 200), 0.0, 0.0, "solo suppresses other track");
    a.setMuted(true);
    render(engine, output);
    near(output.getMagnitude(0, 256), 0.0, 0.0, "mute overrides solo");
    a.setMuted(false);
    b.setSolo(true);
    engine.refreshSoloState();
    engine.setMasterGain(0.5f);
    render(engine, output);
    near(output.getSample(1, 200), 0.125, 0.001, "multiple solo and master gain");
    a.setSolo(false);
    b.setSolo(false);
    engine.refreshSoloState();
    a.setGain(1.0f);
    a.setPan(0.0f);
    b.setMuted(true);
    engine.setMasterGain(1.0f);
    juce::AudioBuffer<float> monoOutput(1, 256);
    render(engine, monoOutput);
    near(monoOutput.getSample(0, 200), 0.25, 0.001, "mono output preserves centered level");
    std::cout << "PASS volume, pan, mute, solo, master, mono output\n";
}

void testDeviceLifecycle(const juce::File& mono)
{
    for (int iteration = 0; iteration < 8; ++iteration)
    {
        AudioEngine engine(false);
        engine.prepareToPlay(128, 44100.0);
        import(engine, mono);
        engine.setMasterGain(1.0f);
        juce::AudioBuffer<float> output(2, 4096);
        engine.play();
        render(engine, output, 0, 128);
        const auto position = engine.getPositionSeconds();
        engine.releaseResources(); // USB disconnect / device switch while playing.
        check(! engine.isPlaying(), "device loss stops transport");
        engine.getNextAudioBlock({ &output, 0, 128 });
        near(output.getMagnitude(0, 128), 0.0, 0.0, "device loss emits silence");
        engine.prepareToPlay(64, 96000.0);
        near(engine.getPositionSeconds(), position, 0.0, "device restart preserves position");
        check(! engine.isPlaying(), "device restart requires explicit play");
        engine.play();
        render(engine, output, 0, 777); // Non-power-of-two, larger than prepared block.
        near(output.getSample(0, 200), 0.25, 0.002, "variable callback size");
        engine.stop();
        engine.setPositionSeconds(1.999);
        engine.play();
        render(engine, output);
        check(! engine.isPlaying(), "EOF stops transport");
        near(engine.getPositionSeconds(), 2.0, 1e-9, "EOF clock clamps to duration");
        engine.play();
        render(engine, output);
        check(engine.getPositionSeconds() < 0.01, "play after EOF rewinds");
        // Destruction while playing, with no hardware callback, must not wait
        // for per-track transport stop acknowledgements.
    }
    std::cout << "PASS device loss/restart, EOF/replay, repeated active destruction\n";
}

void testBuffersAndLimits(const juce::File& directory)
{
    AudioEngine engine(false);
    const auto hot = writeFixture(directory, "loud.wav", 48000, 2, 48000, 0.95f);
    engine.prepareToPlay(256, 48000);
    auto& a = import(engine, hot);
    auto& b = import(engine, hot);
    a.setGain(1.5f); b.setGain(1.5f);
    engine.setMasterGain(1.0f);
    engine.play();
    juce::AudioBuffer<float> output(4, 1024);
    for (int c = 0; c < 4; ++c)
        for (int i = 0; i < 1024; ++i) output.setSample(c, i, -0.7f);
    render(engine, output, 37, 256);
    near(output.getSample(0, 36), -0.7, 1e-6, "prefix guard preserved");
    near(output.getSample(0, 293), -0.7, 1e-6, "suffix guard preserved");
    near(output.getSample(0, 237), 1.0, 0.0, "master sum bounded");
    near(output.getMagnitude(2, 37, 256), 0.0, 0.0, "unused channels cleared");
    engine.getNextAudioBlock({ nullptr, 0, 0 });
    const auto surround = writeFixture(directory, "surround.wav", 48000, 4, 480);
    AudioTrack* rejected = nullptr;
    check(engine.addTrackFromFile(surround, rejected).failed(), "unsupported surround rejected explicitly");
    std::cout << "PASS output bounds, buffer offsets, unused channels, channel validation\n";
}

void testDifferentDurations(const juce::File& mono, const juce::File& directory)
{
    AudioEngine engine(false);
    engine.prepareToPlay(256, 48000);
    const auto shortFile = writeFixture(directory, "short.wav", 48000, 1, 4800);
    import(engine, shortFile);
    import(engine, mono);
    engine.setMasterGain(1.0f);
    engine.setPositionSeconds(0.5);
    engine.play();
    juce::AudioBuffer<float> output(2, 256);
    render(engine, output);
    near(output.getSample(0, 200), 0.25, 0.001, "short track silent after its end");
    check(engine.isPlaying(), "longer track continues after shorter track ends");
    engine.setPositionSeconds(0.0);
    render(engine, output);
    near(output.getSample(0, 200), 0.5, 0.001, "seek back restores shorter track");
    std::cout << "PASS tracks with different durations and seek after EOF\n";
}

void testConcurrency(const juce::File& mono)
{
    AudioEngine engine(false);
    engine.prepareToPlay(64, 48000);
    auto& first = import(engine, mono);
    std::atomic<bool> running { true };
    std::atomic<bool> finite { true };
    std::atomic<int> callbacks { 0 };
    std::thread callback([&]
    {
        juce::AudioBuffer<float> output(2, 128);
        while (running.load())
        {
            engine.getNextAudioBlock({ &output, 0, 128 });
            ++callbacks;
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 128; ++i)
                    if (! std::isfinite(output.getSample(c, i))) finite.store(false);
            juce::Thread::sleep(1);
        }
    });
    // Ensure the callback is joined even if a control assertion throws.
    try
    {
        for (int i = 0; i < 100; ++i)
        {
            engine.play();
            engine.setPositionSeconds((i % 15) * 0.1);
            first.setGain((i % 15) * 0.1f);
            first.setSolo(i % 2 == 0);
            engine.refreshSoloState();
            juce::Thread::sleep(2);
            if (i % 10 == 0) import(engine, mono);
            if (i % 5 == 0)
            {
                engine.releaseResources();
                engine.prepareToPlay(128, i % 2 == 0 ? 44100 : 48000);
            }
            engine.pause(); engine.stop();
        }
    }
    catch (...)
    {
        running.store(false); callback.join(); throw;
    }
    running.store(false); callback.join();
    check(finite.load(), "concurrent callbacks remain finite");
    check(callbacks.load() >= 10, "stress test executes callbacks concurrently");
    check(engine.getTrackCount() == 11, "concurrent imports retained");
    std::cout << "PASS concurrent render/import/seek/stop/device restart (100 cycles)\n";
}

void testSettings()
{
    check(AppServices::getConfigFile().existsAsFile(), "settings created on first launch");
    auto settings = AppServices::loadSettings();
    near(static_cast<double>(settings["masterGain"]), 0.8, 1e-6, "default master gain");
    settings.getDynamicObject()->setProperty("masterGain", 0.42);
    settings.getDynamicObject()->setProperty("audioDeviceState", "<DEVICESETUP audioDeviceRate=\"48000\"/>");
    check(AppServices::saveSettings(settings), "atomic settings write");
    near(static_cast<double>(AppServices::loadSettings()["masterGain"]), 0.42, 1e-6, "settings round trip");
    check(AppServices::getConfigFile().replaceWithText("{broken json"), "corrupt settings fixture");
    settings = AppServices::loadSettings();
    near(static_cast<double>(settings["masterGain"]), 0.8, 1e-6, "corrupt settings recover defaults");
    check(AppServices::getConfigFile().getSiblingFile("settings.invalid.json").existsAsFile(), "corrupt settings preserved");
    check(AppServices::saveSettings(settings), "recovered settings save");
    std::cout << "PASS settings creation, persistence, corruption recovery\n";
}

void testFormatsAndWaveform(const juce::File& directory)
{
    juce::AiffAudioFormat aiff;
    juce::FlacAudioFormat flac;
    juce::OggVorbisAudioFormat ogg;
    for (auto* format : { static_cast<juce::AudioFormat*>(&aiff),
                         static_cast<juce::AudioFormat*>(&flac),
                         static_cast<juce::AudioFormat*>(&ogg) })
    {
        const auto file = writeFixture(directory, "format" + format->getFileExtensions()[0],
                                       44100, 2, 22050, 0.25f, format);
        AudioEngine engine(false);
        engine.prepareToPlay(256, 48000);
        import(engine, file);
        engine.play();
        juce::AudioBuffer<float> output(2, 256);
        render(engine, output);
        check(output.getMagnitude(0, 256) > 0.05f, "encoded format produces audio");
    }
    AudioEngine engine(false);
    engine.prepareToPlay(256, 48000);
    const auto mp3 = juce::File(SONOFORGE_TEST_FIXTURE_DIR).getChildFile("tone-44100.mp3");
    import(engine, mp3);
    engine.play();
    juce::AudioBuffer<float> output(2, 256);
    float maximum = 0.0f;
    for (int block = 0; block < 20; ++block)
    {
        render(engine, output);
        maximum = juce::jmax(maximum, output.getMagnitude(0, 256));
    }
    check(maximum > 0.02f, "MP3 decoder produces audio");
    juce::AudioThumbnail thumbnail(128, engine.getFormatManager(), engine.getThumbnailCache());
    check(thumbnail.setSource(new juce::FileInputSource(mp3)), "waveform accepts MP3");
    for (int attempt = 0; attempt < 1000 && ! thumbnail.isFullyLoaded(); ++attempt)
        juce::Thread::sleep(1);
    check(thumbnail.isFullyLoaded() && thumbnail.getTotalLength() > 0.2, "background waveform completes");
    std::cout << "PASS AIFF, FLAC, OGG, MP3 decoding and asynchronous waveform\n";
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    const auto directory = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("sonoforge-tests", "", false);
    if (directory.createDirectory().failed()) return 2;
    AppServices::initialise(directory.getChildFile("app-data"));
    int exitCode = 0;
    try
    {
        const auto mono = writeFixture(directory, juce::String::fromUTF8("piste été mono.wav"), 48000, 1, 96000);
        const auto stereo = writeFixture(directory, "stereo-44100.wav", 44100, 2, 88200);
        testImportAndTransport(mono, stereo, directory);
        testMixer(mono);
        testDeviceLifecycle(mono);
        testBuffersAndLimits(directory);
        testDifferentDurations(mono, directory);
        testConcurrency(mono);
        testFormatsAndWaveform(directory);
        testSettings();
        std::cout << "All " << checks << " checks passed.\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "FAIL after " << checks << " checks: " << e.what() << '\n';
        exitCode = 1;
    }
    AppServices::shutdown();
    directory.deleteRecursively();
    return exitCode;
}
