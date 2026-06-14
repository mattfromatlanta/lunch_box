// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class MainComponent;

//==============================================================================
// AppMenuBar - macOS system menu bar for Lunch Box
//==============================================================================

class AppMenuBar : public juce::MenuBarModel
{
public:
    explicit AppMenuBar(MainComponent* mc);
    ~AppMenuBar() override;

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

private:
    MainComponent* mainComponent;

    enum MenuItemIds
    {
        openOutputFolder = 1,
        showLogFolder    = 4,
        clearStatusLog   = 5,
        showRuntimeLogs  = 6,
        normalizeToggle  = 7,
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppMenuBar)
};
