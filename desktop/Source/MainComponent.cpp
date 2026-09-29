#include "MainComponent.h"

#include <cmath>

namespace
{
constexpr auto backgroundColour = 0xff111318;
constexpr auto panelColour = 0xff1b1f26;
constexpr auto accentColour = 0xff5aa9ff;
constexpr auto textColour = 0xffe8edf3;
}

MainComponent::MainComponent()
{
    setOpaque(true);
    setSize(900, 520);

    titleLabel.setText("SonoForge Studio 0.1", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(textColour));
    addAndMakeVisible(titleLabel);

    fileLabel.setText("Aucun fichier audio charge", juce::dontSendNotification);
    fileLabel.setColour(juce::Label::textColourId, juce::Colour(textColour).withAlpha(0.75f));
    addAndMakeVisible(fileLabel);

    timeLabel.setText("00:00.000 / 00:00.000", juce::dontSendNotification);
    timeLabel.setJustificationType(juce::Justification::centred);
    timeLabel.setColour(juce::Label::textColourId, juce::Colour(textColour));
    addAndMakeVisible(timeLabel);

    volumeLabel.setText("Master", juce::dontSendNotification);
    volumeLabel.setColour(juce::Label::textColourId, juce::Colour(textColour));
    addAndMakeVisible(volumeLabel);

    for (auto* button : { &openButton, &playButton, &stopButton, &audioSettingsButton })
    {
        button->setColour(juce::TextButton::buttonColourId, juce::Colour(panelColour));
        button->setColour(juce::TextButton::textColourOffId, juce::Colour(textColour));
        addAndMakeVisible(*button);
    }

    playButton.setEnabled(false);
    stopButton.setEnabled(false);

    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.8);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 24);
    volumeSlider.setColour(juce::Slider::trackColourId, juce::Colour(accentColour));
    addAndMakeVisible(volumeSlider);

    openButton.onClick = [this] { openAudioFile(); };

    playButton.onClick = [this]
    {
        audioEngine.play();
        stopButton.setEnabled(true);
    };

    stopButton.onClick = [this]
    {
        audioEngine.stop();
        stopButton.setEnabled(false);
    };

    audioSettingsButton.onClick = [this] { showAudioSettings(); };

    volumeSlider.onValueChange = [this]
    {
        audioEngine.setGain(static_cast<float>(volumeSlider.getValue()));
    };

    audioEngine.setGain(static_cast<float>(volumeSlider.getValue()));

    startTimerHz(20);
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(backgroundColour));

    auto panel = getLocalBounds().reduced(20).withTrimmedTop(80);
    graphics.setColour(juce::Colour(panelColour));
    graphics.fillRoundedRectangle(panel.toFloat(), 8.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(20);

    auto header = area.removeFromTop(54);
    titleLabel.setBounds(header.removeFromLeft(350));
    audioSettingsButton.setBounds(header.removeFromRight(100).reduced(4));

    area.removeFromTop(26);

    auto panel = area.reduced(20);

    auto fileRow = panel.removeFromTop(42);
    openButton.setBounds(fileRow.removeFromLeft(150).reduced(4));
    fileRow.removeFromLeft(10);
    fileLabel.setBounds(fileRow.reduced(4));

    panel.removeFromTop(35);

    auto transportRow = panel.removeFromTop(70);
    playButton.setBounds(transportRow.removeFromLeft(100).reduced(6));
    stopButton.setBounds(transportRow.removeFromLeft(100).reduced(6));
    transportRow.removeFromLeft(20);
    timeLabel.setBounds(transportRow.removeFromLeft(280).reduced(6));

    panel.removeFromTop(35);

    auto volumeRow = panel.removeFromTop(60);
    volumeLabel.setBounds(volumeRow.removeFromLeft(80));
    volumeSlider.setBounds(volumeRow.removeFromLeft(420));
}

void MainComponent::openAudioFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choisir un fichier audio",
        juce::File {},
        "*.wav;*.mp3;*.aiff;*.aif;*.flac;*.ogg"
    );

    auto safeThis = juce::Component::SafePointer<MainComponent>(this);

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr)
                return;

            const auto file = chooser.getResult();

            if (! file.existsAsFile())
                return;

            const auto result = safeThis->audioEngine.loadFile(file);

            if (result.wasOk())
            {
                safeThis->fileLabel.setText(
                    file.getFileName(),
                    juce::dontSendNotification
                );
                safeThis->playButton.setEnabled(true);
                safeThis->stopButton.setEnabled(false);
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    "Import audio impossible",
                    result.getErrorMessage()
                );
            }
        }
    );
}

void MainComponent::showAudioSettings()
{
    auto* selector = new juce::AudioDeviceSelectorComponent(
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
    options.dialogBackgroundColour = juce::Colour(panelColour);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;

    options.launchAsync();
}

void MainComponent::timerCallback()
{
    const auto current = audioEngine.getPositionSeconds();
    const auto length = audioEngine.getLengthSeconds();

    timeLabel.setText(
        formatTime(current) + " / " + formatTime(length),
        juce::dontSendNotification
    );

    if (! audioEngine.isPlaying() && current >= length && length > 0.0)
        stopButton.setEnabled(false);
}

juce::String MainComponent::formatTime(const double seconds)
{
    const auto totalMilliseconds = static_cast<int64_t>(std::round(seconds * 1000.0));
    const auto minutes = totalMilliseconds / 60000;
    const auto remainingMilliseconds = totalMilliseconds % 60000;
    const auto wholeSeconds = remainingMilliseconds / 1000;
    const auto milliseconds = remainingMilliseconds % 1000;

    return juce::String(minutes).paddedLeft('0', 2)
        + ":"
        + juce::String(wholeSeconds).paddedLeft('0', 2)
        + "."
        + juce::String(milliseconds).paddedLeft('0', 3);
}
