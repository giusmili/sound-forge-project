#include "MixdownRenderer.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int blockSize = 1024;
constexpr double automaticTailSeconds = 3.0;
constexpr float normaliseTargetDb = -1.0f;

[[nodiscard]] bool isTrackAudible(
    const AudioEngine::TrackState& state,
    const bool anySolo
)
{
    if (state.muted)
        return false;

    if (anySolo && ! state.solo)
        return false;

    return state.sourceEndSeconds - state.sourceStartSeconds > 0.0;
}

[[nodiscard]] bool hasSolo(
    const std::vector<AudioEngine::TrackState>& tracks
)
{
    return std::any_of(
        tracks.begin(),
        tracks.end(),
        [](const auto& state)
        {
            return state.solo;
        }
    );
}

struct Voice
{
    std::unique_ptr<juce::AudioFormatReaderSource> source;
    juce::AudioTransportSource transport;
    TrackEffects effects;
    juce::int64 startSample = 0;
    juce::int64 lengthSamples = 0;
    float gain = 1.0f;
    float pan = 0.0f;
    bool hasEffects = false;
};
} // namespace

juce::String MixdownRenderer::getFileExtension(
    const MixdownOptions::Format format
)
{
    switch (format)
    {
        case MixdownOptions::Format::flac16:
        case MixdownOptions::Format::flac24:
            return ".flac";

        case MixdownOptions::Format::wav16:
        case MixdownOptions::Format::wav24:
        case MixdownOptions::Format::wav32Float:
            break;
    }

    return ".wav";
}

juce::String MixdownRenderer::getFormatDescription(
    const MixdownOptions::Format format
)
{
    switch (format)
    {
        case MixdownOptions::Format::wav16:
            return "WAV 16 bits";
        case MixdownOptions::Format::wav24:
            return "WAV 24 bits";
        case MixdownOptions::Format::wav32Float:
            return "WAV 32 bits flottant";
        case MixdownOptions::Format::flac16:
            return "FLAC 16 bits";
        case MixdownOptions::Format::flac24:
            return "FLAC 24 bits";
    }

    return "WAV 24 bits";
}

double MixdownRenderer::calculateLengthSeconds(
    const std::vector<AudioEngine::TrackState>& tracks,
    const MixdownOptions& options
)
{
    const auto anySolo = hasSolo(tracks);

    double end = 0.0;
    bool anyEffect = false;

    for (const auto& state : tracks)
    {
        if (! isTrackAudible(state, anySolo))
            continue;

        end = juce::jmax(
            end,
            state.startOffsetSeconds
                + (state.sourceEndSeconds - state.sourceStartSeconds)
        );

        anyEffect = anyEffect || state.effects.hasActiveEffect();
    }

    if (end <= 0.0)
        return 0.0;

    if (options.tailSeconds >= 0.0)
        return end + options.tailSeconds;

    return end + (anyEffect ? automaticTailSeconds : 0.0);
}

