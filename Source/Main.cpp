#include <JuceHeader.h>
#include "MainComponent.h"

class MusicMakerApplication : public juce::JUCEApplication
{
public:
    MusicMakerApplication() {}

    const juce::String getApplicationName() override       { return "Music Maker"; }
    const juce::String getApplicationVersion() override    { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String& commandLine) override
    {
        auto logFile = juce::File ("C:\\music_maker\\debug_log.txt");
        if (logFile.exists()) logFile.deleteFile();
        logger = std::make_unique<juce::FileLogger> (logFile, "Music Maker Log");
        juce::Logger::setCurrentLogger (logger.get());
        
        juce::Logger::writeToLog ("--- Application Starting ---");
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
        juce::Logger::setCurrentLogger (nullptr);
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override {}

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name)
            : DocumentWindow (name,
                              juce::Desktop::getInstance().getDefaultLookAndFeel()
                                                          .findColour (juce::ResizableWindow::backgroundColourId),
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
           #else
            setResizable (true, true);
            centreWithSize (getWidth(), getHeight());
           #endif

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<juce::FileLogger> logger;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (MusicMakerApplication)