#include "MidiTrackRowComponent.h"

namespace
{
constexpr auto rowColour = 0xff28223a;
constexpr auto timelineColour = 0xff171421;
constexpr auto textColour = 0xffe8edf3;
constexpr auto midiColour = 0xffb58cff;
constexpr auto muteColour = 0xffd65454;
constexpr int controlsWidth = 240;
}

MidiTrackRowComponent::MidiTrackRowComponent(
    MidiInstrumentTrack& trackToControl
)
    : track(trackToControl)
{
    nameLabel.setText(
        track.getName(),
        juce::dontSendNotification
    );
    nameLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour)
    );
    nameLabel.setFont(
        juce::FontOptions(16.0f, juce::Font::bold)
    );
    addAndMakeVisible(nameLabel);

    typeLabel.setText(
        "MIDI / Instrument",
        juce::dontSendNotification
    );
    typeLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(midiColour)
    );
    addAndMakeVisible(typeLabel);

    instrumentLabel.setText(
        "Instrument : Basic Synth",
        juce::dontSendNotification
    );
    instrumentLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour).withAlpha(0.75f)
    );
    addAndMakeVisible(instrumentLabel);

    volumeLabel.setText(
        "Vol",
        juce::dontSendNotification
    );
    volumeLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour).withAlpha(0.75f)
    );
    addAndMakeVisible(volumeLabel);

    muteButton.onClick = [this]
    {
        track.setMuted(! track.isMuted());
        refreshMuteButton();

        if (onChanged)
            onChanged();
    };
    addAndMakeVisible(muteButton);

    testButton.onClick = [this]
    {
        track.noteOn(60, 0.8f);
        startTimer(450);
    };
    addAndMakeVisible(testButton);

    deleteButton.onClick = [this]
    {
        if (onDeleteRequested)
            onDeleteRequested(this);
    };
    addAndMakeVisible(deleteButton);

    volumeSlider.setRange(0.0, 1.5, 0.01);
    volumeSlider.setValue(track.getGain());
    volumeSlider.setSliderStyle(
        juce::Slider::LinearHorizontal
    );
    volumeSlider.setTextBoxStyle(
        juce::Slider::TextBoxRight,
        false,
        52,
        22
    );
    volumeSlider.onValueChange = [this]
    {
        track.setGain(
            static_cast<float>(
                volumeSlider.getValue()
            )
        );

        if (onChanged)
            onChanged();
    };
    addAndMakeVisible(volumeSlider);

    refreshMuteButton();
}

MidiTrackRowComponent::~MidiTrackRowComponent()
{
    stopTimer();
    track.allNotesOff();
}

void MidiTrackRowComponent::paint(
    juce::Graphics& graphics
)
{
    const auto bounds =
        getLocalBounds().reduced(2);

    graphics.setColour(
        juce::Colour(rowColour)
    );
    graphics.fillRoundedRectangle(
        bounds.toFloat(),
        6.0f
    );

    auto timeline =
        getLocalBounds()
            .withTrimmedLeft(controlsWidth)
            .reduced(5, 8);

    graphics.setColour(
        juce::Colour(timelineColour)
    );
    graphics.fillRect(timeline);

    graphics.setColour(
        juce::Colour(midiColour).withAlpha(0.18f)
    );
    graphics.fillRoundedRectangle(
        timeline.reduced(8, 12).toFloat(),
        4.0f
    );

    graphics.setColour(
        juce::Colour(midiColour)
    );
    graphics.setFont(14.0f);
    graphics.drawText(
        "Piste instrument MIDI - piano roll a venir",
        timeline.reduced(16, 14),
        juce::Justification::centredLeft
    );
}

void MidiTrackRowComponent::resized()
{
    auto controls =
        getLocalBounds()
            .withWidth(controlsWidth)
            .reduced(10, 7);

    nameLabel.setBounds(
        controls.removeFromTop(22)
    );

    typeLabel.setBounds(
        controls.removeFromTop(18)
    );

    instrumentLabel.setBounds(
        controls.removeFromTop(20)
    );

    auto actions =
        controls.removeFromTop(26);

    muteButton.setBounds(
        actions.removeFromLeft(38)
    );
    actions.removeFromLeft(5);

    testButton.setBounds(
        actions.removeFromLeft(82)
    );
    actions.removeFromLeft(5);

    deleteButton.setBounds(
        actions.removeFromLeft(90)
    );

    auto volume =
        controls.removeFromTop(26);

    volumeLabel.setBounds(
        volume.removeFromLeft(30)
    );
    volumeSlider.setBounds(volume);
}

MidiInstrumentTrack&
MidiTrackRowComponent::getTrack() noexcept
{
    return track;
}

void MidiTrackRowComponent::syncControlsFromTrack()
{
    volumeSlider.setValue(
        track.getGain(),
        juce::dontSendNotification
    );
    refreshMuteButton();
}

void MidiTrackRowComponent::timerCallback()
{
    stopTimer();
    track.noteOff(60);
}

void MidiTrackRowComponent::refreshMuteButton()
{
    muteButton.setColour(
        juce::TextButton::buttonColourId,
        track.isMuted()
            ? juce::Colour(muteColour)
            : juce::Colour(rowColour).brighter(0.15f)
    );
}
