#include "MixerPanelComponent.h"

namespace
{
constexpr auto panelColour = 0xff15191f;
constexpr auto stripColour = 0xff222831;
constexpr auto masterColour = 0xff20252d;
constexpr auto textColour = 0xffe8edf3;
constexpr auto audioAccentColour = 0xff5aa9ff;
constexpr auto midiAccentColour = 0xffb58cff;
constexpr auto meterBackColour = 0xff0d1014;
constexpr auto meterColour = 0xff69c17d;
constexpr auto meterHotColour = 0xffe7b34d;
constexpr auto meterClipColour = 0xffd65454;
constexpr auto muteColour = 0xffd65454;
constexpr auto soloColour = 0xffd4a62a;
constexpr int stripWidth = 124;
constexpr int stripGap = 6;

void drawMeter(
    juce::Graphics& graphics,
    juce::Rectangle<int> bounds,
    const float rawLevel
)
{
    graphics.setColour(juce::Colour(meterBackColour));
    graphics.fillRoundedRectangle(bounds.toFloat(), 2.0f);

    const auto level =
        juce::jlimit(0.0f, 1.0f, rawLevel);

    auto fill = bounds.reduced(2);
    const auto filledHeight =
        juce::roundToInt(
            level
            * static_cast<float>(fill.getHeight())
        );

    fill.removeFromTop(
        juce::jmax(
            0,
            fill.getHeight() - filledHeight
        )
    );

    auto colour = juce::Colour(meterColour);

    if (level >= 0.96f)
        colour = juce::Colour(meterClipColour);
    else if (level >= 0.78f)
        colour = juce::Colour(meterHotColour);

    graphics.setColour(colour);
    graphics.fillRoundedRectangle(fill.toFloat(), 1.5f);
}

class AudioMixerStrip final
    : public juce::Component,
      private juce::Timer
{
public:
    AudioMixerStrip(
        AudioTrack& trackToControl,
        AudioEngine& engineToControl,
        std::function<void()> changedCallback
    )
        : track(trackToControl),
          engine(engineToControl),
          changed(std::move(changedCallback))
    {
        nameLabel.setText(track.getName(), juce::dontSendNotification);
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setColour(juce::Label::textColourId, juce::Colour(textColour));
        nameLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        addAndMakeVisible(nameLabel);

        typeLabel.setText("AUDIO", juce::dontSendNotification);
        typeLabel.setJustificationType(juce::Justification::centred);
        typeLabel.setColour(juce::Label::textColourId, juce::Colour(audioAccentColour));
        typeLabel.setFont(juce::FontOptions(10.0f));
        addAndMakeVisible(typeLabel);

        gainSlider.setRange(0.0, 1.5, 0.01);
        gainSlider.setValue(track.getGain());
        gainSlider.setSliderStyle(juce::Slider::LinearVertical);
        gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 18);
        gainSlider.setColour(juce::Slider::trackColourId, juce::Colour(audioAccentColour));
        gainSlider.onValueChange = [this]
        {
            track.setGain(static_cast<float>(gainSlider.getValue()));
            notifyChanged();
        };
        addAndMakeVisible(gainSlider);

        panSlider.setRange(-1.0, 1.0, 0.01);
        panSlider.setValue(track.getPan());
        panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
        panSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(audioAccentColour));
        panSlider.onValueChange = [this]
        {
            track.setPan(static_cast<float>(panSlider.getValue()));
            notifyChanged();
        };
        addAndMakeVisible(panSlider);

        muteButton.onClick = [this]
        {
            track.setMuted(! track.isMuted());
            refreshButtons();
            notifyChanged();
        };
        addAndMakeVisible(muteButton);

        soloButton.onClick = [this]
        {
            track.setSolo(! track.isSolo());
            engine.refreshSoloState();
            refreshButtons();
            notifyChanged();
        };
        addAndMakeVisible(soloButton);

        refreshButtons();
        startTimerHz(30);
    }

    ~AudioMixerStrip() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.setColour(juce::Colour(stripColour));
        graphics.fillRoundedRectangle(
            getLocalBounds().reduced(2).toFloat(),
            5.0f
        );
        drawMeter(graphics, meterBounds, track.getPeakLevel());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(7, 6);
        nameLabel.setBounds(area.removeFromTop(20));
        typeLabel.setBounds(area.removeFromTop(14));

        auto buttons = area.removeFromBottom(27);
        muteButton.setBounds(
            buttons.removeFromLeft(buttons.getWidth() / 2).reduced(2)
        );
        soloButton.setBounds(buttons.reduced(2));

        auto panArea = area.removeFromBottom(62);
        panSlider.setBounds(panArea.withSizeKeepingCentre(58, 58));

        meterBounds = area.removeFromRight(14).reduced(2, 7);
        gainSlider.setBounds(area.reduced(3, 0));
    }

