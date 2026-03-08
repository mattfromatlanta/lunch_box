#include "AppMenuBar.h"
#include "MainComponent.h"

AppMenuBar::AppMenuBar(MainComponent* mc) : mainComponent(mc) {}

AppMenuBar::~AppMenuBar()
{
    // Must clear before destruction to avoid dangling pointer in JUCE internals
    juce::MenuBarModel::setMacMainMenu(nullptr);
}

juce::StringArray AppMenuBar::getMenuBarNames()
{
    return { "File", "Settings" };
}

juce::PopupMenu AppMenuBar::getMenuForIndex(int menuIndex, const juce::String&)
{
    juce::PopupMenu menu;

    if (menuIndex == 0) // File
    {
        menu.addItem(openOutputFolder, "Open Output Folder...");
        menu.addSeparator();
        menu.addItem(processSamples,   "Process Samples");
    }
    else if (menuIndex == 1) // Settings
    {
        bool consoleOn     = (mainComponent != nullptr) && mainComponent->getConsoleVisible();
        bool runtimeLogsOn = (mainComponent != nullptr) && mainComponent->getShowRuntimeLogs();
        menu.addItem(toggleConsole,   consoleOn ? "Hide console" : "Show console");
        menu.addItem(showRuntimeLogs, "Show Runtime Logs", true, runtimeLogsOn);
        menu.addSeparator();
        menu.addItem(showLogFolder,   "Show Log Folder in Finder");
        menu.addItem(clearStatusLog,  "Clear Status Log");
    }

    return menu;
}

void AppMenuBar::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/)
{
    if (mainComponent == nullptr) return;

    switch (menuItemID)
    {
        case openOutputFolder:  mainComponent->selectOutputFolder();  break;
        case processSamples:    mainComponent->processFiles();        break;
        case toggleConsole:     mainComponent->toggleConsole(); menuItemsChanged(); break;
        case showLogFolder:     mainComponent->showLogFolder();       break;
        case clearStatusLog:    mainComponent->clearStatusLog();      break;
        case showRuntimeLogs:   mainComponent->setShowRuntimeLogs(!mainComponent->getShowRuntimeLogs()); break;
        default: break;
    }
}