MixdownResult MixdownRenderer::runPass(
    juce::AudioFormatManager& formatManager,
    const std::vector<AudioEngine::TrackState>& tracks,
    const MixdownOptions& options,
    const float extraGain,
    juce::AudioFormatWriter* writer,
    const ProgressCallback& progress,
    const double progressStart,
    const double progressSpan
)
{
    MixdownResult result;

    const auto sampleRate = options.sampleRate;
    const auto anySolo = hasSolo(tracks);

    std::vector<std::unique_ptr<Voice>> voices;

    for (const auto& state : tracks)
    {
        if (! isTrackAudible(state, anySolo))
            continue;

        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(state.sourceFile)
        );

        if (reader == nullptr)
        {
            result.status = juce::Result::fail(
                "Impossible de lire le fichier audio : "
                + state.sourceFile.getFullPathName()
            );
            return result;
        }

        auto voice = std::make_unique<Voice>();
        const auto sourceRate = reader->sampleRate;

        voice->source = std::make_unique<juce::AudioFormatReaderSource>(
            reader.release(),
            true
        );

        // Same chain as live playback, minus the read-ahead thread so the
        // reads are synchronous and the render is deterministic.
        voice->transport.setSource(
            voice->source.get(),
            0,
            nullptr,
            sourceRate
        );
        voice->transport.prepareToPlay(blockSize, sampleRate);
        voice->transport.setPosition(state.sourceStartSeconds);
        voice->transport.start();

        voice->effects.apply(state.effects);
        voice->effects.prepare(sampleRate);
        voice->hasEffects = state.effects.hasActiveEffect();

        voice->startSample = static_cast<juce::int64>(
            std::llround(state.startOffsetSeconds * sampleRate)
        );
        voice->lengthSamples = static_cast<juce::int64>(
            std::llround(
                (state.sourceEndSeconds - state.sourceStartSeconds)
                * sampleRate
            )
        );
        voice->gain = state.gain;
        voice->pan = state.pan;

        voices.push_back(std::move(voice));
    }

    const auto totalSamples = static_cast<juce::int64>(
        std::llround(calculateLengthSeconds(tracks, options) * sampleRate)
    );

    result.durationSeconds =
        static_cast<double>(totalSamples) / sampleRate;

    const auto masterGain =
        juce::jlimit(0.0f, 1.0f, options.masterGain) * extraGain;

    juce::AudioBuffer<float> mix(2, blockSize);
    juce::AudioBuffer<float> voiceBuffer(2, blockSize);
    juce::AudioBuffer<float> readBuffer(2, blockSize);

    float peak = 0.0f;

    for (juce::int64 position = 0; position < totalSamples; position += blockSize)
    {
        const auto count = static_cast<int>(
            juce::jmin<juce::int64>(blockSize, totalSamples - position)
        );

        mix.clear();

        for (auto& voice : voices)
        {
            const auto from = juce::jmax(position, voice->startSample);
            const auto to = juce::jmin(
                position + count,
                voice->startSample + voice->lengthSamples
            );
            const auto overlaps = to > from;

            // Idle voices only matter while an effect tail is still ringing.
            if (! overlaps && ! voice->hasEffects)
                continue;

            voiceBuffer.clear();

            if (overlaps)
            {
                const auto readCount = static_cast<int>(to - from);
                const auto offset = static_cast<int>(from - position);

                readBuffer.clear();

                juce::AudioSourceChannelInfo info(&readBuffer, 0, readCount);
                voice->transport.getNextAudioBlock(info);

                for (int channel = 0; channel < 2; ++channel)
                {
                    voiceBuffer.copyFrom(
                        channel,
                        offset,
                        readBuffer,
                        channel,
                        0,
                        readCount
                    );
                }
            }

            voice->effects.process(voiceBuffer, 0, count);

            AudioTrack::applyGainAndPan(
                voiceBuffer,
                0,
                count,
                voice->gain,
                voice->pan
            );

            for (int channel = 0; channel < 2; ++channel)
                mix.addFrom(channel, 0, voiceBuffer, channel, 0, count);
        }

        mix.applyGain(0, count, masterGain);

        for (int channel = 0; channel < 2; ++channel)
        {
            const auto* data = mix.getReadPointer(channel);

            for (int sample = 0; sample < count; ++sample)
            {
                const auto magnitude = std::abs(data[sample]);

                peak = juce::jmax(peak, magnitude);

                if (magnitude > 1.0f)
                    ++result.clippedSamples;
            }
        }

        if (writer != nullptr
            && ! writer->writeFromAudioSampleBuffer(mix, 0, count))
        {
            result.status = juce::Result::fail(
                "Ecriture du fichier impossible (disque plein ou acces refuse)."
            );
            return result;
        }

        if (progress)
        {
            const auto fraction =
                static_cast<double>(position + count)
                / static_cast<double>(totalSamples);

            if (! progress(progressStart + progressSpan * fraction))
            {
                result.cancelled = true;
                return result;
            }
        }
    }

    result.peak = peak;
    return result;
}

