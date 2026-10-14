#include "MainComponent.h"
#include "AppServices.h"

#include <cmath>

namespace
{
constexpr auto backgroundColour = 0xff111318;
constexpr auto panelColour = 0xff1b1f26;
constexpr auto accentColour = 0xff5aa9ff;
constexpr auto textColour = 0xffe8edf3;
constexpr auto snapColour = 0xff3d8f68;
constexpr int trackRowHeight = 128;
constexpr int scrollBarWidth = 14;
constexpr double minimumSplitMarginSeconds = 0.01;
constexpr auto maxHistoryEntries = 50U;
constexpr double autosaveIntervalMs = 30000.0;

// Project files are written with the short extension. The long one is still
// accepted when opening, and old autosave files are migrated.
constexpr auto projectExtension = ".sfs";
constexpr auto legacyProjectExtension = ".sonoforge";

constexpr int menuOpenProject = 1001;
constexpr int menuSaveProject = 1002;
constexpr int menuSaveProjectAs = 1005;
constexpr int menuImportAudio = 1003;
constexpr int menuExportMix = 1004;
constexpr int menuQuit = 1009;

constexpr int menuUndo = 2001;
constexpr int menuRedo = 2002;

constexpr int menuAddAudioTrack = 3001;
constexpr int menuAddMidiTrack = 3002;
constexpr int menuAudioSettings = 3003;
}

MainComponent::MainComponent()
{
    setOpaque(true);
    setSize(1320, 860);
    setWantsKeyboardFocus(true);

    addAndMakeVisible(menuBar);

    titleLabel.setText(
        "SonoForge Studio 0.9.1",
        juce::dontSendNotification
    );
    titleLabel.setFont(
        juce::FontOptions(24.0f, juce::Font::bold)
    );
    titleLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour)
    );
    addAndMakeVisible(titleLabel);

    projectLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour).withAlpha(0.75f)
    );
    addAndMakeVisible(projectLabel);

    timeLabel.setText(
        "00:00.000 / 00:00.000",
        juce::dontSendNotification
    );
    timeLabel.setJustificationType(
        juce::Justification::centred
    );
    timeLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour)
    );
    addAndMakeVisible(timeLabel);

    for (auto* label : {
             &masterLabel,
             &bpmLabel,
             &gridLabel,
             &zoomLabel,
             &viewLabel
         })
    {
        label->setColour(
            juce::Label::textColourId,
            juce::Colour(textColour)
        );
        addAndMakeVisible(*label);
    }

    masterLabel.setText("Master", juce::dontSendNotification);
    bpmLabel.setText("BPM", juce::dontSendNotification);
    gridLabel.setText("Grille", juce::dontSendNotification);
    zoomLabel.setText("Zoom", juce::dontSendNotification);
    viewLabel.setText("Vue", juce::dontSendNotification);

    for (auto* button : {
             &openProjectButton,
             &saveProjectButton,
             &recoverButton,
             &openButton,
             &midiTrackButton,
             &quitButton,
             &undoButton,
             &redoButton,
             &duplicateButton,
             &splitButton,
             &deleteButton,
             &playButton,
             &pauseButton,
             &stopButton,
             &recordButton,
             &audioSettingsButton,
             &mixerButton,
             &exportButton,
             &snapButton
         })
    {
        // Keep the keyboard free for shortcuts (Space = play/pause):
        // a focused button would otherwise swallow the space bar.
        button->setWantsKeyboardFocus(false);
        button->setColour(
            juce::TextButton::buttonColourId,
            juce::Colour(panelColour)
        );
        button->setColour(
            juce::TextButton::textColourOffId,
            juce::Colour(textColour)
        );
        addAndMakeVisible(*button);
    }

    snapButton.setClickingTogglesState(true);
    snapButton.setToggleState(
        true,
        juce::dontSendNotification
    );
    snapButton.setColour(
        juce::TextButton::buttonOnColourId,
        juce::Colour(snapColour)
    );

    gridCombo.addItem("1/4", 1);
    gridCombo.addItem("1/8", 2);
    gridCombo.addItem("1/16", 3);
    gridCombo.setSelectedId(
        1,
        juce::dontSendNotification
    );
    gridCombo.setWantsKeyboardFocus(false);
    addAndMakeVisible(gridCombo);

    recentProjectsCombo.setTextWhenNothingSelected(
        "Projets recents"
    );
    recentProjectsCombo.onChange = [this]
    {
        const auto selectedId =
            recentProjectsCombo.getSelectedId();

        if (selectedId <= 0)
            return;

        const auto index = selectedId - 1;

        recentProjectsCombo.setSelectedId(
            0,
            juce::dontSendNotification
        );

        if (juce::isPositiveAndBelow(
                index,
                recentProjectPaths.size()
            ))
        {
            openRecentProject(
                juce::File(recentProjectPaths[index])
            );
        }
    };
    recentProjectsCombo.setWantsKeyboardFocus(false);
    addAndMakeVisible(recentProjectsCombo);

    masterSlider.setRange(0.0, 1.0, 0.01);
    masterSlider.setValue(0.8);
    masterSlider.setSliderStyle(
        juce::Slider::LinearHorizontal
    );
    masterSlider.setTextBoxStyle(
        juce::Slider::TextBoxRight,
        false,
        64,
        24
    );
    masterSlider.setColour(
        juce::Slider::trackColourId,
        juce::Colour(accentColour)
    );
    addAndMakeVisible(masterSlider);

    bpmSlider.setRange(40.0, 240.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setSliderStyle(
        juce::Slider::LinearHorizontal
    );
    bpmSlider.setTextBoxStyle(
        juce::Slider::TextBoxRight,
        false,
        50,
        22
    );
    addAndMakeVisible(bpmSlider);

    zoomSlider.setRange(1.0, 8.0, 0.25);
    zoomSlider.setValue(1.0);
    zoomSlider.setSliderStyle(
        juce::Slider::LinearHorizontal
    );
    zoomSlider.setTextBoxStyle(
        juce::Slider::TextBoxRight,
        false,
        52,
        22
    );
    addAndMakeVisible(zoomSlider);

    viewSlider.setRange(0.0, 1.0, 0.001);
    viewSlider.setValue(0.0);
    viewSlider.setSliderStyle(
        juce::Slider::LinearHorizontal
    );
    viewSlider.setTextBoxStyle(
        juce::Slider::NoTextBox,
        false,
        0,
        0
    );
    addAndMakeVisible(viewSlider);

    timelineRuler.onSeek = [this](const double seconds)
    {
        seekTo(seconds);
    };
    addAndMakeVisible(timelineRuler);

    trackViewport.setViewedComponent(&trackList, false);
    trackViewport.setScrollBarsShown(true, false);
    trackViewport.setScrollBarThickness(scrollBarWidth);
    addAndMakeVisible(trackViewport);

    mixerButton.setClickingTogglesState(true);
    mixerButton.setColour(
        juce::TextButton::buttonOnColourId,
        juce::Colour(accentColour)
    );

    exportButton.setTooltip("Exporter le mixage en WAV ou FLAC");
    exportButton.onClick = [this]
    {
        exportMix();
    };

    mixerPanel.onChanged = [this]
    {
        masterSlider.setValue(
            audioEngine.getMasterGain(),
            juce::dontSendNotification
        );

        for (auto* row : trackRows)
            row->syncControlsFromTrack();

        for (auto* row : midiTrackRows)
            row->syncControlsFromTrack();

        updateProjectTitle();
    };

    mixerPanel.setVisible(false);
    addAndMakeVisible(mixerPanel);

    openProjectButton.onClick = [this]
    {
        openProject();
    };

    saveProjectButton.onClick = [this]
    {
        saveProject();
    };

    recoverButton.onClick = [this]
    {
        recoverAutosave();
    };

    openButton.onClick = [this]
    {
        openAudioFiles();
    };

    midiTrackButton.onClick = [this]
    {
        addMidiInstrumentTrack();
    };

    quitButton.onClick = [this]
    {
        requestApplicationQuit(
            []
            {
                if (auto* application =
                        juce::JUCEApplication::getInstance())
                {
                    application->quit();
                }
            }
        );
    };

    undoButton.onClick = [this]
    {
        undo();
    };

    redoButton.onClick = [this]
    {
        redo();
    };

    duplicateButton.onClick = [this]
    {
        duplicateSelectedClip();
    };

    splitButton.onClick = [this]
    {
        splitSelectedClip();
    };

    deleteButton.onClick = [this]
    {
        deleteSelectedClip();
    };

    playButton.onClick = [this]
    {
        audioEngine.play();
        pauseButton.setEnabled(audioEngine.isPlaying());
    };

    pauseButton.onClick = [this]
    {
        audioEngine.pause();
        pauseButton.setEnabled(false);
    };

    stopButton.onClick = [this]
    {
        if (audioEngine.isRecording())
            finishAudioRecording();

        audioEngine.stop();
        updateTimeline(0.0);
        pauseButton.setEnabled(false);
    };

    recordButton.onClick = [this]
    {
        toggleRecording();
    };

    audioSettingsButton.onClick = [this]
    {
        showAudioSettings();
    };

    mixerButton.onClick = [this]
    {
        mixerVisible =
            mixerButton.getToggleState();

        mixerPanel.setVisible(mixerVisible);

        if (mixerVisible)
            mixerPanel.syncWithEngine();

        resized();
    };

    snapButton.onClick = [this]
    {
        updateGridSettings();
        updateProjectTitle();
    };

    gridCombo.onChange = [this]
    {
        updateGridSettings();
        updateTimeline(
            audioEngine.getPositionSeconds()
        );
        updateProjectTitle();
    };

    masterSlider.onValueChange = [this]
    {
        audioEngine.setMasterGain(
            static_cast<float>(masterSlider.getValue())
        );
        updateProjectTitle();
    };

    bpmSlider.onValueChange = [this]
    {
        updateGridSettings();
        updateTimeline(
            audioEngine.getPositionSeconds()
        );
        updateProjectTitle();
    };

    zoomSlider.onValueChange = [this]
    {
        updateTimeline(
            audioEngine.getPositionSeconds()
        );
    };

    viewSlider.onValueChange = [this]
    {
        updateTimeline(
            audioEngine.getPositionSeconds()
        );
    };

    audioEngine.setMasterGain(
        static_cast<float>(masterSlider.getValue())
    );

    startTimerHz(30);
    lastAutosaveCheckMs =
        juce::Time::getMillisecondCounterHiRes();

    loadRecentProjects();
    lastSavedFingerprint =
        makeSnapshotFingerprint(captureSnapshot());

    updateProjectTitle();
    updateProjectState();
    updateRecoveryButton();
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(backgroundColour));

    auto panel = getLocalBounds()
                     .reduced(20)
                     .withTrimmedTop(106);

    graphics.setColour(juce::Colour(panelColour));
    graphics.fillRoundedRectangle(
        panel.toFloat(),
        8.0f
    );
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(20);

    menuBar.setBounds(
        area.removeFromTop(26)
    );

    area.removeFromTop(4);

    auto header = area.removeFromTop(52);
    quitButton.setBounds(
        header.removeFromRight(82).reduced(4)
    );
    audioSettingsButton.setBounds(
        header.removeFromRight(100).reduced(4)
    );
    recentProjectsCombo.setBounds(
        header.removeFromRight(240).reduced(4)
    );
    titleLabel.setBounds(header);

    area.removeFromTop(24);
    area.reduce(18, 18);

    auto importRow = area.removeFromTop(44);
    openProjectButton.setBounds(
        importRow.removeFromLeft(120).reduced(4)
    );
    saveProjectButton.setBounds(
        importRow.removeFromLeft(112).reduced(4)
    );
    recoverButton.setBounds(
        importRow.removeFromLeft(100).reduced(4)
    );
    openButton.setBounds(
        importRow.removeFromLeft(92).reduced(4)
    );
    midiTrackButton.setBounds(
        importRow.removeFromLeft(92).reduced(4)
    );
    importRow.removeFromLeft(8);
    projectLabel.setBounds(
        importRow.removeFromLeft(110)
    );
    importRow.removeFromLeft(8);
    undoButton.setBounds(
        importRow.removeFromLeft(86).reduced(4)
    );
    redoButton.setBounds(
        importRow.removeFromLeft(86).reduced(4)
    );
    importRow.removeFromLeft(8);
    duplicateButton.setBounds(
        importRow.removeFromLeft(110).reduced(4)
    );
    splitButton.setBounds(
        importRow.removeFromLeft(90).reduced(4)
    );
    deleteButton.setBounds(
        importRow.removeFromLeft(105).reduced(4)
    );

    area.removeFromTop(10);

    auto transportRow = area.removeFromTop(54);
    playButton.setBounds(
        transportRow.removeFromLeft(76).reduced(5)
    );
    pauseButton.setBounds(
        transportRow.removeFromLeft(76).reduced(5)
    );
    stopButton.setBounds(
        transportRow.removeFromLeft(76).reduced(5)
    );
    recordButton.setBounds(
        transportRow.removeFromLeft(90).reduced(5)
    );
    transportRow.removeFromLeft(12);
    timeLabel.setBounds(
        transportRow.removeFromLeft(260).reduced(5)
    );
    transportRow.removeFromLeft(12);
    masterLabel.setBounds(
        transportRow.removeFromLeft(58)
    );
    masterSlider.setBounds(
        transportRow.removeFromLeft(250).reduced(4)
    );

    area.removeFromTop(6);

    auto viewRow = area.removeFromTop(40);

    snapButton.setBounds(
        viewRow.removeFromLeft(104).reduced(3)
    );

    viewRow.removeFromLeft(8);

    gridLabel.setBounds(
        viewRow.removeFromLeft(42)
    );
    gridCombo.setBounds(
        viewRow.removeFromLeft(72).reduced(3)
    );

    viewRow.removeFromLeft(8);

    bpmLabel.setBounds(
        viewRow.removeFromLeft(38)
    );
    bpmSlider.setBounds(
        viewRow.removeFromLeft(130).reduced(3)
    );

    viewRow.removeFromLeft(10);

    zoomLabel.setBounds(
        viewRow.removeFromLeft(46)
    );
    zoomSlider.setBounds(
        viewRow.removeFromLeft(150).reduced(3)
    );

    viewRow.removeFromLeft(10);

    viewLabel.setBounds(
        viewRow.removeFromLeft(38)
    );
    viewSlider.setBounds(
        viewRow.removeFromLeft(240).reduced(3)
    );

    mixerButton.setBounds(
        viewRow.removeFromRight(96).reduced(3)
    );
    exportButton.setBounds(
        viewRow.removeFromRight(96).reduced(3)
    );

    area.removeFromTop(6);

    timelineRuler.setBounds(
        area.removeFromTop(40).withTrimmedRight(scrollBarWidth)
    );

    area.removeFromTop(2);

    if (mixerVisible)
    {
        auto mixerArea =
            area.removeFromBottom(
                juce::jmin(270, area.getHeight() / 2)
            );

        area.removeFromBottom(6);
        mixerPanel.setBounds(mixerArea);
    }

    trackViewport.setBounds(area);

    layoutTracks();
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return {
        "Fichier",
        "Edition",
        "Piste"
    };
}

