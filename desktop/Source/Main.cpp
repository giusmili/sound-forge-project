#include <juce_gui_extra/juce_gui_extra.h>

#include "MainComponent.h"
#include "AppServices.h"
#include "SonoLookAndFeel.h"

class SonoForgeApplication final
    : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override
    {
        return "SonoForge Studio";
    }

    const juce::String getApplicationVersion() override
    {
        return "0.8.2";
    }

    bool moreThanOneInstanceAllowed() override
    {
        return true;
    }

    void initialise(const juce::String& commandLine) override
    {
        AppServices::initialise();
        juce::SystemStats::setApplicationCrashHandler(
            [](void*)
            {
                AppServices::log(
                    juce::String("CRASH:\n")
                    +juce::SystemStats::getStackBacktrace()
                );
            }
        );
        juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
        AppServices::log("Creating SonoForge 0.8.2 main window");
        mainWindow =
            std::make_unique<MainWindow>(
                getApplicationName()
            );

        // Optional: SonoForge.exe file.wav imports the file at startup.
        juce::Array<juce::File> startupFiles;
        for (const auto& arg : getCommandLineParameterArray())
            if (juce::File(arg).existsAsFile())
                startupFiles.add(juce::File(arg));

        if (auto* main = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
        {
            if (! startupFiles.isEmpty())
                main->importAudioFiles(startupFiles);

            // Test aid: SonoForge.exe file.wav --play starts playback right away.
            if (commandLine.contains("--play"))
                main->startPlayback();
        }
    }

    void shutdown() override
    {
        AppServices::log("Closing SonoForge main window");
        mainWindow.reset();
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
        AppServices::shutdown();
    }

    void systemRequestedQuit() override
    {
        AppServices::log("systemRequestedQuit (OS / session close request)");
        if (mainWindow != nullptr)
        {
            mainWindow->requestSafeQuit();
            return;
        }

        quit();
    }

private:
    class MainWindow final
        : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(
            const juce::String& name
        )
            : DocumentWindow(
                  name,
                  juce::Colour(0xff111318),
                  DocumentWindow::allButtons
              )
        {
            setUsingNativeTitleBar(true);
            setContentOwned(
                new MainComponent(),
                true
            );
            setResizable(true, true);
            auto initialWidth = getWidth();
            auto initialHeight = getHeight();
            if (const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            {
                // JUCE uses logical pixels. Leave room for native window borders
                // and the taskbar, including when Windows display scaling is active.
                initialWidth = juce::jmin(initialWidth, juce::jmax(1, display->userArea.getWidth() - 40));
                initialHeight = juce::jmin(initialHeight, juce::jmax(1, display->userArea.getHeight() - 60));
            }
            centreWithSize(
                initialWidth,
                initialHeight
            );
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            AppServices::log("Window close button pressed");
            requestSafeQuit();
        }

        void requestSafeQuit()
        {
            if (auto* mainComponent =
                    dynamic_cast<MainComponent*>(
                        getContentComponent()
                    ))
            {
                mainComponent->requestApplicationQuit(
                    []
                    {
                        if (auto* application =
                                juce::JUCEApplication::getInstance())
                        {
                            application->quit();
                        }
                    }
                );
                return;
            }

            if (auto* application =
                    juce::JUCEApplication::getInstance())
            {
                application->quit();
            }
        }
    };

    SonoLookAndFeel lookAndFeel;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(SonoForgeApplication)