MixdownResult MixdownRenderer::render(
    juce::AudioFormatManager& formatManager,
    const std::vector<AudioEngine::TrackState>& tracks,
    const MixdownOptions& options,
    const ProgressCallback& progress
)
{
    MixdownResult failure;

    if (options.outputFile.getFullPathName().isEmpty()
        || options.sampleRate < 8000.0)
    {
        failure.status = juce::Result::fail("Reglages d'export invalides.");
        return failure;
    }

    if (calculateLengthSeconds(tracks, options) <= 0.0)
    {
        failure.status = juce::Result::fail(
            "Rien a exporter : aucune piste audio audible."
        );
        return failure;
    }

    // Peak pass first when normalising, so the written pass is a single sweep.
    float extraGain = 1.0f;
    double writeStart = 0.0;
    double writeSpan = 1.0;

    if (options.normalise)
    {
        auto analysis = runPass(
            formatManager,
            tracks,
            options,
            1.0f,
            nullptr,
            progress,
            0.0,
            0.4
        );

        if (analysis.cancelled || analysis.status.failed())
            return analysis;

        if (analysis.peak > 0.00001f)
        {
            extraGain =
                juce::Decibels::decibelsToGain(normaliseTargetDb)
                / analysis.peak;
        }

        writeStart = 0.4;
        writeSpan = 0.6;
    }

    const auto parent = options.outputFile.getParentDirectory();

    if (! parent.exists() && ! parent.createDirectory())
    {
        failure.status = juce::Result::fail(
            "Impossible de creer le dossier d'export."
        );
        return failure;
    }

    // Written next to the target and moved into place, so a failure or a
    // cancel never leaves a half-written file or destroys an older export.
    juce::TemporaryFile temporary(options.outputFile);

    auto stream = temporary.getFile().createOutputStream();

    if (stream == nullptr)
    {
        failure.status = juce::Result::fail(
            "Impossible de creer le fichier d'export."
        );
        return failure;
    }

    std::unique_ptr<juce::OutputStream> outputStream = std::move(stream);

    std::unique_ptr<juce::AudioFormat> audioFormat;
    int bitsPerSample = 24;

    switch (options.format)
    {
        case MixdownOptions::Format::wav16:
            audioFormat = std::make_unique<juce::WavAudioFormat>();
            bitsPerSample = 16;
            break;
        case MixdownOptions::Format::wav24:
            audioFormat = std::make_unique<juce::WavAudioFormat>();
            bitsPerSample = 24;
            break;
        case MixdownOptions::Format::wav32Float:
            audioFormat = std::make_unique<juce::WavAudioFormat>();
            bitsPerSample = 32;
            break;
        case MixdownOptions::Format::flac16:
            audioFormat = std::make_unique<juce::FlacAudioFormat>();
            bitsPerSample = 16;
            break;
        case MixdownOptions::Format::flac24:
            audioFormat = std::make_unique<juce::FlacAudioFormat>();
            bitsPerSample = 24;
            break;
    }

    auto writerOptions = juce::AudioFormatWriterOptions()
        .withSampleRate(options.sampleRate)
        .withNumChannels(2)
        .withBitsPerSample(bitsPerSample);

    if (options.format == MixdownOptions::Format::wav32Float)
    {
        writerOptions = writerOptions.withSampleFormat(
            juce::AudioFormatWriterOptions::SampleFormat::floatingPoint
        );
    }

    auto writer = audioFormat->createWriterFor(outputStream, writerOptions);

    if (writer == nullptr)
    {
        failure.status = juce::Result::fail(
            "Ce format ne prend pas en charge ce taux d'echantillonnage."
        );
        return failure;
    }

    auto result = runPass(
        formatManager,
        tracks,
        options,
        extraGain,
        writer.get(),
        progress,
        writeStart,
        writeSpan
    );

    // Close the writer so the file is complete before it is moved.
    writer.reset();

    if (result.cancelled || result.status.failed())
        return result;

    if (! temporary.overwriteTargetFileWithTemporary())
    {
        result.status = juce::Result::fail(
            "Impossible d'ecrire le fichier : "
            + options.outputFile.getFullPathName()
        );
        return result;
    }

    return result;
}
