#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioEngine.h"

class MixerPanelComponent final : public juce::Component
{
public:
    explicit MixerPanelComponent(AudioEngine& engineToControl);
    ~MixerPanelComponent() override = default;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void syncWithEngine();

    std::function<void()> onChanged;

private:
    void layoutStrips();

    AudioEngine& engine;
    juce::Viewport viewport;
    juce::Component stripContainer;
    juce::OwnedArray<juce::Component> strips;

    std::vector<AudioTrack*> cachedAudioTracks;
    std::vector<MidiInstrumentTrack*> cachedMidiTracks;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        MixerPanelComponent
    )
};