juce::PopupMenu MainComponent::getMenuForIndex(
    const int topLevelMenuIndex,
    const juce::String&
)
{
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 0)
    {
        menu.addItem(
            menuOpenProject,
            "Ouvrir projet...",
            ! audioEngine.isRecording()
        );
        menu.addItem(
            menuSaveProject,
            "Enregistrer",
            true
        );
        menu.addItem(
            menuSaveProjectAs,
            "Enregistrer sous...",
            true
        );
        menu.addSeparator();
        menu.addItem(
            menuImportAudio,
            "Importer audio...",
            ! audioEngine.isRecording()
        );
        menu.addSeparator();
        menu.addItem(
            menuExportMix,
            "Exporter le mixage...",
            ! audioEngine.isRecording()
        );
        menu.addSeparator();
        menu.addItem(
            menuQuit,
            "Quitter"
        );
    }
    else if (topLevelMenuIndex == 1)
    {
        menu.addItem(
            menuUndo,
            "Annuler",
            ! undoStack.empty()
        );
        menu.addItem(
            menuRedo,
            "Retablir",
            ! redoStack.empty()
        );
    }
    else if (topLevelMenuIndex == 2)
    {
        menu.addItem(
            menuAddAudioTrack,
            "Ajouter piste audio...",
            ! audioEngine.isRecording()
        );
        menu.addItem(
            menuAddMidiTrack,
            "Ajouter piste MIDI / Instrument",
            ! audioEngine.isRecording()
        );
        menu.addSeparator();
        menu.addItem(
            menuAudioSettings,
            "Configuration audio...",
            ! audioEngine.isRecording()
        );
    }

    return menu;
}

void MainComponent::menuItemSelected(
    const int menuItemID,
    const int
)
{
    switch (menuItemID)
    {
        case menuOpenProject:
            openProject();
            break;

        case menuSaveProject:
            saveProject();
            break;

        case menuSaveProjectAs:
            saveProjectAs();
            break;

        case menuImportAudio:
        case menuAddAudioTrack:
            openAudioFiles();
            break;

        case menuExportMix:
            exportMix();
            break;

        case menuUndo:
            undo();
            break;

        case menuRedo:
            redo();
            break;

        case menuAddMidiTrack:
            addMidiInstrumentTrack();
            break;

        case menuAudioSettings:
            showAudioSettings();
            break;

        case menuQuit:
            requestApplicationQuit(
                []
                {
                    if (auto* application =
                            juce::JUCEApplication::getInstance())
                    {
                        application->quit();
                    }
                }
            );
            break;

        default:
            break;
    }
}

