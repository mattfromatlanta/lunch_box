#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "GuiProcessor.h"
#include "FolderDropZone.h"
#include "PreviewPanel.h"

//==============================================================================
// MainComponent - Main GUI component for Chompi Pack
//==============================================================================
// Dark-themed layout with drag-and-drop folder selection (M7 + M9).
//==============================================================================

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Header
    juce::Label headerLabel;

    // Section labels (M9 typography)
    juce::Label cubbiSectionLabel;
    juce::Label jammiSectionLabel;
    juce::Label outputSectionLabel;
    juce::Label previewSectionLabel;

    // Folder drop zones (M7 drag-drop)
    std::unique_ptr<FolderDropZone> cubbiDropZone;
    std::unique_ptr<FolderDropZone> jammiDropZone;
    std::unique_ptr<FolderDropZone> outputDropZone;

    // Currently selected folders
    juce::File selectedCubbiFolder;
    juce::File selectedJammiFolder;
    juce::File selectedOutputFolder;

    // Process button and status
    juce::TextButton processButton;
    juce::TextEditor statusTextEditor;

    // Sample preview (M10)
    PreviewPanel previewPanel;

    // File chooser (kept alive through async callback)
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Processing bridge
    std::unique_ptr<GuiProcessor> processor;

    // Unified folder selection handlers (called by both button and drag-drop)
    void handleCubbiFolderSelected(juce::File folder);
    void handleJammiFolderSelected(juce::File folder);
    void handleOutputFolderSelected(juce::File folder);

    // File browser launchers (called by drop zone buttons)
    void selectCubbiFolder();
    void selectJammiFolder();
    void selectOutputFolder();

    void processFiles();
    void updateProcessButtonState();
    void appendStatus(const juce::String& message);

    // Count all supported audio files in a folder
    static int countAudioFiles(const juce::File& folder);

    // Auto-preview the first audio file found in a folder
    void previewFirstAudioFile(const juce::File& folder);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
