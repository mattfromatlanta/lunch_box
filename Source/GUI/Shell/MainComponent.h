// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include "GuiProcessor.h"
#include "LunchBoxFonts.h"
#include "UIColours.h"
#include "UIConstants.h"
#include "PreviewPanel.h"
#include "BankEditorPanel.h"
#include "BankFocusPanel.h"
#include "WipeTabButton.h"
#include "PackButton.h"
#include "PackNameOverlay.h"
#include "FooterButtonLAF.h"
#include "LabelStrings.h"
#if CHOMPI_MELATONIN_INSPECTOR
#include "melatonin_inspector/melatonin_inspector.h"
#endif

class ConsoleWindow;
void suppressTooltipsUntilMouseMove();

//==============================================================================
// MainComponent - Main GUI component for Lunch Box
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
        cmdFill          = 0x200a,
        cmdClear         = 0x200b,
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

    void saveSessionState();
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

    // Mode toggle (Pack / Bank)
    enum class ViewMode { Pack, Bank };
    ViewMode viewMode = ViewMode::Pack;

    WipeTabButton packModeButton;
    WipeTabButton bankModeButton;
    bool consoleVisible = false;
    juce::String     consoleContent { LunchBoxLabels::kConsoleInitial };
    std::unique_ptr<ConsoleWindow> consoleWindow;

    // ── Pack mode ───────────────────────────────────────────
    WipeTabButton cubbiTabButton;
    WipeTabButton jammiTabButton;
    bool showCubbiEditor = true;

    std::unique_ptr<BankEditorPanel> cubbiEditor;
    std::unique_ptr<BankEditorPanel> jammiEditor;

    // ── Bank focus mode ────────────────────────────────────
    std::unique_ptr<BankFocusPanel> bankFocusPanel;
    juce::Label bankStatusLabel;

    // ── Output folder (shared) ────────────────────────────
    juce::File         outputBaseFolder;     // full output folder path
    juce::String       lastPackName { "lunch_box" };

    FooterButtonLAF footerButtonLAF;
    PackButton      processButton;
    PackNameOverlay packNameOverlay;

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
    void setCategoryTab(bool showCubbi, bool animate = false);
    void styleTabButton(WipeTabButton& btn, bool active, juce::Colour activeCol);
    juce::Rectangle<int> computeContentArea() const;
    bool isTransitioning = false;
    std::unique_ptr<juce::ImageComponent> bankTransitionOverlay;
    void animateBankCategorySwitch(bool showCubbi);

    // Cross-tab data sync
    void syncPackToBankFocus();
    void syncBankFocusToAdvanced();

    // Helpers
    juce::File getResolvedOutputFolder();
    BankEditorPanel* getActiveEditor();  // returns the visible Pack-mode editor

    // Persistent folder preferences
    juce::File getSavedFolder(const juce::String& key);
    void       saveFolder(const juce::String& key, const juce::File& folder);
    void       saveString(const juce::String& key, const juce::String& value);
    juce::String getSavedString(const juce::String& key, const juce::String& fallback = {});

    // Session persistence
    void loadSessionState();

    // Undo / redo
    struct UndoState
    {
        juce::File cubbiSlots[LunchBoxNamer::NUM_BANKS][LunchBoxNamer::SLOTS_PER_BANK];
        juce::File jammiSlots[LunchBoxNamer::NUM_BANKS][LunchBoxNamer::SLOTS_PER_BANK];
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


    // Preview
    void stopPreview();

    // juce::KeyListener
    using juce::Component::keyPressed;
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;

    // Processing
    void processFilesFromEditors();
    void updateProcessButtonState();
    void appendStatus(const juce::String& message);
    void appendProcessingResult(const GuiProcessor::ProcessingResult& result, const juce::File& outputFolder);

#if CHOMPI_MELATONIN_INSPECTOR
    std::unique_ptr<melatonin::Inspector> inspector;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
