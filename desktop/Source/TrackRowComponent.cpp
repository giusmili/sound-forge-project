#include "TrackRowComponent.h"

namespace
{
constexpr auto rowColour = 0xff222831;
constexpr auto textColour = 0xffe8edf3;
constexpr auto accentColour = 0xff5aa9ff;
constexpr auto muteColour = 0xffd65454;
}

TrackRowComponent::TrackRowComponent(AudioTrack& trackToControl)
    : track(trackToControl)
{
    nameLabel.setText(track.getName(), juce::dontSendNotification);
    nameLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour)
    );
    nameLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    addAndMakeVisible(nameLabel);

    volumeLabel.setText("Vol", juce::dontSendNotification);
    volumeLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour).withAlpha(0.75f)
    );
    addAndMakeVisible(volumeLabel);

    panLabel.setText("Pan", juce::dontSendNotification);
    panLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour).withAlpha(0.75f)
    );
    addAndMakeVisible(panLabel);

    muteButton.onClick = [this]
    {
        track.setMuted(! track.isMuted());
        refreshMuteButton();
    };
    addAndMakeVisible(muteButton);

    volumeSlider.setRange(0.0, 1.5, 0.01);
    volumeSlider.setValue(track.getGain());
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(
        juce::Slider::TextBoxRight,
        false,
        54,
        22
    );
    volumeSlider.setColour(
        juce::Slider::trackColourId,
        juce::Colour(accentColour)
    );
    volumeSlider.onValueChange = [this]
    {
        track.setGain(
            static_cast<float>(volumeSlider.getValue())
        );
    };
    addAndMakeVisible(volumeSlider);

    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.setValue(track.getPan());
    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle(
        juce::Slider::TextBoxRight,
        false,
        54,
        22
    );
    panSlider.onValueChange = [this]
    {
        track.setPan(
            static_cast<float>(panSlider.getValue())
        );
    };
    addAndMakeVisible(panSlider);

    refreshMuteButton();
}

void TrackRowComponent::paint(juce::Graphics& graphics)
{
    graphics.setColour(juce::Colour(rowColour));
    graphics.fillRoundedRectangle(
        getLocalBounds().reduced(2).toFloat(),
        6.0f
    );
}

void TrackRowComponent::resized()
{
    auto area = getLocalBounds().reduced(10);

    auto left = area.removeFromLeft(190);
    nameLabel.setBounds(left.removeFromTop(32));
    muteButton.setBounds(left.removeFromTop(34).removeFromLeft(54));

    area.removeFromLeft(12);

    auto controls = area;
    auto volumeArea = controls.removeFromTop(34);
    volumeLabel.setBounds(volumeArea.removeFromLeft(40));
    volumeSlider.setBounds(volumeArea);

    controls.removeFromTop(6);

    auto panArea = controls.removeFromTop(34);
    panLabel.setBounds(panArea.removeFromLeft(40));
    panSlider.setBounds(panArea);
}

void TrackRowComponent::refreshMuteButton()
{
    muteButton.setColour(
        juce::TextButton::buttonColourId,
        track.isMuted()
            ? juce::Colour(muteColour)
            : juce::Colour(rowColour).brighter(0.15f)
    );
}
