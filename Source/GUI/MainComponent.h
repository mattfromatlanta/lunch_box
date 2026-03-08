#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include "GuiProcessor.h"
#include "PreviewPanel.h"
#include "BankEditorPanel.h"
#include "BankFocusPanel.h"

#if JUCE_DEBUG
 #include <melatonin_inspector/melatonin_inspector.h>
#endif

class ConsoleWindow;

//==============================================================================
// MainComponent - Main GUI component for Chompi Pack
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
    void selectOutputFolder();
    void processFiles();
    void showLogFolder();
    void clearStatusLog();
    void setShowRuntimeLogs(bool shouldShow);
    bool getShowRuntimeLogs() const { return showRuntimeLogs; }
    bool getConsoleVisible()  const { return consoleVisible; }
    void toggleConsole();

private:
    // Header
    juce::Label headerLabel;

    // Mode toggle (Pack / Bank)
    enum class ViewMode { Pack, Bank };
    ViewMode viewMode = ViewMode::Pack;

    juce::TextButton packModeButton;
    juce::TextButton bankModeButton;
    bool consoleVisible = false;
    juce::String     consoleContent { "Ready to process samples...\n" };
    std::unique_ptr<ConsoleWindow> consoleWindow;

    // ── Pack mode ───────────────────────────────────────────
    juce::TextButton cubbiTabButton;
    juce::TextButton jammiTabButton;
    bool showCubbiEditor = true;

    std::unique_ptr<BankEditorPanel> cubbiEditor;
    std::unique_ptr<BankEditorPanel> jammiEditor;

    // ── Bank focus mode ────────────────────────────────────
    std::unique_ptr<BankFocusPanel> bankFocusPanel;
    juce::Label bankStatusLabel;

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
    juce::TextButton fillButton;
    juce::TextButton clearButton;

    PreviewPanel previewPanel;
    BankEditorPanel::Cell playingCell { -1, -1 };  // grid cell whose file is currently previewing

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<GuiProcessor> processor;

    juce::ApplicationProperties appProperties;

    bool showRuntimeLogs = false;

    // Mode switching
    void setViewMode(ViewMode mode);
    void setCategoryTab(bool showCubbi);
    void styleTabButton(juce::TextButton& btn, bool active);

    // Cross-tab data sync
    void syncPackToBankFocus();
    void syncBankFocusToAdvanced();

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

    void handleOutputFolderSelected(juce::File folder);

    // Preview
    void stopPreview();

    // juce::KeyListener
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;

    // Processing
    void processFilesFromEditors();
    void updateProcessButtonState();
    void appendStatus(const juce::String& message);
    void appendProcessingResult(const GuiProcessor::ProcessingResult& result, const juce::File& outputFolder);

#if JUCE_DEBUG
    melatonin::Inspector inspector { *this };
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
