#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioTrack.h"

class MixerChannelComponent final : public juce::Component
{
public:
    explicit MixerChannelComponent(AudioTrack& trackToControl);

    void paint(juce::Graphics& graphics) override;
    void resized() override;
    void refreshFromTrack();

    std::function<void()> onSoloChanged;

private:
    void refreshButtons();

    AudioTrack& track;
    juce::Label nameLabel;
    juce::Label panLabel { {}, "PAN" };
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };
    juce::Slider gainSlider;
    juce::Slider panSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelComponent)
};

class MixerComponent final : public juce::Component
{
public:
    MixerComponent();

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void addTrack(AudioTrack& track);
    void refresh();
    void setMasterGain(float gain);
    [[nodiscard]] float getMasterGain() const;

    std::function<void(float)> onMasterGainChanged;
    std::function<void()> onSoloChanged;

private:
    juce::Label titleLabel { {}, "MIXER" };
    juce::Label masterLabel { {}, "MASTER" };
    juce::Slider masterSlider;
    juce::Component channelContainer;
    juce::Viewport viewport;
    juce::OwnedArray<MixerChannelComponent> channels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};
