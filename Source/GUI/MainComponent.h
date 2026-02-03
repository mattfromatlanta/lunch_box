#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "GuiProcessor.h"

//==============================================================================
// MainComponent - Main GUI component for Chompi Pack
//==============================================================================
// Contains all UI elements:
// - Header label
// - Cubbi folder selection
// - Jammi folder selection
// - Output folder selection
// - Process button
// - Status display
//==============================================================================

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Header label
    juce::Label headerLabel;

    // Cubbi folder selection
    juce::TextButton selectCubbiButton;
    juce::Label cubbiPathLabel;
    juce::File selectedCubbiFolder;

    // Jammi folder selection
    juce::TextButton selectJammiButton;
    juce::Label jammiPathLabel;
    juce::File selectedJammiFolder;

    // Output folder selection
    juce::TextButton selectOutputButton;
    juce::Label outputPathLabel;
    juce::File selectedOutputFolder;

    // Processing
    juce::TextButton processButton;

    // Status/output
    juce::TextEditor statusTextEditor;

    // File chooser
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Processing bridge
    std::unique_ptr<GuiProcessor> processor;

    // Callbacks
    void selectCubbiFolder();
    void selectJammiFolder();
    void selectOutputFolder();
    void processFiles();
    void updateProcessButtonState();
    void appendStatus(const juce::String& message);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
