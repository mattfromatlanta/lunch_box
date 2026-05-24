// SPDX-License-Identifier: AGPL-3.0-or-later
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
    return { "File", "Edit", "Settings" };
}

juce::PopupMenu AppMenuBar::getMenuForIndex(int menuIndex, const juce::String&)
{
    juce::PopupMenu menu;

    if (menuIndex == 0) // File
    {
        menu.addItem(openOutputFolder, "Change Output Folder...");
        if (mainComponent != nullptr)
            menu.addCommandItem(&mainComponent->commandManager, MainComponent::cmdOpenOutput);
        menu.addSeparator();
        if (mainComponent != nullptr)
            menu.addCommandItem(&mainComponent->commandManager, MainComponent::cmdProcess);
    }
    else if (menuIndex == 1) // Edit — use addCommandItem so JUCE wires NSMenuItem key equivalents
    {
        if (mainComponent != nullptr)
        {
            menu.addCommandItem(&mainComponent->commandManager, MainComponent::cmdUndo);
            menu.addCommandItem(&mainComponent->commandManager, MainComponent::cmdRedo);
            menu.addSeparator();
            menu.addCommandItem(&mainComponent->commandManager, MainComponent::cmdCopy);
            menu.addCommandItem(&mainComponent->commandManager, MainComponent::cmdCut);
            menu.addCommandItem(&mainComponent->commandManager, MainComponent::cmdPaste);
            menu.addSeparator();
            menu.addCommandItem(&mainComponent->commandManager, MainComponent::cmdSelectAll);
        }
    }
    else if (menuIndex == 2) // Settings
    {
        bool runtimeLogsOn = (mainComponent != nullptr) && mainComponent->getShowRuntimeLogs();

        if (mainComponent != nullptr)
            menu.addCommandItem(&mainComponent->commandManager, MainComponent::cmdToggleConsole);

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

    // Note: items added via addCommandItem are handled by the ApplicationCommandTarget
    // (MainComponent::perform) and never reach menuItemSelected.
    switch (menuItemID)
    {
        case openOutputFolder:  mainComponent->selectOutputFolder();  break;
        case showLogFolder:     mainComponent->showLogFolder();       break;
        case clearStatusLog:    mainComponent->clearStatusLog();      break;
        case showRuntimeLogs:   mainComponent->setShowRuntimeLogs(!mainComponent->getShowRuntimeLogs()); break;
        default: break;
    }
}
