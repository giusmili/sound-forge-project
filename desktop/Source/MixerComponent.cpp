#include "MixerComponent.h"

namespace
{
constexpr auto backgroundColour = 0xff15191f;
constexpr auto stripColour = 0xff222831;
constexpr auto borderColour = 0xff39424e;
constexpr auto textColour = 0xffe8edf3;
constexpr auto accentColour = 0xff5aa9ff;
constexpr auto muteColour = 0xffd65454;
constexpr auto soloColour = 0xffd4a62a;
constexpr int channelWidth = 112;
}

MixerChannelComponent::MixerChannelComponent(AudioTrack& trackToControl)
    : track(trackToControl)
{
    nameLabel.setText(track.getName(), juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, juce::Colour(textColour));
    nameLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    addAndMakeVisible(nameLabel);

    panLabel.setJustificationType(juce::Justification::centred);
    panLabel.setColour(juce::Label::textColourId, juce::Colour(textColour).withAlpha(0.65f));
    addAndMakeVisible(panLabel);

    muteButton.onClick = [this]
    {
        track.setMuted(! track.isMuted());
        refreshButtons();
    };
    addAndMakeVisible(muteButton);

    soloButton.onClick = [this]
    {
        track.setSolo(! track.isSolo());
        refreshButtons();
        if (onSoloChanged)
            onSoloChanged();
    };
    addAndMakeVisible(soloButton);

    gainSlider.setRange(0.0, 1.5, 0.01);
    gainSlider.setSliderStyle(juce::Slider::LinearVertical);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 22);
    gainSlider.setColour(juce::Slider::trackColourId, juce::Colour(accentColour));
    gainSlider.onValueChange = [this]
    {
        track.setGain(static_cast<float>(gainSlider.getValue()));
    };
    addAndMakeVisible(gainSlider);

    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 20);
    panSlider.onValueChange = [this]
    {
        track.setPan(static_cast<float>(panSlider.getValue()));
    };
    addAndMakeVisible(panSlider);

    refreshFromTrack();
}

void MixerChannelComponent::paint(juce::Graphics& graphics)
{
    graphics.setColour(juce::Colour(stripColour));
    graphics.fillRoundedRectangle(getLocalBounds().reduced(3).toFloat(), 6.0f);
    graphics.setColour(juce::Colour(borderColour));
    graphics.drawRoundedRectangle(getLocalBounds().reduced(3).toFloat(), 6.0f, 1.0f);

    auto meterArea = getLocalBounds().reduced(8).withTrimmedTop(184).withTrimmedBottom(28).removeFromRight(18);
    auto leftMeter = meterArea.removeFromLeft(7);
    meterArea.removeFromLeft(2);
    auto rightMeter = meterArea.removeFromLeft(7);
    graphics.setColour(juce::Colour(0xff0b0d10));
    graphics.fillRect(leftMeter);
    graphics.fillRect(rightMeter);
    const auto drawMeter = [&graphics](juce::Rectangle<int> meter, const float level)
    {
        const auto h = static_cast<int>(level * static_cast<float>(meter.getHeight()));
        auto lit = meter.removeFromBottom(h);
        graphics.setColour(level > 0.92f ? juce::Colour(0xffe35d5d) : (level > 0.72f ? juce::Colour(0xffe0b84f) : juce::Colour(0xff55c878)));
        graphics.fillRect(lit);
    };
    drawMeter(leftMeter, displayPeakLeft);
    drawMeter(rightMeter, displayPeakRight);
}

void MixerChannelComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    nameLabel.setBounds(area.removeFromTop(32));
    auto buttons = area.removeFromTop(28);
    muteButton.setBounds(buttons.removeFromLeft(42));
    buttons.removeFromLeft(6);
    soloButton.setBounds(buttons.removeFromLeft(42));
    area.removeFromTop(6);
    panLabel.setBounds(area.removeFromTop(18));
    panSlider.setBounds(area.removeFromTop(72));
    area.removeFromTop(4);
    gainSlider.setBounds(area);
}

void MixerChannelComponent::refreshFromTrack()
{
    if (! gainSlider.isMouseButtonDown())
        gainSlider.setValue(track.getGain(), juce::dontSendNotification);
    if (! panSlider.isMouseButtonDown())
        panSlider.setValue(track.getPan(), juce::dontSendNotification);
    displayPeakLeft = juce::jmax(track.getPeakLeft(), displayPeakLeft * 0.82f);
    displayPeakRight = juce::jmax(track.getPeakRight(), displayPeakRight * 0.82f);
    refreshButtons();
    repaint();
}

void MixerChannelComponent::refreshButtons()
{
    muteButton.setColour(
        juce::TextButton::buttonColourId,
        track.isMuted() ? juce::Colour(muteColour) : juce::Colour(stripColour).brighter(0.18f)
    );
    soloButton.setColour(
        juce::TextButton::buttonColourId,
        track.isSolo() ? juce::Colour(soloColour) : juce::Colour(stripColour).brighter(0.18f)
    );
}

MixerComponent::MixerComponent()
{
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(textColour));
    titleLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    masterLabel.setColour(juce::Label::textColourId, juce::Colour(textColour));
    masterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterLabel);

    masterSlider.setRange(0.0, 1.0, 0.01);
    masterSlider.setValue(0.8);
    masterSlider.setSliderStyle(juce::Slider::LinearVertical);
    masterSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 22);
    masterSlider.setColour(juce::Slider::trackColourId, juce::Colour(accentColour));
    masterSlider.onValueChange = [this]
    {
        if (onMasterGainChanged)
            onMasterGainChanged(static_cast<float>(masterSlider.getValue()));
    };
    addAndMakeVisible(masterSlider);

    viewport.setViewedComponent(&channelContainer, false);
    viewport.setScrollBarsShown(false, true);
    addAndMakeVisible(viewport);
}

void MixerComponent::paint(juce::Graphics& graphics)
{
    graphics.setColour(juce::Colour(backgroundColour));
    graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 7.0f);
}

void MixerComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    titleLabel.setBounds(area.removeFromTop(26));

    auto masterArea = area.removeFromRight(100);
    masterLabel.setBounds(masterArea.removeFromTop(24));
    masterSlider.setBounds(masterArea.reduced(8, 2));

    area.removeFromRight(6);
    viewport.setBounds(area);

    const auto width = juce::jmax(viewport.getWidth(), channels.size() * channelWidth);
    channelContainer.setSize(width, viewport.getHeight());

    int x = 0;
    for (auto* channel : channels)
    {
        channel->setBounds(x, 0, channelWidth, channelContainer.getHeight());
        x += channelWidth;
    }
}

void MixerComponent::addTrack(AudioTrack& track)
{
    auto* channel = channels.add(new MixerChannelComponent(track));
    channel->onSoloChanged = [this]
    {
        if (onSoloChanged)
            onSoloChanged();
    };
    channelContainer.addAndMakeVisible(channel);
    resized();
}

void MixerComponent::refresh()
{
    for (auto* channel : channels)
        channel->refreshFromTrack();
}

void MixerComponent::setMasterGain(const float gain)
{
    if (! masterSlider.isMouseButtonDown())
        masterSlider.setValue(gain, juce::dontSendNotification);
}

float MixerComponent::getMasterGain() const
{
    return static_cast<float>(masterSlider.getValue());
}
