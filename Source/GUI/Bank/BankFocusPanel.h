// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "FocusedSlotRow.h"
#include "../Common/ClipboardEntry.h"
#include "../Common/DragController.h"
#include "../Common/DragHost.h"
#include "../Processing/LunchBoxNamer.h"
#include "../Processing/BankFolderParser.h"

//==============================================================================
// BankFocusPanel - Single-bank focus view (third tab)
//
// Shows one bank at a time as a vertical list of 14 FocusedSlotRows.
// Bank selector (A-E) is a left column; Cubbi/Jammi toggle is at the top.
// Stores all data internally (slots[2][5][14]) so assignments are preserved
// when switching banks/categories. Synced with BankEditorPanel on tab switch
// by MainComponent.
//==============================================================================

class BankFocusPanel : public juce::Component,
                       public juce::FileDragAndDropTarget,
                       public DragHost
{
public:
    BankFocusPanel(juce::AudioFormatManager& fmt, juce::AudioThumbnailCache& cache);
    ~BankFocusPanel() override;

    // --- Data access ---

    // Get all assignments for one category (all 5 banks)
    // Note: flushes active row state to storage before reading
    juce::Array<BankFolderParser::BankAssignment> getAssignments(LunchBoxNamer::Category cat);

    // Set a specific slot (used by MainComponent for cross-tab sync)
    void setSlot(LunchBoxNamer::Category cat, int bankIdx, int slotIdx, const juce::File& file);
    juce::File getSlotFile(LunchBoxNamer::Category cat, int bankIdx, int slotIdx);
    void clearAll();

    int getFilledCount(LunchBoxNamer::Category cat);

    // --- Bulk operations on the active bank ---
    void autoFillActiveFromFiles(const juce::Array<juce::File>& files);
    void clearActiveBank();

    // --- Keyboard navigation (driven by MainComponent::keyPressed) ---
    void moveFocusedRow(int delta);          // Up/Down arrow
    void expandRowSelection(int delta);      // Shift+Up/Down
    void playFocused();                      // Space
    void browseForFocused();                 // Enter
    void clearFocusedRows();                 // Delete/Backspace
    void selectAll();                        // Cmd+A
    void clearSelection();                   // Esc

    void triggerAutoFill();   // shows folder picker and fills active bank
    void triggerClear();      // clears active bank and refreshes rows

    // Copy / cut / paste
    juce::Array<ClipboardEntry> getSelectedClipboard();
    void pasteClipboard(const juce::Array<ClipboardEntry>& entries);

    // --- Category / bank switching (driven by MainComponent) ---
    void switchToCategory(LunchBoxNamer::Category cat);

    // Set focused bank + row directly (used when syncing focus from Pack mode)
    void setActiveFocus(int bankIdx, int rowIdx);
    int  getActiveBank()  const { return activeBank; }
    int  getFocusedRow()  const { return focusedRowIdx; }

    // --- Callbacks ---
    // Fired just before any slot content is about to change (for undo capture)
    std::function<void()>                    onBeforeChange;

    std::function<void()>                    onAssignmentsChanged;
    std::function<void(const juce::File&)>   onSlotClicked;
    std::function<void()>                    onPreviewStop;
    std::function<juce::File()>              getStartDirectory;
    std::function<void(juce::File)>          onFolderBrowsed;
    std::function<void(const juce::String&)> onLog;

    // --- juce::Component ---
    void paint(juce::Graphics& g) override;
    void resized() override;
    void modifierKeysChanged(const juce::ModifierKeys& mods) override;

    // --- juce::FileDragAndDropTarget ---
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragMove (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    // --- DragHost ---
    LunchBoxDrag::GridDims  getGridDims() const override;
    LunchBoxDrag::GridDims  getVisualDims() const override;
    LunchBoxDrag::GridCell  toVisual  (LunchBoxDrag::GridCell c) const override;
    LunchBoxDrag::GridCell  fromVisual(LunchBoxDrag::GridCell c) const override;
    std::pair<int,int>      getBankClampRange() const override;
    LunchBoxDrag::GridCell  cellAtPoint(juce::Point<int> panelPt) const override;
    juce::Rectangle<int>    cellBoundsInPanel(LunchBoxDrag::GridCell c) const override;
    juce::File              getFileAt(LunchBoxDrag::GridCell c) const override;
    void                    setFileAt(LunchBoxDrag::GridCell c, const juce::File& f) override;
    void                    setCellPreview(LunchBoxDrag::GridCell c, const juce::File& f) override;
    void                    setCellDragRoleSource     (LunchBoxDrag::GridCell c, bool s) override;
    void                    setCellDragRoleDestination(LunchBoxDrag::GridCell c, bool s) override;
    void                    setCellDragRoleDisplace   (LunchBoxDrag::GridCell c, int dir) override;
    void                    clearAllCellPreviews() override;
    void                    onDragCommitWillBegin() override;
    void                    onDragCommitFinished(const juce::Array<LunchBoxDrag::GridCell>& newSelection,
                                                const juce::Array<LunchBoxDrag::GridCell>& oldSources) override;

private:
    // Shared audio resources (from AudioPreviewPlayer, owned by MainComponent)
    juce::AudioFormatManager&  formatManager;
    juce::AudioThumbnailCache& thumbnailCache;

    // All slot data: [category][bank][slot]
    juce::File slots[2][LunchBoxNamer::NUM_BANKS][LunchBoxNamer::SLOTS_PER_BANK];

    // Per-category persisted focus state (saved/restored on category toggle)
    struct CategoryFocusState
    {
        int activeBank    = 0;
        int focusedRowIdx = 0;
        juce::Array<int> selection;
        CategoryFocusState() { selection.add(0); }
    };
    CategoryFocusState perCategoryState[2];  // index 0=Cubbi, 1=Jammi

    // Active view state
    LunchBoxNamer::Category activeCategory = LunchBoxNamer::Category::Cubbi;
    int activeBank = 0;  // 0=A … 4=E

    // Child components
    juce::TextButton bankButtons[LunchBoxNamer::NUM_BANKS];  // A-E

    juce::OwnedArray<FocusedSlotRow> rows;  // 14 rows for active bank

    std::unique_ptr<juce::FileChooser> fileChooser;

    // External file drag state
    juce::StringArray externalDragFiles;
    void updateExternalDragHighlight(int x, int y);

    // Selection / keyboard focus state
    int  focusedRowIdx   = 0;
    int  selectionAnchor = 0;
    juce::Array<int> selection;

    // Drag-move state is owned by the shared DragController. Deferred-deselect
    // (plain mouseDown on a multi-selected row, decide on mouseUp) stays here.
    bool deferredDeselect = false;
    int  mouseDownRowIdx  = -1;

    DragController dragController { *this };

    static constexpr int BANK_COL_WIDTH  = 45;
    static constexpr int ROW_HEIGHT      = 55;
    static constexpr int ROW_GAP         = 1;

    // Internal helpers
    void switchToBank(int bankIdx);
    void flushRowsToStorage();      // write active row UI state back to slots[]
    void populateRowsFromStorage();
    void updateBankButtonStyles();
    void wireRowCallbacks(FocusedSlotRow* row, int rowIdx);

    // Selection helpers
    void setFocusedRow(int idx, bool clearOthers);
    void notifyPreviewForFocused();
    void updateRowVisuals();
    void toggleRowInSelection(int idx);
    void selectRowRange(int from, int to);

    bool isPopulating = false;      // prevents re-entrant flushRowsToStorage()

    std::unique_ptr<juce::LookAndFeel_V4> bankButtonLAF;

    void handleRowMouseDown(FocusedSlotRow* row, const juce::MouseEvent& e);
    void handleRowMouseDrag(FocusedSlotRow* row, const juce::MouseEvent& e);
    void handleRowMouseUp  (FocusedSlotRow* row, const juce::MouseEvent& e);

    int rowIndexFor(FocusedSlotRow* row) const;
    int rowAtPoint(int x, int y) const;

    void styleTabButton(juce::TextButton& btn, bool active, int bankIdx);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankFocusPanel)
};