private:
    void timerCallback() override
    {
        gainSlider.setValue(track.getGain(), juce::dontSendNotification);
        panSlider.setValue(track.getPan(), juce::dontSendNotification);
        refreshButtons();
        repaint(meterBounds);
    }

    void refreshButtons()
    {
        muteButton.setColour(
            juce::TextButton::buttonColourId,
            track.isMuted()
                ? juce::Colour(muteColour)
                : juce::Colour(stripColour).brighter(0.16f)
        );
        soloButton.setColour(
            juce::TextButton::buttonColourId,
            track.isSolo()
                ? juce::Colour(soloColour)
                : juce::Colour(stripColour).brighter(0.16f)
        );
    }

    void notifyChanged()
    {
        if (changed)
            changed();
    }

    AudioTrack& track;
    AudioEngine& engine;
    std::function<void()> changed;
    juce::Label nameLabel;
    juce::Label typeLabel;
    juce::Slider gainSlider;
    juce::Slider panSlider;
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };
    juce::Rectangle<int> meterBounds;
};

class MidiMixerStrip final
    : public juce::Component,
      private juce::Timer
{
public:
    MidiMixerStrip(
        MidiInstrumentTrack& trackToControl,
        std::function<void()> changedCallback
    )
        : track(trackToControl),
          changed(std::move(changedCallback))
    {
        nameLabel.setText(track.getName(), juce::dontSendNotification);
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setColour(juce::Label::textColourId, juce::Colour(textColour));
        nameLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        addAndMakeVisible(nameLabel);

        typeLabel.setText("MIDI / SYNTH", juce::dontSendNotification);
        typeLabel.setJustificationType(juce::Justification::centred);
        typeLabel.setColour(juce::Label::textColourId, juce::Colour(midiAccentColour));
        typeLabel.setFont(juce::FontOptions(10.0f));
        addAndMakeVisible(typeLabel);

        instrumentLabel.setText("Basic Synth", juce::dontSendNotification);
        instrumentLabel.setJustificationType(juce::Justification::centred);
        instrumentLabel.setColour(
            juce::Label::textColourId,
            juce::Colour(textColour).withAlpha(0.7f)
        );
        instrumentLabel.setFont(juce::FontOptions(10.0f));
        addAndMakeVisible(instrumentLabel);

        gainSlider.setRange(0.0, 1.5, 0.01);
        gainSlider.setValue(track.getGain());
        gainSlider.setSliderStyle(juce::Slider::LinearVertical);
        gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 18);
        gainSlider.setColour(juce::Slider::trackColourId, juce::Colour(midiAccentColour));
        gainSlider.onValueChange = [this]
        {
            track.setGain(static_cast<float>(gainSlider.getValue()));
            notifyChanged();
        };
        addAndMakeVisible(gainSlider);

        muteButton.onClick = [this]
        {
            track.setMuted(! track.isMuted());
            refreshMuteButton();
            notifyChanged();
        };
        addAndMakeVisible(muteButton);

        refreshMuteButton();
        startTimerHz(30);
    }

    ~MidiMixerStrip() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.setColour(juce::Colour(stripColour));
        graphics.fillRoundedRectangle(
            getLocalBounds().reduced(2).toFloat(),
            5.0f
        );
        drawMeter(graphics, meterBounds, track.getPeakLevel());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(7, 6);
        nameLabel.setBounds(area.removeFromTop(20));
        typeLabel.setBounds(area.removeFromTop(14));
        instrumentLabel.setBounds(area.removeFromBottom(18));
        muteButton.setBounds(area.removeFromBottom(28).reduced(2));

        meterBounds = area.removeFromRight(14).reduced(2, 7);
        gainSlider.setBounds(area.reduced(3, 0));
    }

private:
    void timerCallback() override
    {
        gainSlider.setValue(track.getGain(), juce::dontSendNotification);
        refreshMuteButton();
        repaint(meterBounds);
    }

    void refreshMuteButton()
    {
        muteButton.setColour(
            juce::TextButton::buttonColourId,
            track.isMuted()
                ? juce::Colour(muteColour)
                : juce::Colour(stripColour).brighter(0.16f)
        );
    }

    void notifyChanged()
    {
        if (changed)
            changed();
    }

    MidiInstrumentTrack& track;
    std::function<void()> changed;
    juce::Label nameLabel;
    juce::Label typeLabel;
    juce::Label instrumentLabel;
    juce::Slider gainSlider;
    juce::TextButton muteButton { "M" };
    juce::Rectangle<int> meterBounds;
};

