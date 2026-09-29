#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "MidiInstrumentTrack.h"

class MidiTrackRowComponent final
    : public juce::Component,
      private juce::Timer
{
public:
    explicit MidiTrackRowComponent(
        MidiInstrumentTrack& trackToControl
    );

    ~MidiTrackRowComponent() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    [[nodiscard]] MidiInstrumentTrack& getTrack() noexcept;
    void syncControlsFromTrack();

    std::function<void()> onChanged;
    std::function<void(MidiTrackRowComponent*)>
        onDeleteRequested;

private:
    void timerCallback() override;
    void refreshMuteButton();

    MidiInstrumentTrack& track;

    juce::Label nameLabel;
    juce::Label typeLabel;
    juce::Label instrumentLabel;
    juce::Label volumeLabel;

    juce::TextButton muteButton { "M" };
    juce::TextButton testButton { "Tester C4" };
    juce::TextButton deleteButton { "Supprimer" };
    juce::Slider volumeSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        MidiTrackRowComponent
    )
};