void MainComponent::requestApplicationQuit(
    std::function<void()> quitCallback
)
{
    AppServices::log("Quit requested (unsaved changes: "
                     + juce::String(hasUnsavedChanges() ? "yes" : "no") + ")");
    confirmSaveBeforeAction(
        std::move(quitCallback)
    );
}

void MainComponent::importAudioFiles(const juce::Array<juce::File>& files)
{
    AppServices::log(
        "Import: " + juce::String(files.size()) + " file(s) selected"
    );

    const auto before =
        captureSnapshot();

    bool projectChanged = false;
    TrackRowComponent* lastAdded = nullptr;

    for (const auto& file : files)
    {
        if (! file.existsAsFile())
            continue;

        AudioTrack* createdTrack = nullptr;
        AppServices::log("Import: loading " + file.getFullPathName());

        const auto result =
            audioEngine.addTrackFromFile(
                file,
                createdTrack
            );
        AppServices::log(
            result.wasOk()
                ? "Import: engine OK, creating track row"
                : "Import: engine failed - " + result.getErrorMessage()
        );

        if (result.wasOk()
            && createdTrack != nullptr)
        {
            lastAdded =
                addTrackRow(*createdTrack);
            AppServices::log("Import: track row created");
            projectChanged = true;
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "Import audio impossible",
                file.getFileName()
                    + "\n\n"
                    + result.getErrorMessage()
            );
        }
    }

    if (lastAdded != nullptr)
        selectRow(lastAdded);

    if (projectChanged)
        pushUndoSnapshot(before);

    updateProjectState();
}

void MainComponent::openAudioFiles()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choisir une ou plusieurs pistes audio",
        juce::File {},
        "*.wav;*.mp3;*.aiff;*.aif;*.flac;*.ogg"
    );

    auto safeThis =
        juce::Component::SafePointer<MainComponent>(this);

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::canSelectMultipleItems,
        [safeThis](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr)
                return;

            safeThis->importAudioFiles(chooser.getResults());
        }
    );
}

namespace
{
// Runs the offline mixdown on a worker thread behind a progress window with
// a Cancel button. Deletes itself once the result has been delivered.
class MixdownJob final : public juce::ThreadWithProgressWindow
{
public:
    MixdownJob(
        juce::AudioFormatManager& manager,
        std::vector<AudioEngine::TrackState> trackStates,
        MixdownOptions exportOptions,
        std::function<void(const MixdownResult&)> onFinished
    )
        : juce::ThreadWithProgressWindow(
              "Export du mixage en cours...",
              true,
              true,
              5000,
              "Annuler"
          ),
          formatManager(manager),
          tracks(std::move(trackStates)),
          options(std::move(exportOptions)),
          finished(std::move(onFinished))
    {
    }

    void run() override
    {
        result = MixdownRenderer::render(
            formatManager,
            tracks,
            options,
            [this](const double fraction)
            {
                setProgress(fraction);
                return ! threadShouldExit();
            }
        );
    }

    void threadComplete(const bool) override
    {
        if (finished)
            finished(result);

        // This runs inside the window's own timer callback: delete later.
        juce::MessageManager::callAsync(
            [job = this]
            {
                delete job;
            }
        );
    }

private:
    juce::AudioFormatManager& formatManager;
    std::vector<AudioEngine::TrackState> tracks;
    MixdownOptions options;
    std::function<void(const MixdownResult&)> finished;
    MixdownResult result;
};

void showExportResult(
    const MixdownOptions& options,
    const MixdownResult& result
)
{
    if (result.cancelled)
    {
        AppServices::log("Export cancelled");
        return;
    }

    if (result.status.failed())
    {
        AppServices::log("Export failed: " + result.status.getErrorMessage());
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Export impossible",
            result.status.getErrorMessage()
        );
        return;
    }

    const auto level = result.peak > 0.00001f
        ? juce::String(juce::Decibels::gainToDecibels(result.peak), 1) + " dBFS"
        : juce::String("silence");

    juce::String message;
    message << options.outputFile.getFileName() << "\n\n"
            << "Format : "
            << MixdownRenderer::getFormatDescription(options.format)
            << ", " << juce::String(juce::roundToInt(options.sampleRate))
            << " Hz, stereo\n"
            << "Duree : "
            << juce::String(result.durationSeconds, 1) << " s\n"
            << "Niveau crete : " << level;

    if (result.clippedSamples > 0)
    {
        message << "\n\nAttention : "
                << juce::String(result.clippedSamples)
                << " echantillons depassaient 0 dBFS et ont ete ecretes. "
                   "Baissez le volume Master ou activez la normalisation.";
    }

    AppServices::log("Export done: " + options.outputFile.getFullPathName());

    const auto file = options.outputFile;

    juce::AlertWindow::showAsync(
        juce::MessageBoxOptions()
            .withIconType(juce::MessageBoxIconType::InfoIcon)
            .withTitle("Export termine")
            .withMessage(message)
            .withButton("Afficher le fichier")
            .withButton("Fermer"),
        [file](const int choice)
        {
            if (choice == 1)
                file.revealToUser();
        }
    );
}
} // namespace

juce::File MainComponent::getExportDirectory() const
{
    if (currentProjectFile.getFullPathName().isNotEmpty())
    {
        return currentProjectFile
            .getParentDirectory()
            .getChildFile("Exports");
    }

    return juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory
    )
        .getChildFile("SonoForge Exports");
}

void MainComponent::exportMix()
{
    if (audioEngine.isRecording())
        return;

    audioEngine.pause();

    const auto tracks = audioEngine.captureTrackStates();

    MixdownOptions probe;
    probe.masterGain = audioEngine.getMasterGain();

    const auto length =
        MixdownRenderer::calculateLengthSeconds(tracks, probe);

    if (length <= 0.0)
    {
        juce::String reason =
            "Rien a exporter : le mixage ne contient aucune piste audio audible.";

        if (! audioEngine.getMidiTrackPointers().empty())
        {
            reason += "\nLes pistes MIDI / Instrument ne sont pas encore "
                      "exportees : elles n'ont pas de clips.";
        }

        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "Exporter le mixage",
            reason
        );
        return;
    }

    auto* window = new juce::AlertWindow(
        "Exporter le mixage",
        "Duree du mixage : " + formatTime(length)
            + "\nLe mixage inclut les effets, le volume, le pan, "
              "Mute/Solo et le Master.",
        juce::MessageBoxIconType::QuestionIcon
    );

    window->addComboBox(
        "format",
        { "WAV 24 bits", "WAV 16 bits", "WAV 32 bits flottant",
          "FLAC 24 bits", "FLAC 16 bits" },
        "Format"
    );
    window->addComboBox(
        "rate",
        { "44100 Hz", "48000 Hz", "96000 Hz" },
        "Taux d'echantillonnage"
    );
    window->addComboBox(
        "normalise",
        { "Non", "Oui, a -1 dBFS" },
        "Normaliser"
    );

    window->getComboBoxComponent("format")->setSelectedItemIndex(0);

    auto deviceRate = 48000.0;

    if (auto* device = audioEngine.getDeviceManager().getCurrentAudioDevice())
        deviceRate = device->getCurrentSampleRate();

    window->getComboBoxComponent("rate")->setSelectedItemIndex(
        deviceRate > 70000.0 ? 2 : (deviceRate < 46000.0 ? 0 : 1)
    );
    window->getComboBoxComponent("normalise")->setSelectedItemIndex(0);

    window->addButton(
        "Exporter",
        1,
        juce::KeyPress(juce::KeyPress::returnKey)
    );
    window->addButton(
        "Annuler",
        0,
        juce::KeyPress(juce::KeyPress::escapeKey)
    );

    auto safeThis =
        juce::Component::SafePointer<MainComponent>(this);
    const auto masterGain = audioEngine.getMasterGain();

    window->enterModalState(
        true,
        juce::ModalCallbackFunction::create(
            [safeThis, window, tracks, masterGain](const int result)
            {
                if (result != 1 || safeThis == nullptr)
                    return;

                static constexpr MixdownOptions::Format formats[] = {
                    MixdownOptions::Format::wav24,
                    MixdownOptions::Format::wav16,
                    MixdownOptions::Format::wav32Float,
                    MixdownOptions::Format::flac24,
                    MixdownOptions::Format::flac16
                };
                static constexpr double rates[] = { 44100.0, 48000.0, 96000.0 };

                // The window is still alive here: it is deleted after this callback.
                const auto formatIndex = juce::jlimit(
                    0,
                    4,
                    window->getComboBoxComponent("format")->getSelectedItemIndex()
                );
                const auto rateIndex = juce::jlimit(
                    0,
                    2,
                    window->getComboBoxComponent("rate")->getSelectedItemIndex()
                );

                MixdownOptions options;
                options.format = formats[formatIndex];
                options.sampleRate = rates[rateIndex];
                options.normalise =
                    window->getComboBoxComponent("normalise")->getSelectedItemIndex() == 1;
                options.masterGain = masterGain;

                // Let the options window finish closing, and focus return to
                // the main window, before the native save dialog appears.
                juce::MessageManager::callAsync(
                    [safeThis, tracks, options]
                    {
                        if (safeThis != nullptr)
                            safeThis->chooseExportFile(tracks, options);
                    }
                );
            }
        ),
        true
    );
}

