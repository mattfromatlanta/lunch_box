#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include "GuiProcessor.h"
#include "FolderDropZone.h"
#include "PreviewPanel.h"
#include "BankEditorPanel.h"

//==============================================================================
// MainComponent - Main GUI component for Chompi Pack
//==============================================================================
// Dark-themed layout with drag-and-drop folder selection (M7 + M9).
//==============================================================================

class MainComponent : public juce::Component,
                      public juce::KeyListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

    // Public interface for menu bar actions
    void selectCubbiFolder();
    void selectJammiFolder();
    void selectOutputFolder();
    void processFiles();
    void showLogFolder();
    void clearStatusLog();
    void setShowRuntimeLogs(bool shouldShow);
    bool getShowRuntimeLogs() const { return showRuntimeLogs; }

private:
    // Header
    juce::Label headerLabel;

    // Mode toggle (Simple / Advanced)
    juce::TextButton simpleModeButton;
    juce::TextButton advancedModeButton;
    bool isAdvancedMode = false;

    // ── Simple mode ────────────────────────────────────────
    juce::Label cubbiSectionLabel;
    juce::Label jammiSectionLabel;
    juce::Label previewSectionLabel;

    std::unique_ptr<FolderDropZone> cubbiDropZone;
    std::unique_ptr<FolderDropZone> jammiDropZone;

    juce::File selectedCubbiFolder;
    juce::File selectedJammiFolder;

    // ── Advanced mode ──────────────────────────────────────
    juce::TextButton cubbiTabButton;
    juce::TextButton jammiTabButton;
    bool showCubbiEditor = true;

    std::unique_ptr<BankEditorPanel> cubbiEditor;
    std::unique_ptr<BankEditorPanel> jammiEditor;

    // ── Output folder (shared) ────────────────────────────
    juce::Label        outputSectionLabel;
    juce::TextButton   outputParentButton;   // shows base folder; click to browse
    juce::Label        outputSlashLabel;     // "/" separator
    juce::TextEditor   outputNameEditor;     // editable folder name / subpath
    juce::Label        outputPathLabel;      // shows full resolved path
    juce::ToggleButton cleanOutputToggle;    // "Clean folder before export?"
    juce::File         outputBaseFolder;     // ~/Desktop by default

    juce::TextButton processButton;
    juce::TextButton openOutputButton;
    juce::TextEditor statusTextEditor;

    PreviewPanel previewPanel;
    BankEditorPanel::Cell playingCell { -1, -1 };  // grid cell whose file is currently previewing

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<GuiProcessor> processor;

    juce::ApplicationProperties appProperties;

    bool showRuntimeLogs = false;

    // Mode switching
    void setMode(bool advanced);
    void setCategoryTab(bool showCubbi);
    void styleTabButton(juce::TextButton& btn, bool active);

    // Helpers
    juce::File getResolvedOutputFolder();
    void updateOutputPathDisplay();
    void layoutOutputSection(juce::Rectangle<int>& area, int labelH, int itemGap, int sectionGap);
    void prepareOutputFolder(const juce::File& folder);   // clean + create
    void layoutButtonRow(juce::Rectangle<int>& area, int h);

    // Persistent folder preferences
    juce::File getSavedFolder(const juce::String& key);
    void       saveFolder(const juce::String& key, const juce::File& folder);
    void       saveString(const juce::String& key, const juce::String& value);
    juce::String getSavedString(const juce::String& key, const juce::String& fallback = {});

    // Simple mode folder handlers
    void handleCubbiFolderSelected(juce::File folder);
    void handleJammiFolderSelected(juce::File folder);
    void handleOutputFolderSelected(juce::File folder);   // decomposes into base+name
    void selectFolderFor(const juce::String& title, const juce::File& startDir,
                         std::function<void(juce::File)> handler);

    // Preview (advanced mode)
    void stopPreview();

    // juce::KeyListener
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;

    // Processing
    void processFilesAdvanced();
    void updateProcessButtonState();
    void appendStatus(const juce::String& message);

    static int countAudioFiles(const juce::File& folder);
    void previewFirstAudioFile(const juce::File& folder);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
