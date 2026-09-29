#include "MainComponent.h"

#include <cmath>

namespace
{
constexpr auto backgroundColour = 0xff111318;
constexpr auto panelColour = 0xff1b1f26;
constexpr auto accentColour = 0xff5aa9ff;
constexpr auto textColour = 0xffe8edf3;
constexpr auto snapColour = 0xff3d8f68;
constexpr int trackRowHeight = 112;
constexpr double minimumSplitMarginSeconds = 0.01;
constexpr auto maxHistoryEntries = 50U;
constexpr double autosaveIntervalMs = 30000.0;
}

MainComponent::MainComponent()
{
    setOpaque(true);
    setSize(1320, 820);

    titleLabel.setText(
        "SonoForge Studio 0.5.1",
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
             &undoButton,
             &redoButton,
             &duplicateButton,
             &splitButton,
             &deleteButton,
             &playButton,
             &pauseButton,
             &stopButton,
             &audioSettingsButton,
             &snapButton
         })
    {
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
    addAndMakeVisible(gridCombo);

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
    addAndMakeVisible(trackViewport);

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
        audioEngine.stop();
        updateTimeline(0.0);
        pauseButton.setEnabled(false);
    };

    audioSettingsButton.onClick = [this]
    {
        showAudioSettings();
    };

    snapButton.onClick = [this]
    {
        updateGridSettings();
    };

    gridCombo.onChange = [this]
    {
        updateGridSettings();
        updateTimeline(
            audioEngine.getPositionSeconds()
        );
    };

    masterSlider.onValueChange = [this]
    {
        audioEngine.setMasterGain(
            static_cast<float>(masterSlider.getValue())
        );
    };

    bpmSlider.onValueChange = [this]
    {
        updateGridSettings();
        updateTimeline(
            audioEngine.getPositionSeconds()
        );
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
    updateProjectTitle();
    updateProjectState();
    updateRecoveryButton();
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(backgroundColour));

    auto panel = getLocalBounds()
                     .reduced(20)
                     .withTrimmedTop(76);

    graphics.setColour(juce::Colour(panelColour));
    graphics.fillRoundedRectangle(
        panel.toFloat(),
        8.0f
    );
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(20);

    auto header = area.removeFromTop(52);
    titleLabel.setBounds(
        header.removeFromLeft(360)
    );
    audioSettingsButton.setBounds(
        header.removeFromRight(100).reduced(4)
    );

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
        importRow.removeFromLeft(140).reduced(4)
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

    area.removeFromTop(6);

    timelineRuler.setBounds(
        area.removeFromTop(34)
    );

    area.removeFromTop(2);
    trackViewport.setBounds(area);

    layoutTracks();
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

            const auto files = chooser.getResults();

            const auto before =
                safeThis->captureSnapshot();

            bool projectChanged = false;
            TrackRowComponent* lastAdded = nullptr;

            for (const auto& file : files)
            {
                if (! file.existsAsFile())
                    continue;

                AudioTrack* createdTrack = nullptr;

                const auto result =
                    safeThis->audioEngine.addTrackFromFile(
                        file,
                        createdTrack
                    );

                if (result.wasOk()
                    && createdTrack != nullptr)
                {
                    lastAdded =
                        safeThis->addTrackRow(*createdTrack);
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
                safeThis->selectRow(lastAdded);

            if (projectChanged)
                safeThis->pushUndoSnapshot(before);

            safeThis->updateProjectState();
        }
    );
}

void MainComponent::openProject()
{
    projectFileChooser = std::make_unique<juce::FileChooser>(
        "Ouvrir un projet SonoForge",
        juce::File {},
        "*.sonoforge"
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

            safeThis->loadProjectFile(file);
        }
    );
}