void MainComponent::chooseExportFile(
    std::vector<AudioEngine::TrackState> tracks,
    MixdownOptions options
)
{
    const auto baseName =
        currentProjectFile.getFullPathName().isNotEmpty()
            ? currentProjectFile.getFileNameWithoutExtension()
            : juce::String("Mixage");

    const auto extension =
        MixdownRenderer::getFileExtension(options.format);

    // Open in the Exports folder once it exists, otherwise in its parent
    // (project folder or Documents) rather than wherever Windows last was.
    auto directory = getExportDirectory();

    if (! directory.isDirectory())
        directory = directory.getParentDirectory();

    // Owned by the main window: an unowned native dialog could open behind it
    // and look like the export had frozen.
    exportFileChooser = std::make_unique<juce::FileChooser>(
        "Exporter le mixage",
        directory.getChildFile(baseName + extension),
        "*" + extension,
        true,
        false,
        getTopLevelComponent()
    );

    auto safeThis =
        juce::Component::SafePointer<MainComponent>(this);

    exportFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis, tracks, options](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr)
                return;

            const auto chosen = chooser.getResult();

            if (chosen.getFullPathName().isEmpty()
                || safeThis->audioEngine.isRecording())
            {
                return;
            }

            auto exportOptions = options;
            exportOptions.outputFile = chosen.withFileExtension(
                MixdownRenderer::getFileExtension(options.format)
            );

            AppServices::log(
                "Export: "
                + exportOptions.outputFile.getFullPathName()
                + " ("
                + MixdownRenderer::getFormatDescription(exportOptions.format)
                + ", "
                + juce::String(juce::roundToInt(exportOptions.sampleRate))
                + " Hz)"
            );

            auto* job = new MixdownJob(
                safeThis->audioEngine.getFormatManager(),
                tracks,
                exportOptions,
                [exportOptions](const MixdownResult& result)
                {
                    showExportResult(exportOptions, result);
                }
            );

            job->launchThread();
        }
    );
}

void MainComponent::openProject()
{
    projectFileChooser = std::make_unique<juce::FileChooser>(
        "Ouvrir un projet SonoForge",
        juce::File {},
        juce::String("*") + projectExtension
            + ";*" + legacyProjectExtension
    );

    auto safeThis =
        juce::Component::SafePointer<MainComponent>(this);

    projectFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr)
                return;

            const auto file = chooser.getResult();

            if (! file.existsAsFile())
                return;

            safeThis->confirmSaveBeforeAction(
                [safeThis, file]
                {
                    if (safeThis != nullptr)
                        safeThis->loadProjectFile(file);
                }
            );
        }
    );
}

void MainComponent::openRecentProject(
    const juce::File& file
)
{
    if (! file.existsAsFile())
    {
        recentProjectPaths.removeString(
            file.getFullPathName()
        );
        saveRecentProjects();
        refreshRecentProjectsCombo();

        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Projet introuvable",
            "Ce projet recent n'existe plus a cet emplacement."
        );
        return;
    }

    auto safeThis =
        juce::Component::SafePointer<MainComponent>(this);

    confirmSaveBeforeAction(
        [safeThis, file]
        {
            if (safeThis != nullptr)
                safeThis->loadProjectFile(file);
        }
    );
}

void MainComponent::saveProject()
{
    saveProjectWithCompletion({});
}

void MainComponent::saveProjectAs()
{
    saveProjectWithCompletion({}, true);
}

void MainComponent::saveProjectWithCompletion(
    std::function<void(bool)> completion,
    const bool alwaysAskForFile
)
{
    if (audioEngine.isRecording())
        finishAudioRecording();

    const auto hasProjectFile =
        currentProjectFile.getFullPathName().isNotEmpty();

    if (hasProjectFile && ! alwaysAskForFile)
    {
        const auto success =
            writeProjectFile(currentProjectFile);

        if (completion)
            completion(success);

        return;
    }

    // Suggest the current name with the short extension, next to the
    // current file, so converting an old project is a single click.
    const auto suggestion = hasProjectFile
        ? currentProjectFile
              .getSiblingFile(
                  currentProjectFile.getFileNameWithoutExtension()
                  + projectExtension)
        : juce::File::getSpecialLocation(
              juce::File::userDocumentsDirectory
          ).getChildFile(juce::String("projet") + projectExtension);

    projectFileChooser = std::make_unique<juce::FileChooser>(
        alwaysAskForFile
            ? "Enregistrer le projet SonoForge sous..."
            : "Enregistrer le projet SonoForge",
        suggestion,
        juce::String("*") + projectExtension,
        true,
        false,
        getTopLevelComponent()
    );

    auto safeThis =
        juce::Component::SafePointer<MainComponent>(this);

    projectFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis, completion](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr)
            {
                if (completion)
                    completion(false);
                return;
            }

            auto file = chooser.getResult();

            if (file.getFullPathName().isEmpty())
            {
                if (completion)
                    completion(false);
                return;
            }

            if (! file.hasFileExtension(projectExtension))
                file = file.withFileExtension(projectExtension);

            const auto success =
                safeThis->writeProjectFile(file);

            if (completion)
                completion(success);
        }
    );
}

void MainComponent::recoverAutosave()
{
    const auto autosaveFile = getAutosaveFile();

    if (! autosaveFile.existsAsFile())
    {
        updateRecoveryButton();
        return;
    }

    auto safeThis =
        juce::Component::SafePointer<MainComponent>(this);

    confirmSaveBeforeAction(
        [safeThis, autosaveFile]
        {
            if (safeThis != nullptr)
            {
                safeThis->loadProjectFile(
                    autosaveFile,
                    false
                );
            }
        }
    );
}

void MainComponent::confirmSaveBeforeAction(
    std::function<void()> continuation
)
{
    if (audioEngine.isRecording())
        finishAudioRecording();

    if (! hasUnsavedChanges())
    {
        if (continuation)
            continuation();
        return;
    }

    auto safeThis =
        juce::Component::SafePointer<MainComponent>(this);

    auto options = juce::MessageBoxOptions()
        .withIconType(
            juce::MessageBoxIconType::QuestionIcon
        )
        .withTitle(
            "Modifications non enregistrees"
        )
        .withMessage(
            "Le projet contient des modifications non enregistrees."
        )
        .withButton("Enregistrer")
        .withButton("Ne pas enregistrer")
        .withButton("Annuler");

    juce::AlertWindow::showAsync(
        options,
        [safeThis, continuation](const int result)
        {
            if (safeThis == nullptr)
                return;

            if (result == 1)
            {
                safeThis->saveProjectWithCompletion(
                    [safeThis, continuation](const bool success)
                    {
                        if (safeThis != nullptr
                            && success
                            && continuation)
                        {
                            continuation();
                        }
                    }
                );
            }
            else if (result == 2)
            {
                if (continuation)
                    continuation();
            }
        }
    );
}

bool MainComponent::writeProjectFile(
    const juce::File& file
)
{
    const auto previousAutosave =
        getAutosaveFile();

    if (file.existsAsFile())
    {
        const auto backupFile =
            getBackupFile(file);

        if (backupFile.existsAsFile())
            backupFile.deleteFile();

        if (! file.copyFileTo(backupFile))
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "Sauvegarde de secours impossible",
                "SonoForge n'a pas pu creer le fichier de sauvegarde (.backup"
                + juce::String(projectExtension) + "). "
                "Le projet principal n'a pas ete ecrase."
            );
            return false;
        }
    }

    const auto snapshot = captureSnapshot();

    if (! writeSnapshotFile(file, snapshot))
        return false;

    currentProjectFile = file;
    lastSavedFingerprint =
        makeSnapshotFingerprint(snapshot);
    addRecentProject(file);

    if (previousAutosave.existsAsFile())
        previousAutosave.deleteFile();

    const auto currentAutosave =
        getAutosaveFile();

    if (currentAutosave.existsAsFile())
        currentAutosave.deleteFile();

    lastAutosaveSerialisedState =
        juce::JSON::toString(
            serialiseSnapshot(
                snapshot,
                currentAutosave
            ),
            false
        );

    updateProjectTitle();
    updateRecoveryButton();

    return true;
}

bool MainComponent::writeSnapshotFile(
    const juce::File& file,
    const ProjectSnapshot& snapshot
)
{
    const auto parent =
        file.getParentDirectory();

    if (! parent.exists()
        && ! parent.createDirectory())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Enregistrement impossible",
            "SonoForge n'a pas pu creer le dossier de sauvegarde."
        );
        return false;
    }

    const auto data =
        serialiseSnapshot(snapshot, file);

    const auto json =
        juce::JSON::toString(data, false);

    if (! file.replaceWithText(json))
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Enregistrement impossible",
            "SonoForge n'a pas pu ecrire le fichier projet."
        );
        return false;
    }

    return true;
}

