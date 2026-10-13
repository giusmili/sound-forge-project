#include "../Source/MainComponent.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

class MidiLifetimeTests
{
public:
    static void run()
    {
        juce::TemporaryFile wav(".wav");
        MainComponent app;
        // Render deterministically without sending test tones to the user's speakers.
        app.audioEngine.getDeviceManager().closeAudioDevice();
        app.audioEngine.prepareToPlay(256, 48000.0);
        juce::AudioBuffer<float> buffer(2, 256);
        MidiInstrumentTrack idle("Idle");
        idle.prepareToPlay(256, 48000.0);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                buffer.setSample(channel, sample, 1.0f);
        idle.getNextAudioBlock({ &buffer, 0, buffer.getNumSamples() });
        require(buffer.getMagnitude(0, buffer.getNumSamples()) < 0.00001f,
                "Idle MIDI source reused previous audio samples");
        for (int iteration = 0; iteration < 50; ++iteration)
        {
            app.addMidiInstrumentTrack();
            require(app.midiTrackRows.size() == 1, "MIDI row missing");
            auto* track = &app.midiTrackRows[0]->getTrack();
            track->setGain(0.37f);
            track->noteOn(60, 0.8f);
            buffer.clear();
            app.audioEngine.getNextAudioBlock({ &buffer, 0, buffer.getNumSamples() });
            require(buffer.getMagnitude(0, buffer.getNumSamples()) > 0.001f,
                    "Synth did not produce samples");
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    require(std::isfinite(buffer.getSample(channel, sample)), "Non-finite synth output");
            track->setMuted(true);
            buffer.clear();
            app.audioEngine.getNextAudioBlock({ &buffer, 0, buffer.getNumSamples() });
            require(buffer.getMagnitude(0, buffer.getNumSamples()) < 0.00001f,
                    "Muted synth produced output");
            track->setMuted(false);
            app.undo();
            require(app.midiTrackRows.isEmpty(), "Undo did not remove MIDI row");
            require(app.audioEngine.getMidiTrackPointers().empty(), "Undo did not remove track");
            app.redo();
            require(app.midiTrackRows.size() == 1, "Redo did not restore MIDI row");
            require(std::abs(app.midiTrackRows[0]->getTrack().getGain() - 0.37f) < 0.001f,
                    "Redo lost volume");
            app.midiTrackRows[0]->getTrack().noteOn(60, 0.8f);
            app.deleteMidiTrackRow(app.midiTrackRows[0]);
            require(app.midiTrackRows.isEmpty(), "Delete did not remove MIDI row");
            app.undo();
            require(app.midiTrackRows.size() == 1, "Undo deletion did not restore MIDI row");
            app.redo();
            require(app.midiTrackRows.isEmpty(), "Redo deletion did not remove MIDI row");
        }
        // Import and render a real WAV, then check the transport and mute.
        juce::AudioBuffer<float> tone(1, 4800);
        for (int sample = 0; sample < tone.getNumSamples(); ++sample)
            tone.setSample(0, sample, 0.2f * static_cast<float>(
                std::sin(juce::MathConstants<double>::twoPi * 440.0 * sample / 48000.0)));
        auto stream = wav.getFile().createOutputStream();
        require(stream != nullptr, "Cannot create WAV fixture");
        juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatWriter> writer(
            format.createWriterFor(stream.get(), 48000.0, 1, 16, {}, 0));
        require(writer != nullptr, "Cannot initialise WAV writer");
        stream.release();
        require(writer->writeFromAudioSampleBuffer(tone, 0, tone.getNumSamples()), "Cannot write WAV");
        writer.reset();
        AudioTrack* audio = nullptr;
        require(app.audioEngine.addTrackFromFile(wav.getFile(), audio).wasOk(), "WAV import failed");
        require(audio != nullptr, "WAV import created no track");
        app.addTrackRow(*audio);
        require(std::abs(app.audioEngine.getLengthSeconds() - 0.1) < 0.001, "WAV duration incorrect");
        app.audioEngine.play();
        buffer.clear();
        app.audioEngine.getNextAudioBlock({ &buffer, 0, buffer.getNumSamples() });
        require(buffer.getMagnitude(0, buffer.getNumSamples()) > 0.001f, "WAV playback silent");
        app.audioEngine.pause();
        require(!app.audioEngine.isPlaying(), "Pause did not stop playback");
        app.audioEngine.stop();
        require(app.audioEngine.getPositionSeconds() == 0.0, "Stop did not rewind");
        audio->setMuted(true);
        app.audioEngine.play();
        buffer.clear();
        app.audioEngine.getNextAudioBlock({ &buffer, 0, buffer.getNumSamples() });
        require(buffer.getMagnitude(0, buffer.getNumSamples()) < 0.00001f, "Muted WAV produced output");
        app.audioEngine.stop();
        // A missing source must leave existing audio and MIDI usable after UI reconstruction.
        app.addMidiInstrumentTrack();
        app.midiTrackRows[0]->getTrack().setGain(0.42f);
        app.midiTrackRows[0]->getTrack().setMuted(true);
        juce::TemporaryFile project(".sonoforge");
        require(app.writeSnapshotFile(project.getFile(), app.captureSnapshot()), "Project save failed");
        app.midiTrackRows[0]->getTrack().setGain(0.9f);
        require(app.loadProjectFile(project.getFile(), false), "Project reload failed");
        require(std::abs(app.midiTrackRows[0]->getTrack().getGain() - 0.42f) < 0.001f,
                "Project reload lost volume");
        require(app.midiTrackRows[0]->getTrack().isMuted(), "Project reload lost mute");
        require(app.trackRows.size() == 1, "Project reload lost WAV track");
        auto invalid = app.captureSnapshot();
        AudioEngine::TrackState missing;
        missing.sourceFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile(juce::Uuid().toString() + ".wav");
        invalid.tracks.push_back(missing);
        require(!app.restoreSnapshot(invalid), "Missing audio source unexpectedly restored");
        require(app.midiTrackRows.size() == 1, "Failed restore lost existing MIDI row");
        app.midiTrackRows[0]->getTrack().noteOn(60, 0.8f);
        app.deleteMidiTrackRow(app.midiTrackRows[0]);
        juce::ModalComponentManager::getInstance()->cancelAllModalComponents();
        std::cout << "PASS: 50 MIDI undo/redo/delete cycles, synth rendering, WAV import/transport/mute, "
                     "mixed project save/reload and failed restoration\n";
    }
private:
    static void require(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }
};

int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;
    try
    {
        MidiLifetimeTests::run();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
