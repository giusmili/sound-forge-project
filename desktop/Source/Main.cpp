#include <juce_gui_extra/juce_gui_extra.h>

#include "MainComponent.h"

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
        return "0.7.0";
    }

    bool moreThanOneInstanceAllowed() override
    {
        return true;
    }

    void initialise(const juce::String&) override
    {
        mainWindow =
            std::make_unique<MainWindow>(
                getApplicationName()
            );
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
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
            centreWithSize(
                getWidth(),
                getHeight()
            );
            setVisible(true);
        }

        void closeButtonPressed() override
        {
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

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(SonoForgeApplication)
