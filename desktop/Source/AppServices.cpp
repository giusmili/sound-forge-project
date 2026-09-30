#include "AppServices.h"
#include <cmath>

std::unique_ptr<juce::FileLogger> AppServices::logger;
juce::File AppServices::dataDirectoryOverride;

juce::File AppServices::getDataDirectory()
{
    if (dataDirectoryOverride != juce::File {})
        return dataDirectoryOverride;
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

void AppServices::initialise(const juce::File& overrideDirectory)
{
    dataDirectoryOverride = overrideDirectory;
    auto dataDirectory = getDataDirectory();
    dataDirectory.createDirectory();
    getConfigFile().getParentDirectory().createDirectory();
    getLogFile().getParentDirectory().createDirectory();

    logger = std::make_unique<juce::FileLogger>(
        getLogFile(),
        "SonoForge Studio diagnostic log",
        256 * 1024
    );

    juce::Logger::setCurrentLogger(logger.get());
    log("Application starting");
    log("Data directory: " + dataDirectory.getFullPathName());

    saveSettings(loadSettings());
}

void AppServices::shutdown()
{
    log("Application shutting down");
    juce::Logger::setCurrentLogger(nullptr);
    logger.reset();
    dataDirectoryOverride = juce::File {};
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

    auto parsed = juce::JSON::parse(file.loadFileAsString());
    if (parsed.getDynamicObject() == nullptr)
    {
        if (file.existsAsFile())
        {
            const auto backup = file.getSiblingFile("settings.invalid.json");
            file.copyFileTo(backup);
            log("WARNING: invalid settings; defaults restored, backup: " + backup.getFullPathName());
        }
        parsed = juce::var(new juce::DynamicObject());
    }
    auto* object = parsed.getDynamicObject();
    auto gain = object->getProperty("masterGain");
    const auto numericGain = static_cast<double>(gain);
    if (! (gain.isDouble() || gain.isInt() || gain.isInt64())
        || ! std::isfinite(numericGain) || numericGain < 0.0 || numericGain > 1.0)
        object->setProperty("masterGain", 0.8);
    object->setProperty("schemaVersion", 1);
    for (const auto* key : { "lastImportDirectory", "audioDevice", "audioDeviceState" })
        if (! object->getProperty(key).isString())
            object->setProperty(key, "");
    if (! object->hasProperty("sampleRate")) object->setProperty("sampleRate", 0.0);
    if (! object->hasProperty("bufferSize")) object->setProperty("bufferSize", 0);
    return parsed;
}

bool AppServices::saveSettings(const juce::var& settings)
{
    const auto file = getConfigFile();
    file.getParentDirectory().createDirectory();

    const auto json = juce::JSON::toString(settings, true);
    juce::TemporaryFile temporary(file);
    const auto ok = settings.getDynamicObject() != nullptr
        && temporary.getFile().replaceWithText(json)
        && temporary.overwriteTargetFileWithTemporary();

    if (! ok)
        log("ERROR: unable to write settings.json");

    return ok;
}
