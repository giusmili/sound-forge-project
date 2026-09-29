#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "AudioEngine.h"
#include "TimelineRulerComponent.h"
#include "TrackRowComponent.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void paint(juce::Graphics& graphics) override;
    void resized() override;
    void requestApplicationQuit(
        std::function<void()> quitCallback
    );

private:
    struct ProjectSnapshot
    {
        std::vector<AudioEngine::TrackState> tracks;
        double positionSeconds = 0.0;
        double bpm = 120.0;
        double masterGain = 0.8;
        double zoom = 1.0;
        double view = 0.0;
        int gridId = 1;
        bool snapEnabled = true;
    };

    void openAudioFiles();
    void openProject();
    void openRecentProject(const juce::File& file);
    void saveProject();
    void saveProjectWithCompletion(
        std::function<void(bool)> completion
    );
    void recoverAutosave();
    void confirmSaveBeforeAction(
        std::function<void()> continuation
    );
    bool writeProjectFile(const juce::File& file);
    bool loadProjectFile(
        const juce::File& file,
        bool setAsCurrentProject = true
    );
    bool writeSnapshotFile(
        const juce::File& file,
        const ProjectSnapshot& snapshot
    );
    void performAutosave();
    void updateRecoveryButton();
    [[nodiscard]] juce::File getAutosaveFile() const;
    [[nodiscard]] juce::File getBackupFile(
        const juce::File& projectFile
    ) const;
    [[nodiscard]] juce::var serialiseSnapshot(
        const ProjectSnapshot& snapshot,
        const juce::File& projectFile
    ) const;
    juce::Result deserialiseSnapshot(
        const juce::var& data,
        const juce::File& projectFile,
        ProjectSnapshot& snapshot,
        juce::StringArray& missingFiles
    ) const;
    void updateProjectTitle();
    [[nodiscard]] bool hasUnsavedChanges() const;
    [[nodiscard]] juce::String makeSnapshotFingerprint(
        const ProjectSnapshot& snapshot
    ) const;
    void addRecentProject(const juce::File& file);
    void loadRecentProjects();
    void saveRecentProjects() const;
    void refreshRecentProjectsCombo();
    [[nodiscard]] juce::File getRecentProjectsFile() const;

    TrackRowComponent* addTrackRow(AudioTrack& track);
    void rebuildTrackRowsFromEngine();
    void layoutTracks();
    void updateTimeline(double position);
    void updateProjectState();
    void seekTo(double seconds);
    void clipMoved();
    void selectRow(TrackRowComponent* row);
    void duplicateSelectedClip();
    void splitSelectedClip();
    void deleteSelectedClip();
    void toggleRecording();
    void startAudioRecording();
    void finishAudioRecording();
    [[nodiscard]] juce::File getRecordingDirectory() const;

    [[nodiscard]] ProjectSnapshot captureSnapshot() const;
    void pushUndoSnapshot(const ProjectSnapshot& snapshot);
    bool restoreSnapshot(const ProjectSnapshot& snapshot);
    void undo();
    void redo();
    void updateHistoryButtons();

    void updateGridSettings();
    void showAudioSettings();
    void timerCallback() override;

    [[nodiscard]] double getViewDuration(double projectLength) const;
    [[nodiscard]] double getViewStart(double projectLength, double viewDuration) const;
    [[nodiscard]] double getGridIntervalSeconds() const;
    [[nodiscard]] bool canSplitSelectedClip() const;

    static juce::String formatTime(double seconds);

    AudioEngine audioEngine;

    juce::Label titleLabel;
    juce::Label projectLabel;
    juce::Label timeLabel;
    juce::Label masterLabel;
    juce::Label bpmLabel;
    juce::Label gridLabel;
    juce::Label zoomLabel;
    juce::Label viewLabel;

    juce::TextButton openProjectButton { "Ouvrir projet" };
    juce::TextButton saveProjectButton { "Enregistrer" };
    juce::TextButton recoverButton { "Recuperer" };
    juce::TextButton openButton { "Importer pistes" };
    juce::ComboBox recentProjectsCombo;
    juce::TextButton undoButton { "Annuler" };
    juce::TextButton redoButton { "Retablir" };
    juce::TextButton duplicateButton { "Dupliquer" };
    juce::TextButton splitButton { "Couper" };
    juce::TextButton deleteButton { "Supprimer" };
    juce::TextButton playButton { "Play" };
    juce::TextButton pauseButton { "Pause" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton recordButton { "Record" };
    juce::TextButton audioSettingsButton { "Audio" };
    juce::TextButton snapButton { "Snap Grid" };

    juce::ComboBox gridCombo;

    juce::Slider masterSlider;
    juce::Slider bpmSlider;
    juce::Slider zoomSlider;
    juce::Slider viewSlider;

    TimelineRulerComponent timelineRuler;
    juce::Component trackList;
    juce::Viewport trackViewport;
    juce::OwnedArray<TrackRowComponent> trackRows;
    TrackRowComponent* selectedRow = nullptr;

    std::vector<ProjectSnapshot> undoStack;
    std::vector<ProjectSnapshot> redoStack;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileChooser> projectFileChooser;
    juce::File currentProjectFile;
    juce::StringArray recentProjectPaths;

    juce::File activeRecordingFile;
    double activeRecordingStart = 0.0;
    std::unique_ptr<ProjectSnapshot> recordingUndoSnapshot;

    double lastAutosaveCheckMs = 0.0;
    juce::String lastAutosaveSerialisedState;
    juce::String lastSavedFingerprint;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