void MainComponent::saveProject()
{
    if (currentProjectFile.getFullPathName().isNotEmpty())
    {
        writeProjectFile(currentProjectFile);
        return;
    }

    projectFileChooser = std::make_unique<juce::FileChooser>(
        "Enregistrer le projet SonoForge",
        juce::File::getSpecialLocation(
            juce::File::userDocumentsDirectory
        ).getChildFile("projet.sonoforge"),
        "*.sonoforge"
    );

    auto safeThis =
        juce::Component::SafePointer<MainComponent>(this);

    projectFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr)
                return;

            auto file = chooser.getResult();

            if (file.getFullPathName().isEmpty())
                return;

            if (! file.hasFileExtension(".sonoforge"))
                file = file.withFileExtension(".sonoforge");

            safeThis->writeProjectFile(file);
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

    loadProjectFile(autosaveFile, false);
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
                "SonoForge n'a pas pu creer le fichier .backup.sonoforge. "
                "Le projet principal n'a pas ete ecrase."
            );
            return false;
        }
    }

    const auto snapshot = captureSnapshot();

    if (! writeSnapshotFile(file, snapshot))
        return false;

    currentProjectFile = file;

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
    if (currentProjectFile
            .getFullPathName()
            .isNotEmpty())
    {
        return currentProjectFile
            .getSiblingFile(
                currentProjectFile
                    .getFileNameWithoutExtension()
                + ".autosave.sonoforge"
            );
    }

    return juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory
    )
        .getChildFile("SonoForge")
        .getChildFile("Autosave")
        .getChildFile("Recovery.autosave.sonoforge");
}

juce::File MainComponent::getBackupFile(
    const juce::File& projectFile
) const
{
    return projectFile.getSiblingFile(
        projectFile.getFileNameWithoutExtension()
        + ".backup.sonoforge"
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
        currentProjectFile = file;

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
    root->setProperty("formatVersion", 1);
    root->setProperty("appVersion", "0.5.1");
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

        tracks.add(juce::var(track));
    }

    root->setProperty(
        "tracks",
        juce::var(tracks)
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

    if (formatVersion != 1)
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

        snapshot.tracks.push_back(
            std::move(state)
        );
    }

    return juce::Result::ok();
}

void MainComponent::updateProjectTitle()
{
    auto text = juce::String("SonoForge Studio 0.5.1");

    if (currentProjectFile.getFullPathName().isNotEmpty())
    {
        text += " - "
            + currentProjectFile
                .getFileNameWithoutExtension();
    }

    titleLabel.setText(
        text,
        juce::dontSendNotification
    );
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

    row->setSnapSettings(
        snapButton.getToggleState(),
        getGridIntervalSeconds()
    );

    trackList.addAndMakeVisible(row);
    layoutTracks();

    return row;
}

void MainComponent::layoutTracks()
{
    const auto width =
        juce::jmax(500, trackViewport.getWidth() - 14);

    const auto height =
        juce::jmax(
            trackViewport.getHeight(),
            trackRows.size() * trackRowHeight
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
    updateTimeline(
        audioEngine.getPositionSeconds()
    );
    updateHistoryButtons();
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

    const auto before = captureSnapshot();

    AudioTrack* rightTrack = nullptr;

    const auto result =
        audioEngine.splitTrackAtProjectPosition(
            selectedRow->getTrack(),
            audioEngine.getPositionSeconds(),
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

    for (auto* track : audioEngine.getTrackPointers())
    {
        if (track != nullptr)
            addTrackRow(*track);
    }
}

MainComponent::ProjectSnapshot
MainComponent::captureSnapshot() const
{
    ProjectSnapshot snapshot;
    snapshot.tracks = audioEngine.captureTrackStates();
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

    const auto result =
        audioEngine.restoreTrackStates(snapshot.tracks);

    if (result.failed())
    {
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
    auto* selector =
        new juce::AudioDeviceSelectorComponent(
            audioEngine.getDeviceManager(),
            0,
            0,
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

void MainComponent::timerCallback()
{
    const auto current =
        audioEngine.getPositionSeconds();

    updateTimeline(current);
    pauseButton.setEnabled(audioEngine.isPlaying());

    const auto now =
        juce::Time::getMillisecondCounterHiRes();

    if (now - lastAutosaveCheckMs
        >= autosaveIntervalMs)
    {
        lastAutosaveCheckMs = now;
        performAutosave();
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
