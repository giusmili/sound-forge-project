#include "AppServices.h"

std::unique_ptr<juce::FileLogger> AppServices::logger;

juce::File AppServices::getDataDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("SonoForge");
}

juce::File AppServices::getConfigFile()
{
    return getDataDirectory().getChildFile("config").getChildFile("settings.json");
}

juce::File AppServices::getLogFile()
{
    return getDataDirectory().getChildFile("logs").getChildFile("sonoforge.log");
}

void AppServices::initialise()
{
    auto dataDirectory = getDataDirectory();
    dataDirectory.createDirectory();
    getConfigFile().getParentDirectory().createDirectory();
    getLogFile().getParentDirectory().createDirectory();

    logger = std::make_unique<juce::FileLogger>(
        getLogFile(),
        "SonoForge Studio diagnostic log",
        0
    );

    juce::Logger::setCurrentLogger(logger.get());
    log("Application starting");
    log("Data directory: " + dataDirectory.getFullPathName());

    if (! getConfigFile().existsAsFile())
    {
        auto defaults = new juce::DynamicObject();
        defaults->setProperty("schemaVersion", 1);
        defaults->setProperty("masterGain", 0.8);
        defaults->setProperty("lastImportDirectory", "");
        defaults->setProperty("audioDevice", "");
        defaults->setProperty("sampleRate", 0.0);
        defaults->setProperty("bufferSize", 0);
        saveSettings(juce::var(defaults));
        log("Default settings created");
    }
    else
    {
        log("Existing settings found");
    }
}

void AppServices::shutdown()
{
    log("Application shutting down");
    juce::Logger::setCurrentLogger(nullptr);
    logger.reset();
}

void AppServices::log(const juce::String& message)
{
    juce::Logger::writeToLog(
        juce::Time::getCurrentTime().toString(true, true, true, true)
        + " | " + message
    );
}

juce::var AppServices::loadSettings()
{
    const auto file = getConfigFile();

    if (! file.existsAsFile())
        return {};

    const auto json = file.loadFileAsString();
    const auto parsed = juce::JSON::parse(json);

    if (parsed.isVoid())
        log("WARNING: settings.json could not be parsed");

    return parsed;
}

bool AppServices::saveSettings(const juce::var& settings)
{
    const auto file = getConfigFile();
    file.getParentDirectory().createDirectory();

    const auto json = juce::JSON::toString(settings, true);
    const auto ok = file.replaceWithText(json);

    if (! ok)
        log("ERROR: unable to write settings.json");

    return ok;
}