void MainComponent::performAutosave()
{
    if (audioEngine.getTrackCount() <= 0)
        return;

    const auto autosaveFile =
        getAutosaveFile();

    const auto snapshot =
        captureSnapshot();

    const auto json =
        juce::JSON::toString(
            serialiseSnapshot(
                snapshot,
                autosaveFile
            ),
            false
        );

    if (json == lastAutosaveSerialisedState)
        return;

    const auto parent =
        autosaveFile.getParentDirectory();

    if (! parent.exists()
        && ! parent.createDirectory())
    {
        return;
    }

    if (autosaveFile.replaceWithText(json))
    {
        lastAutosaveSerialisedState = json;
        updateRecoveryButton();
    }
}

void MainComponent::updateRecoveryButton()
{
    const auto autosaveFile =
        getAutosaveFile();

    bool canRecover =
        autosaveFile.existsAsFile();

    if (canRecover
        && currentProjectFile.existsAsFile())
    {
        canRecover =
            autosaveFile
                .getLastModificationTime()
                .toMilliseconds()
            > currentProjectFile
                .getLastModificationTime()
                .toMilliseconds();
    }

    recoverButton.setEnabled(canRecover);
}

juce::File MainComponent::getAutosaveFile() const
{
    juce::File current;
    juce::File legacy;

    if (currentProjectFile
            .getFullPathName()
            .isNotEmpty())
    {
        const auto baseName =
            currentProjectFile.getFileNameWithoutExtension();

        current = currentProjectFile.getSiblingFile(
            baseName + ".autosave" + projectExtension
        );
        legacy = currentProjectFile.getSiblingFile(
            baseName + ".autosave" + legacyProjectExtension
        );
    }
    else
    {
        const auto directory = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory
        )
            .getChildFile("SonoForge")
            .getChildFile("Autosave");

        current = directory.getChildFile(
            juce::String("Recovery.autosave") + projectExtension
        );
        legacy = directory.getChildFile(
            juce::String("Recovery.autosave") + legacyProjectExtension
        );
    }

    // An autosave left by an older version is the only copy of unsaved work
    // after a crash: carry it over so recovery keeps finding it.
    if (legacy.existsAsFile() && ! current.existsAsFile())
        legacy.moveFileTo(current);

    return current;
}

juce::File MainComponent::getBackupFile(
    const juce::File& projectFile
) const
{
    return projectFile.getSiblingFile(
        projectFile.getFileNameWithoutExtension()
        + ".backup" + projectExtension
    );
}

bool MainComponent::loadProjectFile(
    const juce::File& file,
    const bool setAsCurrentProject
)
{
    juce::var data;

    const auto parseResult = juce::JSON::parse(
        file.loadFileAsString(),
        data
    );

    if (parseResult.failed())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Projet illisible",
            parseResult.getErrorMessage()
        );
        return false;
    }

    ProjectSnapshot snapshot;
    juce::StringArray missingFiles;

    const auto result = deserialiseSnapshot(
        data,
        file,
        snapshot,
        missingFiles
    );

    if (result.failed())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Projet invalide",
            result.getErrorMessage()
        );
        return false;
    }

    if (! missingFiles.isEmpty())
    {
        juce::String message =
            "Les fichiers audio suivants sont introuvables :\n\n";

        const auto visibleCount =
            juce::jmin(10, missingFiles.size());

        for (int index = 0; index < visibleCount; ++index)
            message += missingFiles[index] + "\n";

        if (missingFiles.size() > visibleCount)
        {
            message += "\n... et "
                + juce::String(
                    missingFiles.size() - visibleCount
                )
                + " autre(s).";
        }

        message +=
            "\nLe projet n'a pas ete charge afin d'eviter une restauration incomplete.";

        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Medias manquants",
            message
        );
        return false;
    }

    if (! restoreSnapshot(snapshot))
        return false;

    undoStack.clear();
    redoStack.clear();

    if (setAsCurrentProject)
    {
        currentProjectFile = file;
        lastSavedFingerprint =
            makeSnapshotFingerprint(snapshot);
        addRecentProject(file);
    }

    lastAutosaveSerialisedState =
        juce::JSON::toString(
            serialiseSnapshot(
                captureSnapshot(),
                getAutosaveFile()
            ),
            false
        );

    updateProjectTitle();
    updateHistoryButtons();
    updateRecoveryButton();

    return true;
}

juce::var MainComponent::serialiseSnapshot(
    const ProjectSnapshot& snapshot,
    const juce::File& projectFile
) const
{
    auto* root = new juce::DynamicObject();

    root->setProperty("format", "SonoForgeProject");
    root->setProperty("formatVersion", 2);
    root->setProperty("appVersion", "0.9.1");
    root->setProperty(
        "positionSeconds",
        snapshot.positionSeconds
    );
    root->setProperty("bpm", snapshot.bpm);
    root->setProperty(
        "masterGain",
        snapshot.masterGain
    );
    root->setProperty("zoom", snapshot.zoom);
    root->setProperty("view", snapshot.view);
    root->setProperty("gridId", snapshot.gridId);
    root->setProperty(
        "snapEnabled",
        snapshot.snapEnabled
    );

    juce::Array<juce::var> tracks;

    for (const auto& state : snapshot.tracks)
    {
        auto* track = new juce::DynamicObject();

        track->setProperty(
            "absolutePath",
            state.sourceFile.getFullPathName()
        );

        track->setProperty(
            "relativePath",
            state.sourceFile.getRelativePathFrom(
                projectFile.getParentDirectory()
            )
        );

        track->setProperty(
            "startOffsetSeconds",
            state.startOffsetSeconds
        );
        track->setProperty(
            "sourceStartSeconds",
            state.sourceStartSeconds
        );
        track->setProperty(
            "sourceEndSeconds",
            state.sourceEndSeconds
        );
        track->setProperty("gain", state.gain);
        track->setProperty("pan", state.pan);
        track->setProperty("muted", state.muted);
        track->setProperty("solo", state.solo);
        track->setProperty("effects", state.effects.toVar());

        tracks.add(juce::var(track));
    }

    root->setProperty(
        "tracks",
        juce::var(tracks)
    );

    juce::Array<juce::var> midiTracks;

    for (const auto& state : snapshot.midiTracks)
    {
        auto* track = new juce::DynamicObject();

        track->setProperty("name", state.name);
        track->setProperty("gain", state.gain);
        track->setProperty("muted", state.muted);
        track->setProperty(
            "instrumentId",
            state.instrumentId
        );
        track->setProperty("effects", state.effects.toVar());

        midiTracks.add(juce::var(track));
    }

    root->setProperty(
        "midiTracks",
        juce::var(midiTracks)
    );

    return juce::var(root);
}

