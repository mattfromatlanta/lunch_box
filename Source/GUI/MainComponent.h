// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include "GuiProcessor.h"
#include "PreviewPanel.h"
#include "BankEditorPanel.h"
#include "BankFocusPanel.h"
#if CHOMPI_MELATONIN_INSPECTOR
#include "melatonin_inspector/melatonin_inspector.h"
#endif

class ConsoleWindow;

//==============================================================================
// WipeTabButton — TextButton with a horizontal colour-wipe animation
//==============================================================================
class WipeTabButton : public juce::TextButton,
                      private juce::Timer
{
public:
    WipeTabButton() = default;

    void snapToColour (juce::Colour col)
    {
        stopTimer();
        currentFill  = col;
        wipeProgress = 1.0f;
        repaint();
    }

    // Animate from current colour to target, sweeping in from the right (fromRight=true)
    // or from the left (fromRight=false), over durationMs milliseconds.
    void startWipe (juce::Colour target, bool fromRight, int durationMs = 200)
    {
        stopTimer();
        fromFill      = currentFill;
        targetFill    = target;
        wipeFromRight = fromRight;
        wipeProgress  = 0.0f;
        stepPerFrame  = 1.0f / (float (durationMs) * 60.0f / 1000.0f);
        startTimerHz (60);
    }

private:
    void timerCallback() override
    {
        wipeProgress += stepPerFrame;
        if (wipeProgress >= 1.0f)
        {
            wipeProgress = 1.0f;
            currentFill  = targetFill;
            stopTimer();
        }
        repaint();
    }

    void paintButton (juce::Graphics& g, bool, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        constexpr float r = 4.0f;

        auto drawHalf = [&] (juce::Rectangle<float> clip, juce::Colour col)
        {
            g.saveState();
            g.reduceClipRegion (clip.toNearestInt());
            g.setColour (col);
            g.fillRoundedRectangle (b, r);
            g.restoreState();
        };

        if (wipeProgress > 0.0f && wipeProgress < 1.0f)
        {
            const float splitX = wipeFromRight ? b.getWidth() * (1.0f - wipeProgress)
                                               : b.getWidth() * wipeProgress;
            drawHalf (b.withWidth (splitX),  wipeFromRight ? fromFill   : targetFill);
            drawHalf (b.withLeft  (splitX),  wipeFromRight ? targetFill : fromFill);
        }
        else
        {
            g.setColour (currentFill);
            g.fillRoundedRectangle (b, r);
        }

        g.setColour (findColour (juce::TextButton::textColourOffId));
        g.setFont (getLookAndFeel().getTextButtonFont (*this, getHeight()));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred, false);
    }

    juce::Colour fromFill    { juce::Colour (0xff1e2838) };
    juce::Colour targetFill  { juce::Colour (0xff1e2838) };
    juce::Colour currentFill { juce::Colour (0xff1e2838) };
    float        wipeProgress = 1.0f;
    float        stepPerFrame = 1.0f / 12.0f;
    bool         wipeFromRight = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WipeTabButton)
};

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
    juce::Image logoImage;

    // Mode toggle (Pack / Bank)
    enum class ViewMode { Pack, Bank };
    ViewMode viewMode = ViewMode::Pack;

    WipeTabButton packModeButton;
    WipeTabButton bankModeButton;
    bool consoleVisible = false;
    juce::String     consoleContent { "Ready to process samples...\n" };
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

    juce::TextButton processButton;
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
    void styleTabButton(WipeTabButton& btn, bool active);
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


    // Preview
    void stopPreview();

    // juce::KeyListener
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
