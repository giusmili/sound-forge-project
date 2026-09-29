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
}

MainComponent::MainComponent()
{
    setOpaque(true);
    setSize(1320, 820);

    titleLabel.setText(
        "SonoForge Studio 0.4.1",
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
             &openButton,
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

    openButton.onClick = [this]
    {
        openAudioFiles();
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
    updateProjectState();
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
    openButton.setBounds(
        importRow.removeFromLeft(160).reduced(4)
    );
    importRow.removeFromLeft(8);
    projectLabel.setBounds(
        importRow.removeFromLeft(120)
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

            safeThis->updateProjectState();
        }
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

    updateProjectState();
}

void MainComponent::splitSelectedClip()
{
    if (selectedRow == nullptr)
        return;

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

    updateProjectState();
}

void MainComponent::deleteSelectedClip()
{
    if (selectedRow == nullptr)
        return;

    auto* rowToDelete = selectedRow;
    auto* trackToDelete = &selectedRow->getTrack();

    auto safeThis =
        juce::Component::SafePointer<MainComponent>(this);

    juce::MessageManager::callAsync(
        [safeThis, rowToDelete, trackToDelete]
        {
            if (safeThis == nullptr)
                return;

            safeThis->selectedRow = nullptr;
            safeThis->trackRows.removeObject(
                rowToDelete,
                true
            );

            safeThis->audioEngine.removeTrack(
                trackToDelete
            );

            safeThis->updateProjectState();
        }
    );
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
