#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include "GuiProcessor.h"
#include "PreviewPanel.h"
#include "BankEditorPanel.h"
#include "BankFocusPanel.h"

class ConsoleWindow;

//==============================================================================
// MainComponent - Main GUI component for Chompi Pack
//==============================================================================

class MainComponent : public juce::Component,
                      public juce::KeyListener,
                      public juce::ApplicationCommandTarget
{
public:
    // Command IDs — used by AppMenuBar to wire menu items with keyboard shortcuts
    enum CommandIDs
    {
        cmdCopy          = 0x2001,
        cmdCut           = 0x2002,
        cmdPaste         = 0x2003,
        cmdToggleConsole = 0x2004,
        cmdUndo          = 0x2005,
        cmdRedo          = 0x2006,
        cmdSelectAll     = 0x2007,
        cmdOpenOutput    = 0x2008,
        cmdProcess       = 0x2009,
    };

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

    // Edit operations (also invoked via ApplicationCommandTarget::perform)
    void editCopy();
    void editCut();
    void editPaste();

    // ApplicationCommandTarget
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>&) override;
    void getCommandInfo(juce::CommandID, juce::ApplicationCommandInfo&) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo&) override;

    // Shared command manager — AppMenuBar uses this for addCommandItem
    juce::ApplicationCommandManager commandManager;

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
    juce::TextButton   outputParentButton;   // shows truncated output path; click to browse
    juce::ToggleButton cleanOutputToggle;    // "Clean folder before export?"
    juce::File         outputBaseFolder;     // full output folder path

    juce::TextButton processButton;
    juce::TextButton openOutputButton;
    juce::TextButton fillButton;
    juce::TextButton clearButton;

    PreviewPanel previewPanel;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<GuiProcessor> processor;

    juce::ApplicationProperties appProperties;
    juce::Array<juce::File> sampleClipboard;       // shared between Pack and Bank views
    int lastInternalCopyChangeCount = -1;          // NSPasteboard changeCount at last internal copy

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
    void layoutOutputSection(juce::Rectangle<int>& area, int sectionGap);
    BankEditorPanel* getActiveEditor();  // returns the visible Pack-mode editor
    void prepareOutputFolder(const juce::File& folder);   // clean + create
    void layoutButtonRow(juce::Rectangle<int>& area, int h);

    // Persistent folder preferences
    juce::File getSavedFolder(const juce::String& key);
    void       saveFolder(const juce::String& key, const juce::File& folder);
    void       saveString(const juce::String& key, const juce::String& value);
    juce::String getSavedString(const juce::String& key, const juce::String& fallback = {});

    // Undo / redo
    struct UndoState
    {
        juce::File cubbiSlots[ChompiNamer::NUM_BANKS][ChompiNamer::SLOTS_PER_BANK];
        juce::File jammiSlots[ChompiNamer::NUM_BANKS][ChompiNamer::SLOTS_PER_BANK];
    };

    static constexpr int MAX_UNDO_STEPS = 10;
    juce::Array<UndoState> undoStack;
    juce::Array<UndoState> redoStack;
    bool isApplyingUndoState = false;

    UndoState readCurrentState();
    void captureUndoState();
    void applyUndoState(const UndoState& state);
    void performUndo();
    void performRedo();

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