juce::Result MainComponent::deserialiseSnapshot(
    const juce::var& data,
    const juce::File& projectFile,
    ProjectSnapshot& snapshot,
    juce::StringArray& missingFiles
) const
{
    const auto* root = data.getDynamicObject();

    if (root == nullptr)
    {
        return juce::Result::fail(
            "Le fichier ne contient pas un objet projet valide."
        );
    }

    if (root->getProperty("format").toString()
        != "SonoForgeProject")
    {
        return juce::Result::fail(
            "Ce fichier n'est pas un projet SonoForge."
        );
    }

    const auto formatVersion =
        static_cast<int>(
            root->getProperty("formatVersion")
        );

    if (formatVersion < 1 || formatVersion > 2)
    {
        return juce::Result::fail(
            "Version de projet non prise en charge : "
            + juce::String(formatVersion)
        );
    }

    snapshot.positionSeconds =
        static_cast<double>(
            root->getProperty("positionSeconds")
        );
    snapshot.bpm =
        static_cast<double>(
            root->getProperty("bpm")
        );
    snapshot.masterGain =
        static_cast<double>(
            root->getProperty("masterGain")
        );
    snapshot.zoom =
        static_cast<double>(
            root->getProperty("zoom")
        );
    snapshot.view =
        static_cast<double>(
            root->getProperty("view")
        );
    snapshot.gridId =
        static_cast<int>(
            root->getProperty("gridId")
        );
    snapshot.snapEnabled =
        static_cast<bool>(
            root->getProperty("snapEnabled")
        );

    const auto tracksValue =
        root->getProperty("tracks");

    const auto* tracks = tracksValue.getArray();

    if (tracks == nullptr)
    {
        return juce::Result::fail(
            "La liste des pistes est absente du projet."
        );
    }

    snapshot.tracks.clear();
    snapshot.tracks.reserve(
        static_cast<size_t>(tracks->size())
    );

    for (const auto& value : *tracks)
    {
        const auto* track = value.getDynamicObject();

        if (track == nullptr)
        {
            return juce::Result::fail(
                "Une piste du projet est invalide."
            );
        }

        const auto absolutePath =
            track->getProperty("absolutePath").toString();

        const auto relativePath =
            track->getProperty("relativePath").toString();

        auto sourceFile = juce::File(absolutePath);

        if (! sourceFile.existsAsFile()
            && relativePath.isNotEmpty())
        {
            const auto relativeFile =
                projectFile.getParentDirectory()
                    .getChildFile(relativePath);

            if (relativeFile.existsAsFile())
                sourceFile = relativeFile;
        }

        if (! sourceFile.existsAsFile())
        {
            missingFiles.add(
                absolutePath.isNotEmpty()
                    ? absolutePath
                    : relativePath
            );
            continue;
        }

        AudioEngine::TrackState state;
        state.sourceFile = sourceFile;
        state.startOffsetSeconds =
            static_cast<double>(
                track->getProperty(
                    "startOffsetSeconds"
                )
            );
        state.sourceStartSeconds =
            static_cast<double>(
                track->getProperty(
                    "sourceStartSeconds"
                )
            );
        state.sourceEndSeconds =
            static_cast<double>(
                track->getProperty(
                    "sourceEndSeconds"
                )
            );
        state.gain =
            static_cast<float>(
                static_cast<double>(
                    track->getProperty("gain")
                )
            );
        state.pan =
            static_cast<float>(
                static_cast<double>(
                    track->getProperty("pan")
                )
            );
        state.muted =
            static_cast<bool>(
                track->getProperty("muted")
            );
        state.solo =
            static_cast<bool>(
                track->getProperty("solo")
            );
        // Absent in projects saved before effects existed: defaults apply.
        state.effects = EffectsSettings::fromVar(
            track->getProperty("effects")
        );

        snapshot.tracks.push_back(
            std::move(state)
        );
    }

    snapshot.midiTracks.clear();

    if (formatVersion >= 2)
    {
        const auto midiTracksValue =
            root->getProperty("midiTracks");

        if (const auto* midiTracks =
                midiTracksValue.getArray())
        {
            snapshot.midiTracks.reserve(
                static_cast<size_t>(
                    midiTracks->size()
                )
            );

            for (const auto& value : *midiTracks)
            {
                const auto* track =
                    value.getDynamicObject();

                if (track == nullptr)
                {
                    return juce::Result::fail(
                        "Une piste MIDI du projet est invalide."
                    );
                }

                AudioEngine::MidiTrackState state;
                state.name =
                    track->getProperty("name")
                        .toString();
                state.gain =
                    static_cast<float>(
                        static_cast<double>(
                            track->getProperty("gain")
                        )
                    );
                state.muted =
                    static_cast<bool>(
                        track->getProperty("muted")
                    );
                state.instrumentId =
                    static_cast<int>(
                        track->getProperty(
                            "instrumentId"
                        )
                    );
                state.effects = EffectsSettings::fromVar(
                    track->getProperty("effects")
                );

                snapshot.midiTracks.push_back(
                    std::move(state)
                );
            }
        }
    }

    return juce::Result::ok();
}

void MainComponent::updateProjectTitle()
{
    auto text = juce::String("SonoForge Studio 0.9.1");

    if (currentProjectFile.getFullPathName().isNotEmpty())
    {
        text += " - "
            + currentProjectFile
                .getFileNameWithoutExtension();
    }

    if (hasUnsavedChanges())
        text += " *";

    titleLabel.setText(
        text,
        juce::dontSendNotification
    );
}

bool MainComponent::hasUnsavedChanges() const
{
    return makeSnapshotFingerprint(
        captureSnapshot()
    ) != lastSavedFingerprint;
}

juce::String MainComponent::makeSnapshotFingerprint(
    const ProjectSnapshot& snapshot
) const
{
    juce::String fingerprint;

    fingerprint
        << "bpm=" << juce::String(snapshot.bpm, 6)
        << "|master=" << juce::String(snapshot.masterGain, 6)
        << "|grid=" << juce::String(snapshot.gridId)
        << "|snap=" << juce::String(snapshot.snapEnabled ? 1 : 0);

    for (const auto& state : snapshot.tracks)
    {
        fingerprint
            << "|file=" << state.sourceFile.getFullPathName()
            << "|offset=" << juce::String(state.startOffsetSeconds, 9)
            << "|sourceStart=" << juce::String(state.sourceStartSeconds, 9)
            << "|sourceEnd=" << juce::String(state.sourceEndSeconds, 9)
            << "|gain=" << juce::String(state.gain, 6)
            << "|pan=" << juce::String(state.pan, 6)
            << "|mute=" << juce::String(state.muted ? 1 : 0)
            << "|solo=" << juce::String(state.solo ? 1 : 0)
            << "|fx=" << state.effects.fingerprint();
    }

    for (const auto& state : snapshot.midiTracks)
    {
        fingerprint
            << "|midiName=" << state.name
            << "|midiGain=" << juce::String(state.gain, 6)
            << "|midiMute=" << juce::String(state.muted ? 1 : 0)
            << "|instrument=" << juce::String(state.instrumentId)
            << "|midiFx=" << state.effects.fingerprint();
    }

    return fingerprint;
}

void MainComponent::addRecentProject(
    const juce::File& file
)
{
    const auto path = file.getFullPathName();

    if (path.isEmpty())
        return;

    recentProjectPaths.removeString(path);
    recentProjectPaths.insert(0, path);

    while (recentProjectPaths.size() > 8)
        recentProjectPaths.remove(
            recentProjectPaths.size() - 1
        );

    saveRecentProjects();
    refreshRecentProjectsCombo();
}

void MainComponent::loadRecentProjects()
{
    recentProjectPaths.clear();

    const auto file = getRecentProjectsFile();

    if (file.existsAsFile())
        file.readLines(recentProjectPaths);

    for (int index = recentProjectPaths.size() - 1;
         index >= 0;
         --index)
    {
        if (! juce::File(
                recentProjectPaths[index]
            ).existsAsFile())
        {
            recentProjectPaths.remove(index);
        }
    }

    while (recentProjectPaths.size() > 8)
        recentProjectPaths.remove(
            recentProjectPaths.size() - 1
        );

    saveRecentProjects();
    refreshRecentProjectsCombo();
}

void MainComponent::saveRecentProjects() const
{
    const auto file = getRecentProjectsFile();
    const auto parent = file.getParentDirectory();

    if (! parent.exists())
        parent.createDirectory();

    juce::String content;

    for (const auto& path : recentProjectPaths)
        content += path + "\n";

    file.replaceWithText(content);
}

void MainComponent::refreshRecentProjectsCombo()
{
    recentProjectsCombo.clear(
        juce::dontSendNotification
    );

    for (int index = 0;
         index < recentProjectPaths.size();
         ++index)
    {
        const auto file =
            juce::File(recentProjectPaths[index]);

        recentProjectsCombo.addItem(
            file.getFileNameWithoutExtension(),
            index + 1
        );
    }

    recentProjectsCombo.setSelectedId(
        0,
        juce::dontSendNotification
    );
}

juce::File MainComponent::getRecentProjectsFile() const
{
    return juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory
    )
        .getChildFile("SonoForge")
        .getChildFile("recent-projects.txt");
}

TrackRowComponent* MainComponent::addTrackRow(
    AudioTrack& track
)
{
    auto* row = trackRows.add(
        new TrackRowComponent(
            track,
            audioEngine.getFormatManager(),
            audioEngine.getThumbnailCache()
        )
    );

    row->onSeek = [this](const double seconds)
    {
        seekTo(seconds);
    };

    row->onSoloChanged = [this]
    {
        audioEngine.refreshSoloState();
    };

    row->onClipMoved = [this]
    {
        clipMoved();
    };

    row->onEditBegin = [this]
    {
        pushUndoSnapshot(captureSnapshot());
    };

    row->onSelectionRequested =
        [this](TrackRowComponent* requestedRow)
        {
            selectRow(requestedRow);
        };

    // Right-click clip menu: same operations as the toolbar buttons, but the
    // cut position can be the pointer instead of the playhead.
    row->onSplitRequested = [this, row](const double seconds)
    {
        splitClipAt(*row, seconds);
    };

    row->onDuplicateRequested = [this]
    {
        duplicateSelectedClip();
    };

    row->onDeleteRequested = [this]
    {
        deleteSelectedClip();
    };

    row->setSnapSettings(
        snapButton.getToggleState(),
        getGridIntervalSeconds()
    );

    trackList.addAndMakeVisible(row);
    layoutTracks();

    return row;
}

MidiTrackRowComponent*
MainComponent::addMidiTrackRow(
    MidiInstrumentTrack& track
)
{
    auto* row = midiTrackRows.add(
        new MidiTrackRowComponent(track)
    );

    row->onChanged = [this]
    {
        updateProjectTitle();
    };

    row->onDeleteRequested =
        [this](MidiTrackRowComponent* rowToDelete)
        {
            // Let the button callback finish before destroying its row.
            auto safeThis = juce::Component::SafePointer<MainComponent>(this);
            auto safeRow = juce::Component::SafePointer<MidiTrackRowComponent>(rowToDelete);
            juce::MessageManager::callAsync([safeThis, safeRow]
            {
                if (safeThis != nullptr && safeRow != nullptr)
                    safeThis->deleteMidiTrackRow(safeRow.getComponent());
            });
        };

    trackList.addAndMakeVisible(row);
    layoutTracks();

    return row;
}

