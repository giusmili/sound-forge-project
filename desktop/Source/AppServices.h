#pragma once

#include <juce_core/juce_core.h>

class AppServices final
{
public:
    static void initialise(const juce::File& dataDirectory = {});
    static void shutdown();

    static void log(const juce::String& message);
    static juce::File getDataDirectory();
    static juce::File getConfigFile();
    static juce::File getLogFile();

    static juce::var loadSettings();
    static bool saveSettings(const juce::var& settings);

private:
    static std::unique_ptr<juce::FileLogger> logger;
    static juce::File dataDirectoryOverride;
};
