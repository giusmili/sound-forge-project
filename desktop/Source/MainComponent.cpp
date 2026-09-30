#include "MainComponent.h"
#include "AppServices.h"

#include <cmath>

namespace
{
constexpr auto backgroundColour = 0xff111318;
constexpr auto panelColour = 0xff1b1f26;
constexpr auto accentColour = 0xff5aa9ff;
constexpr auto textColour = 0xffe8edf3;
constexpr int trackRowHeight = 112;
}

MainComponent::MainComponent()
{
    AppServices::log("MainComponent constructed");
    setOpaque(true);
    setSize(1180, 760);

    titleLabel.setText(
        "SonoForge Studio 0.2.3",
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
    audioStatusLabel.setFont(juce::FontOptions(12.0f));
    audioStatusLabel.setColour(juce::Label::textColourId, juce::Colour(textColour));
    addAndMakeVisible(audioStatusLabel);

    projectLabel.setText(
        "0 piste",
        juce::dontSendNotification
    );
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

    masterLabel.setText(
        "Master",
        juce::dontSendNotification
    );
    masterLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour)
    );
    addAndMakeVisible(masterLabel);

    for (auto* button : {
             &openButton,
             &playButton,
             &pauseButton,
             &stopButton,
             &audioSettingsButton,
             &mixerButton
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

    playButton.setEnabled(false);
    pauseButton.setEnabled(false);
    stopButton.setEnabled(false);

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

    timelineRuler.onSeek = [this](const double seconds)
    {
        seekTo(seconds);
    };
    addAndMakeVisible(timelineRuler);

    trackViewport.setViewedComponent(&trackList, false);
    trackViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(trackViewport);

    mixer.onMasterGainChanged = [this](const float gain)
    {
        masterSlider.setValue(gain, juce::sendNotificationSync);
    };
    mixer.onSoloChanged = [this]
    {
        audioEngine.refreshSoloState();
    };
    addAndMakeVisible(mixer);

    openButton.onClick = [this]
    {
        openAudioFiles();
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

    mixerButton.onClick = [this]
    {
        mixerVisible = ! mixerVisible;
        mixer.setVisible(mixerVisible);
        resized();
    };

    masterSlider.onValueChange = [this]
    {
        const auto gain = static_cast<float>(masterSlider.getValue());
        audioEngine.setMasterGain(gain);
        mixer.setMasterGain(gain);

        auto settings = AppServices::loadSettings();
        if (auto* object = settings.getDynamicObject())
        {
            object->setProperty("masterGain", gain);
            AppServices::saveSettings(settings);
        }
    };

    const auto settings = AppServices::loadSettings();
    if (auto* object = settings.getDynamicObject())
    {
        const auto savedGain = static_cast<double>(object->getProperty("masterGain"));
        if (savedGain >= 0.0 && savedGain <= 1.0)
            masterSlider.setValue(savedGain, juce::dontSendNotification);
    }

    audioEngine.setMasterGain(
        static_cast<float>(masterSlider.getValue())
    );
    mixer.setMasterGain(
        static_cast<float>(masterSlider.getValue())
    );

    AppServices::log("MainComponent ready");
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    fileChooser.reset();
    // The selector holds a reference to the engine's device manager.
    // Destroy it before the engine, even if the user left the dialog open.
    if (audioSettingsWindow != nullptr)
        delete audioSettingsWindow.getComponent();
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
    auto titleArea = header.removeFromLeft(juce::jmax(340, header.getWidth() - 220));
    titleLabel.setBounds(titleArea.removeFromTop(30));
    audioStatusLabel.setBounds(titleArea);
    audioSettingsButton.setBounds(
        header.removeFromRight(100).reduced(4)
    );
    mixerButton.setBounds(
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
        importRow.removeFromLeft(180)
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

    area.removeFromTop(8);

    timelineRuler.setBounds(
        area.removeFromTop(34)
    );

    area.removeFromTop(2);

    if (mixerVisible)
    {
        auto mixerArea = area.removeFromBottom(260);
        area.removeFromBottom(8);
        mixer.setBounds(mixerArea);
    }
    else
    {
        mixer.setBounds({});
    }

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
            AppServices::log("File chooser returned " + juce::String(files.size()) + " file(s)");

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
                    AppServices::log("UI: adding track row");
                    safeThis->addTrackRow(*createdTrack);
                    AppServices::log("UI: track row added");
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

            const auto hasTracks =
                safeThis->audioEngine.getTrackCount() > 0;

            safeThis->playButton.setEnabled(hasTracks);
            safeThis->stopButton.setEnabled(hasTracks);

            safeThis->projectLabel.setText(
                juce::String(
                    safeThis->audioEngine.getTrackCount()
                )
                    + " piste(s)",
                juce::dontSendNotification
            );

            safeThis->updateTimeline(
                safeThis->audioEngine.getPositionSeconds()
            );
            safeThis->layoutTracks();
        }
    );
}

void MainComponent::addTrackRow(AudioTrack& track)
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

    trackList.addAndMakeVisible(row);
    mixer.addTrack(track);
    layoutTracks();
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

    timelineRuler.setProjectLength(length);
    timelineRuler.setPosition(position);

    for (auto* row : trackRows)
        row->setTimelineState(length, position);

    timeLabel.setText(
        formatTime(position)
            + " / "
            + formatTime(length),
        juce::dontSendNotification
    );
}

void MainComponent::seekTo(const double seconds)
{
    audioEngine.setPositionSeconds(seconds);
    updateTimeline(
        audioEngine.getPositionSeconds()
    );
}

void MainComponent::showAudioSettings()
{
    if (audioSettingsWindow != nullptr)
    {
        audioSettingsWindow->toFront(true);
        return;
    }
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

    audioSettingsWindow = options.launchAsync();
}

void MainComponent::timerCallback()
{
    audioStatusLabel.setText(audioEngine.getAudioStatus(), juce::dontSendNotification);
    const auto current =
        audioEngine.getPositionSeconds();

    updateTimeline(current);
    mixer.refresh();
    mixer.setMasterLevels(
        audioEngine.getMasterPeakLeft(),
        audioEngine.getMasterPeakRight()
    );
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