class MasterMixerStrip final
    : public juce::Component,
      private juce::Timer
{
public:
    MasterMixerStrip(
        AudioEngine& engineToControl,
        std::function<void()> changedCallback
    )
        : engine(engineToControl),
          changed(std::move(changedCallback))
    {
        nameLabel.setText("MASTER", juce::dontSendNotification);
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setColour(juce::Label::textColourId, juce::Colour(textColour));
        nameLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        addAndMakeVisible(nameLabel);

        typeLabel.setText("OUTPUT", juce::dontSendNotification);
        typeLabel.setJustificationType(juce::Justification::centred);
        typeLabel.setColour(juce::Label::textColourId, juce::Colour(meterColour));
        typeLabel.setFont(juce::FontOptions(10.0f));
        addAndMakeVisible(typeLabel);

        gainSlider.setRange(0.0, 1.0, 0.01);
        gainSlider.setValue(engine.getMasterGain());
        gainSlider.setSliderStyle(juce::Slider::LinearVertical);
        gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 18);
        gainSlider.setColour(juce::Slider::trackColourId, juce::Colour(meterColour));
        gainSlider.onValueChange = [this]
        {
            engine.setMasterGain(static_cast<float>(gainSlider.getValue()));
            if (changed)
                changed();
        };
        addAndMakeVisible(gainSlider);

        startTimerHz(30);
    }

    ~MasterMixerStrip() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.setColour(juce::Colour(masterColour));
        graphics.fillRoundedRectangle(
            getLocalBounds().reduced(2).toFloat(),
            5.0f
        );
        drawMeter(graphics, meterBounds, engine.getMasterPeakLevel());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(7, 6);
        nameLabel.setBounds(area.removeFromTop(20));
        typeLabel.setBounds(area.removeFromTop(14));
        area.removeFromBottom(28);
        meterBounds = area.removeFromRight(14).reduced(2, 7);
        gainSlider.setBounds(area.reduced(3, 0));
    }

private:
    void timerCallback() override
    {
        gainSlider.setValue(engine.getMasterGain(), juce::dontSendNotification);
        repaint(meterBounds);
    }

    AudioEngine& engine;
    std::function<void()> changed;
    juce::Label nameLabel;
    juce::Label typeLabel;
    juce::Slider gainSlider;
    juce::Rectangle<int> meterBounds;
};
}

MixerPanelComponent::MixerPanelComponent(
    AudioEngine& engineToControl
)
    : engine(engineToControl)
{
    setOpaque(true);

    viewport.setViewedComponent(&stripContainer, false);
    viewport.setScrollBarsShown(false, true);
    addAndMakeVisible(viewport);

    syncWithEngine();
}

void MixerPanelComponent::paint(
    juce::Graphics& graphics
)
{
    graphics.fillAll(juce::Colour(panelColour));

    graphics.setColour(
        juce::Colour(textColour).withAlpha(0.8f)
    );
    graphics.setFont(
        juce::FontOptions(13.0f, juce::Font::bold)
    );
    graphics.drawText(
        "MIXER",
        10,
        4,
        80,
        20,
        juce::Justification::centredLeft
    );
}

void MixerPanelComponent::resized()
{
    auto area = getLocalBounds().reduced(6);
    area.removeFromTop(22);
    viewport.setBounds(area);
    layoutStrips();
}

void MixerPanelComponent::syncWithEngine()
{
    auto audioTracks = engine.getTrackPointers();
    auto midiTracks = engine.getMidiTrackPointers();

    if (audioTracks == cachedAudioTracks
        && midiTracks == cachedMidiTracks
        && ! strips.isEmpty())
    {
        return;
    }

    cachedAudioTracks = audioTracks;
    cachedMidiTracks = midiTracks;
    strips.clear(true);

    auto changedCallback = [this]
    {
        if (onChanged)
            onChanged();
    };

    for (auto* track : audioTracks)
    {
        if (track == nullptr)
            continue;

        auto* strip = strips.add(
            new AudioMixerStrip(*track, engine, changedCallback)
        );
        stripContainer.addAndMakeVisible(strip);
    }

    for (auto* track : midiTracks)
    {
        if (track == nullptr)
            continue;

        auto* strip = strips.add(
            new MidiMixerStrip(*track, changedCallback)
        );
        stripContainer.addAndMakeVisible(strip);
    }

    auto* masterStrip = strips.add(
        new MasterMixerStrip(engine, changedCallback)
    );
    stripContainer.addAndMakeVisible(masterStrip);

    layoutStrips();
}

void MixerPanelComponent::layoutStrips()
{
    const auto visibleWidth =
        juce::jmax(1, viewport.getWidth());

    const auto contentWidth =
        juce::jmax(
            visibleWidth,
            stripGap
                + strips.size()
                    * (stripWidth + stripGap)
        );

    const auto contentHeight =
        juce::jmax(120, viewport.getHeight());

    stripContainer.setSize(contentWidth, contentHeight);

    int x = stripGap;

    for (auto* strip : strips)
    {
        strip->setBounds(
            x,
            0,
            stripWidth,
            contentHeight
        );
        x += stripWidth + stripGap;
    }
}
