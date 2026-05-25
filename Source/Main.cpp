// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_gui_basics/juce_gui_basics.h>
#include "CLI/CliProcessor.h"
#include "GUI/Shell/MainWindow.h"
#include "GUI/Shell/AppMenuBar.h"

//==============================================================================
// LunchBoxApplication - JUCE application wrapper
//==============================================================================

class LunchBoxApplication : public juce::JUCEApplication
{
public:
    LunchBoxApplication() {}

    const juce::String getApplicationName() override { return "Lunch Box"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
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

           #if JUCE_MAC
            menuBar = std::make_unique<AppMenuBar>(mainWindow->getMainComponent());
            juce::MenuBarModel::setMacMainMenu(menuBar.get());
           #endif
        }
    }

    void shutdown() override
    {
       #if JUCE_MAC
        juce::MenuBarModel::setMacMainMenu(nullptr);
        menuBar = nullptr;
       #endif
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override
    {
    }

private:
    std::unique_ptr<MainWindow> mainWindow;
   #if JUCE_MAC
    std::unique_ptr<AppMenuBar> menuBar;
   #endif

    void runCliMode(const juce::StringArray& args)
    {
        CliProcessor cliProcessor;
        cliProcessor.run(args);
    }
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(LunchBoxApplication)