void MainComponent::addMidiInstrumentTrack()
{
    if (audioEngine.isRecording())
        return;

    const auto before = captureSnapshot();

    const auto trackNumber =
        audioEngine.getMidiTrackPointers().size() + 1;

    auto* track =
        audioEngine.addMidiInstrumentTrack(
            "Instrument "
            + juce::String(
                static_cast<int>(trackNumber)
            )
        );

    if (track == nullptr)
        return;

    addMidiTrackRow(*track);
    pushUndoSnapshot(before);
    updateProjectState();
}

void MainComponent::deleteMidiTrackRow(
    MidiTrackRowComponent* row
)
{
    if (row == nullptr)
        return;

    const auto before = captureSnapshot();
    auto* track = &row->getTrack();

    // The row destructor stops notes on its track. Destroy the UI first.
    mixerPanel.clearTrackStrips();
    midiTrackRows.removeObject(row, true);
    if (! audioEngine.removeMidiInstrumentTrack(track))
    {
        rebuildTrackRowsFromEngine();
        updateProjectState();
        return;
    }
    pushUndoSnapshot(before);
    updateProjectState();
}

void MainComponent::layoutTracks()
{
    const auto width =
        juce::jmax(500, trackViewport.getWidth() - scrollBarWidth);

    const auto height =
        juce::jmax(
            trackViewport.getHeight(),
            (trackRows.size() + midiTrackRows.size())
                * trackRowHeight
        );

    trackList.setSize(width, height);

    int y = 0;

    for (auto* row : trackRows)
    {
        row->setBounds(
            0,
            y,
            width,
            trackRowHeight - 6
        );

        y += trackRowHeight;
    }

    for (auto* row : midiTrackRows)
    {
        row->setBounds(
            0,
            y,
            width,
            trackRowHeight - 6
        );

        y += trackRowHeight;
    }
}

void MainComponent::updateTimeline(
    const double position
)
{
    const auto length =
        audioEngine.getLengthSeconds();

    const auto viewDuration =
        getViewDuration(length);

    const auto viewStart =
        getViewStart(length, viewDuration);

    viewSlider.setEnabled(
        length > 0.0
        && viewDuration < length - 0.001
    );

    timelineRuler.setView(
        length,
        viewStart,
        viewDuration,
        position,
        getGridIntervalSeconds()
    );

    for (auto* row : trackRows)
    {
        row->setTimelineState(
            length,
            position,
            viewStart,
            viewDuration
        );
    }

    timeLabel.setText(
        formatTime(position)
            + " / "
            + formatTime(length),
        juce::dontSendNotification
    );

    splitButton.setEnabled(canSplitSelectedClip());
}

void MainComponent::updateProjectState()
{
    const auto trackCount =
        audioEngine.getTrackCount();

    const auto hasTracks = trackCount > 0;
    const auto hasSelection = selectedRow != nullptr;

    projectLabel.setText(
        juce::String(trackCount) + " piste(s)",
        juce::dontSendNotification
    );

    playButton.setEnabled(hasTracks);
    stopButton.setEnabled(hasTracks);
    duplicateButton.setEnabled(hasSelection);
    deleteButton.setEnabled(hasSelection);
    splitButton.setEnabled(canSplitSelectedClip());

    if (! hasTracks)
    {
        selectedRow = nullptr;
        pauseButton.setEnabled(false);
        splitButton.setEnabled(false);
    }

    layoutTracks();
    mixerPanel.syncWithEngine();
    updateTimeline(
        audioEngine.getPositionSeconds()
    );
    updateHistoryButtons();
    updateProjectTitle();
}

void MainComponent::seekTo(const double seconds)
{
    audioEngine.setPositionSeconds(seconds);

    updateTimeline(
        audioEngine.getPositionSeconds()
    );
}

void MainComponent::clipMoved()
{
    audioEngine.refreshTrackAlignment();

    updateTimeline(
        audioEngine.getPositionSeconds()
    );
    updateProjectTitle();
}

void MainComponent::selectRow(
    TrackRowComponent* row
)
{
    selectedRow = row;

    for (auto* candidate : trackRows)
    {
        candidate->setSelected(
            candidate == selectedRow
        );
    }

    duplicateButton.setEnabled(selectedRow != nullptr);
    deleteButton.setEnabled(selectedRow != nullptr);
    splitButton.setEnabled(canSplitSelectedClip());
}

void MainComponent::duplicateSelectedClip()
{
    if (selectedRow == nullptr)
        return;

    const auto before = captureSnapshot();

    AudioTrack* createdTrack = nullptr;

    const auto result = audioEngine.duplicateTrack(
        selectedRow->getTrack(),
        createdTrack
    );

    if (result.failed() || createdTrack == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Duplication impossible",
            result.getErrorMessage()
        );
        return;
    }

    createdTrack->setStartOffsetSeconds(
        selectedRow->getTrack().getStartOffsetSeconds()
            + getGridIntervalSeconds()
    );

    audioEngine.refreshTrackAlignment();

    auto* newRow = addTrackRow(*createdTrack);
    selectRow(newRow);

    pushUndoSnapshot(before);
    updateProjectState();
}

void MainComponent::splitSelectedClip()
{
    if (selectedRow == nullptr)
        return;

    splitClipAt(
        *selectedRow,
        audioEngine.getPositionSeconds()
    );
}

void MainComponent::splitClipAt(
    TrackRowComponent& row,
    const double projectSeconds
)
{
    const auto before = captureSnapshot();

    AudioTrack* rightTrack = nullptr;

    const auto result =
        audioEngine.splitTrackAtProjectPosition(
            row.getTrack(),
            projectSeconds,
            rightTrack
        );

    if (result.failed() || rightTrack == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Decoupage impossible",
            result.getErrorMessage()
        );
        return;
    }

    auto* rightRow = addTrackRow(*rightTrack);
    selectRow(rightRow);

    pushUndoSnapshot(before);
    updateProjectState();
}

void MainComponent::deleteSelectedClip()
{
    if (selectedRow == nullptr)
        return;

    const auto before = captureSnapshot();

    auto* rowToDelete = selectedRow;
    auto* trackToDelete = &selectedRow->getTrack();

    auto safeThis =
        juce::Component::SafePointer<MainComponent>(this);

    juce::MessageManager::callAsync(
        [safeThis, rowToDelete, trackToDelete, before]
        {
            if (safeThis == nullptr)
                return;

            safeThis->selectedRow = nullptr;
            safeThis->mixerPanel.clearTrackStrips();
            safeThis->trackRows.removeObject(
                rowToDelete,
                true
            );

            const auto removed =
                safeThis->audioEngine.removeTrack(
                    trackToDelete
                );

            if (removed)
                safeThis->pushUndoSnapshot(before);

            safeThis->updateProjectState();
        }
    );
}

void MainComponent::rebuildTrackRowsFromEngine()
{
    selectedRow = nullptr;
    trackRows.clear(true);
    midiTrackRows.clear(true);

    for (auto* track : audioEngine.getTrackPointers())
    {
        if (track != nullptr)
            addTrackRow(*track);
    }

    for (auto* track :
         audioEngine.getMidiTrackPointers())
    {
        if (track != nullptr)
            addMidiTrackRow(*track);
    }
}

MainComponent::ProjectSnapshot
MainComponent::captureSnapshot() const
{
    ProjectSnapshot snapshot;
    snapshot.tracks = audioEngine.captureTrackStates();
    snapshot.midiTracks =
        audioEngine.captureMidiTrackStates();
    snapshot.positionSeconds =
        audioEngine.getPositionSeconds();
    snapshot.bpm = bpmSlider.getValue();
    snapshot.masterGain = masterSlider.getValue();
    snapshot.zoom = zoomSlider.getValue();
    snapshot.view = viewSlider.getValue();
    snapshot.gridId = gridCombo.getSelectedId();
    snapshot.snapEnabled = snapButton.getToggleState();

    return snapshot;
}

void MainComponent::pushUndoSnapshot(
    const ProjectSnapshot& snapshot
)
{
    undoStack.push_back(snapshot);

    if (undoStack.size() > maxHistoryEntries)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
    updateHistoryButtons();
}

