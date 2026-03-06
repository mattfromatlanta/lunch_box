#include <juce_gui_basics/juce_gui_basics.h>
#include "CLI/CliProcessor.h"
#include "GUI/MainWindow.h"

//==============================================================================
// ChompiPackApplication - JUCE application wrapper
//==============================================================================

class ChompiPackApplication : public juce::JUCEApplication
{
public:
    ChompiPackApplication() {}

    const juce::String getApplicationName() override { return "Chompi Pack"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override
    {
        // Check if CLI arguments were provided
        auto args = getCommandLineParameterArray();

        if (args.size() > 0)
        {
            // CLI mode - process arguments and quit
            runCliMode(args);
            quit();
        }
        else
        {
            // GUI mode - create window
            mainWindow = std::make_unique<MainWindow>(getApplicationName());
        }
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
    }

private:
    std::unique_ptr<MainWindow> mainWindow;

    void runCliMode(const juce::StringArray& args)
    {
        CliProcessor cliProcessor;
        cliProcessor.run(args);
    }
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(ChompiPackApplication)
