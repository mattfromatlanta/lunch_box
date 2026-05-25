// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MainComponent.h"

//==============================================================================
// MainWindow - Main application window for Lunch Box GUI
//==============================================================================

class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(juce::String name);
    ~MainWindow() override;

    void closeButtonPressed() override;

    MainComponent* getMainComponent() { return mainComponent.get(); }

private:
    std::unique_ptr<MainComponent> mainComponent;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