bool MainComponent::restoreSnapshot(
    const ProjectSnapshot& snapshot
)
{
    audioEngine.pause();

    // Rows and mixer strips hold references to the current engine tracks.
    // Stop their timers and destroy them while those tracks are still alive.
    selectedRow = nullptr;
    mixerPanel.clearTrackStrips();
    trackRows.clear(true);
    midiTrackRows.clear(true);

    const auto result =
        audioEngine.restoreProjectTracks(
            snapshot.tracks,
            snapshot.midiTracks
        );

    if (result.failed())
    {
        // The engine preserves the old tracks if restoration fails.
        rebuildTrackRowsFromEngine();
        updateProjectState();
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Restauration impossible",
            result.getErrorMessage()
        );
        return false;
    }

    bpmSlider.setValue(
        snapshot.bpm,
        juce::dontSendNotification
    );
    masterSlider.setValue(
        snapshot.masterGain,
        juce::dontSendNotification
    );
    zoomSlider.setValue(
        snapshot.zoom,
        juce::dontSendNotification
    );
    viewSlider.setValue(
        snapshot.view,
        juce::dontSendNotification
    );
    gridCombo.setSelectedId(
        snapshot.gridId,
        juce::dontSendNotification
    );
    snapButton.setToggleState(
        snapshot.snapEnabled,
        juce::dontSendNotification
    );

    audioEngine.setMasterGain(
        static_cast<float>(snapshot.masterGain)
    );
    audioEngine.setPositionSeconds(
        snapshot.positionSeconds
    );

    rebuildTrackRowsFromEngine();
    updateGridSettings();
    updateProjectState();

    return true;
}

void MainComponent::undo()
{
    if (undoStack.empty())
        return;

    const auto current = captureSnapshot();
    const auto target = undoStack.back();

    if (! restoreSnapshot(target))
        return;

    undoStack.pop_back();
    redoStack.push_back(current);

    if (redoStack.size() > maxHistoryEntries)
        redoStack.erase(redoStack.begin());

    updateHistoryButtons();
}

void MainComponent::redo()
{
    if (redoStack.empty())
        return;

    const auto current = captureSnapshot();
    const auto target = redoStack.back();

    if (! restoreSnapshot(target))
        return;

    redoStack.pop_back();
    undoStack.push_back(current);

    if (undoStack.size() > maxHistoryEntries)
        undoStack.erase(undoStack.begin());

    updateHistoryButtons();
}

void MainComponent::updateHistoryButtons()
{
    undoButton.setEnabled(! undoStack.empty());
    redoButton.setEnabled(! redoStack.empty());
}

void MainComponent::toggleRecording()
{
    if (audioEngine.isRecording())
        finishAudioRecording();
    else
        startAudioRecording();
}

void MainComponent::startAudioRecording()
{
    if (audioEngine.isRecording())
        return;

    const auto directory =
        getRecordingDirectory();

    if (! directory.exists()
        && ! directory.createDirectory())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Enregistrement impossible",
            "Impossible de creer le dossier Audio."
        );
        return;
    }

    activeRecordingFile =
        directory.getChildFile(
            "Recording_"
            + juce::String(
                juce::Time::currentTimeMillis()
            )
            + ".wav"
        );

    recordingUndoSnapshot =
        std::make_unique<ProjectSnapshot>(
            captureSnapshot()
        );

    const auto result =
        audioEngine.startRecording(
            activeRecordingFile
        );

    if (result.failed())
    {
        recordingUndoSnapshot.reset();
        activeRecordingFile = {};

        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Enregistrement impossible",
            result.getErrorMessage()
        );
        return;
    }

    activeRecordingStart =
        audioEngine.getRecordingStartPosition();

    recordButton.setButtonText("Stop Rec");
    recordButton.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour(0xffb13b3b)
    );

    playButton.setEnabled(false);
    pauseButton.setEnabled(false);
    openProjectButton.setEnabled(false);
    audioSettingsButton.setEnabled(false);
}

void MainComponent::finishAudioRecording()
{
    if (! audioEngine.isRecording())
        return;

    audioEngine.stopRecording();

    recordButton.setButtonText("Record");
    recordButton.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour(panelColour)
    );

    openProjectButton.setEnabled(true);
    audioSettingsButton.setEnabled(true);

    if (! activeRecordingFile.existsAsFile()
        || activeRecordingFile.getSize() <= 44)
    {
        recordingUndoSnapshot.reset();
        activeRecordingFile = {};

        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Enregistrement vide",
            "Aucun contenu audio exploitable n'a ete enregistre."
        );

        updateProjectState();
        return;
    }

    AudioTrack* createdTrack = nullptr;

    const auto result =
        audioEngine.addTrackFromFile(
            activeRecordingFile,
            createdTrack
        );

    if (result.failed() || createdTrack == nullptr)
    {
        recordingUndoSnapshot.reset();

        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Import de l'enregistrement impossible",
            result.getErrorMessage()
        );

        activeRecordingFile = {};
        updateProjectState();
        return;
    }

    createdTrack->setStartOffsetSeconds(
        activeRecordingStart
    );

    audioEngine.refreshTrackAlignment();

    auto* row = addTrackRow(*createdTrack);
    selectRow(row);

    if (recordingUndoSnapshot != nullptr)
    {
        pushUndoSnapshot(
            *recordingUndoSnapshot
        );
    }

    recordingUndoSnapshot.reset();
    activeRecordingFile = {};

    updateProjectState();
}

juce::File MainComponent::getRecordingDirectory() const
{
    if (currentProjectFile
            .getFullPathName()
            .isNotEmpty())
    {
        return currentProjectFile
            .getParentDirectory()
            .getChildFile("Audio");
    }

    return juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory
    )
        .getChildFile("SonoForge Recordings");
}

void MainComponent::updateGridSettings()
{
    const auto interval =
        getGridIntervalSeconds();

    for (auto* row : trackRows)
    {
        row->setSnapSettings(
            snapButton.getToggleState(),
            interval
        );
    }
}

double MainComponent::getViewDuration(
    const double projectLength
) const
{
    if (projectLength <= 0.0)
        return 0.0;

    const auto zoom = juce::jmax(
        1.0,
        zoomSlider.getValue()
    );

    return juce::jlimit(
        0.1,
        projectLength,
        projectLength / zoom
    );
}

double MainComponent::getViewStart(
    const double projectLength,
    const double viewDuration
) const
{
    const auto maxStart = juce::jmax(
        0.0,
        projectLength - viewDuration
    );

    return viewSlider.getValue() * maxStart;
}

double MainComponent::getGridIntervalSeconds() const
{
    const auto bpm = juce::jmax(
        1.0,
        bpmSlider.getValue()
    );

    const auto beat = 60.0 / bpm;

    switch (gridCombo.getSelectedId())
    {
        case 2:
            return beat * 0.5;

        case 3:
            return beat * 0.25;

        case 1:
        default:
            return beat;
    }
}

bool MainComponent::canSplitSelectedClip() const
{
    if (selectedRow == nullptr)
        return false;

    const auto& track = selectedRow->getTrack();

    const auto playhead =
        audioEngine.getPositionSeconds();

    const auto start =
        track.getStartOffsetSeconds();

    const auto end =
        track.getProjectEndSeconds();

    return playhead > start + minimumSplitMarginSeconds
        && playhead < end - minimumSplitMarginSeconds;
}

void MainComponent::showAudioSettings()
{
    if (audioEngine.isRecording())
        return;

    auto* selector =
        new juce::AudioDeviceSelectorComponent(
            audioEngine.getDeviceManager(),
            0,
            2,
            0,
            2,
            false,
            false,
            true,
            false
        );

    selector->setSize(540, 420);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(selector);
    options.dialogTitle = "Configuration audio";
    options.dialogBackgroundColour =
        juce::Colour(panelColour);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;

    options.launchAsync();
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        if (audioEngine.isRecording())
            return true;

        if (audioEngine.isPlaying())
        {
            pauseButton.triggerClick();
            return true;
        }

        if (playButton.isEnabled())
        {
            playButton.triggerClick();
            return true;
        }
    }

    return false;
}

void MainComponent::timerCallback()
{
    if (juce::Component::getCurrentlyFocusedComponent() == nullptr
        && isShowing()
        && juce::Process::isForegroundProcess())
    {
        grabKeyboardFocus();
    }

    const auto current =
        audioEngine.getPositionSeconds();

    updateTimeline(current);

    if (! audioEngine.isRecording())
    {
        pauseButton.setEnabled(audioEngine.isPlaying());
        recordButton.setEnabled(true);
    }
    else
    {
        pauseButton.setEnabled(false);
    }

    const auto now =
        juce::Time::getMillisecondCounterHiRes();

    if (now - lastAutosaveCheckMs
        >= autosaveIntervalMs)
    {
        lastAutosaveCheckMs = now;
        performAutosave();
        updateProjectTitle();
        updateRecoveryButton();
    }
}

juce::String MainComponent::formatTime(
    const double seconds
)
{
    const auto totalMilliseconds =
        static_cast<int64_t>(
            std::round(seconds * 1000.0)
        );

    const auto minutes =
        totalMilliseconds / 60000;
    const auto remainingMilliseconds =
        totalMilliseconds % 60000;
    const auto wholeSeconds =
        remainingMilliseconds / 1000;
    const auto milliseconds =
        remainingMilliseconds % 1000;

    return juce::String(minutes).paddedLeft('0', 2)
        + ":"
        + juce::String(wholeSeconds).paddedLeft('0', 2)
        + "."
        + juce::String(milliseconds).paddedLeft('0', 3);
}
