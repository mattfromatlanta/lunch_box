#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class MainComponent;

//==============================================================================
// AppMenuBar - macOS system menu bar for Chompi Pack
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
        openCubbiFolder  = 1,
        openJammiFolder  = 2,
        openOutputFolder = 3,
        processSamples   = 4,
        showLogFolder    = 5,
        clearStatusLog   = 6,
        showRuntimeLogs  = 7,
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppMenuBar)
};
