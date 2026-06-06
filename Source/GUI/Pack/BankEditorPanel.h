// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BankRowComponent.h"
#include "../Common/ClipboardEntry.h"
#include "../Common/DragController.h"
#include "../Common/DragHost.h"
#include "../Processing/LunchBoxNamer.h"
#include "../Processing/BankFolderParser.h"

//==============================================================================
// BankEditorPanel - 5×14 grid for one CHOMPI category (Cubbi or Jammi).
// Composes 5 BankRowComponents (one per bank A–E); footer-level actions
// (auto-fill, clear, process) are owned by MainComponent.
//==============================================================================

class BankEditorPanel : public juce::Component,
                        public juce::DragAndDropContainer,
                        public juce::FileDragAndDropTarget,
                        public DragHost
{
public:
    // Grid cell coordinate (row = bank 0-4, col = slot 0-13)
    struct Cell
    {
        int row = -1, col = -1;
        bool isValid()                    const { return row >= 0 && col >= 0; }
        bool operator==(const Cell& o)    const { return row == o.row && col == o.col; }
        bool operator!=(const Cell& o)    const { return !(*this == o); }
    };

    explicit BankEditorPanel(LunchBoxNamer::Category category);

    // Get all filled-slot assignments (used for processing)
    juce::Array<BankFolderParser::BankAssignment> getAssignments() const;
    int getFilledCount() const;

    // Bulk operations
    void clearAllBanks();
    void autoFillFromFolder(const juce::File& folder);

    // Individual slot access (used for cross-tab sync and undo)
    void setSlotFile(int bankIdx, int slotIdx, const juce::File& file);
    juce::File getSlotFile(int bankIdx, int slotIdx) const;

    // Selection / keyboard navigation (driven by MainComponent::keyPressed)
    int  selectionSize()  const { return selection.size(); }
    void clearSelection();
    void selectAll();
    void moveFocus(int dRow, int dCol);
    void expandSelection(int dRow, int dCol);  // Shift+Arrow: grow the selection bbox by 1
    void tabFocus();
    void playFocused();
    void clearSelectedCells();
    void browseForFocused();

    // Copy / cut / paste
    juce::Array<ClipboardEntry> getSelectedClipboard() const;
    void pasteClipboard(const juce::Array<ClipboardEntry>& entries);

    // Fired just before any slot content is about to change (for undo capture)
    std::function<void()> onBeforeChange;

    // Fired whenever any slot content changes
    std::function<void()> onAssignmentsChanged;

    // Fired when preview should stop (multi-cell selection, empty cell focused, or background click)
    std::function<void()> onPreviewStop;

    // Fired when a slot is previewed — passes the grid cell and file
    std::function<void(Cell, const juce::File&)> onSlotClicked;

    // Returns the current keyboard-focus cell
    Cell getFocusCell() const { return focusCell; }

    // Move focus and select only that cell (used when syncing focus from Bank mode)
    void setFocusCellAndSelect(Cell cell);

    // Folder memory: return start dir for file browser; receive parent of selected file
    std::function<juce::File()>                  getStartDirectory;
    std::function<void(juce::File)>              onFolderBrowsed;

    // Log messages (e.g. overflow warnings during auto-fill)
    std::function<void(const juce::String&)>     onLog;

    // Fired when the user clicks on empty panel space (stops preview)
    std::function<void()> onBackgroundClicked;

    // juce::Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void modifierKeysChanged(const juce::ModifierKeys& mods) override;

    // juce::FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragMove (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    // DragHost
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
    void                    onPreviewRebuild(const LunchBoxDrag::DragOp& op) override;
    void                    onDragCommitWillBegin() override;
    void                    onDragCommitFinished(const juce::Array<LunchBoxDrag::GridCell>& newSelection,
                                                const juce::Array<LunchBoxDrag::GridCell>& oldSources) override;

private:
    LunchBoxNamer::Category category;
    juce::OwnedArray<BankRowComponent> banks;  // A–E

    std::unique_ptr<juce::FileChooser> fileChooser;

    static constexpr int ROW_HEIGHT    = 65;
    static constexpr int ROW_GAP       = 0;

    // Selection state
    juce::Array<Cell> selection;
    Cell focusCell        { 0, 0 };

    // Drag-select (rubber-band) — unchanged from prior behavior
    bool isDragSelecting  = false;
    Cell dragAnchor       { -1, -1 };

    // Drag-move state is owned by the shared DragController. Click-collapse
    // (mouseDown on a selected cell, no drag) is still handled here.
    bool mouseDownOnSelected = false;
    Cell mouseDownCell       { -1, -1 };

    DragController dragController { *this };

    // External file drag state
    juce::StringArray externalDragFiles;
    juce::Array<Cell> externalDropTargets;     // cells highlighted while external drag is active

    // External drag helpers
    juce::Array<Cell> getExternalDropCells(Cell start, int count) const;
    void updateExternalDragHighlight(int x, int y);

    // Internal helpers
    BankSlotComponent* getSlotAt(int bankIdx, int slotIdx) const;
    Cell getCellFor(BankSlotComponent* slot) const;
    Cell getCellAtPoint(juce::Point<int> ptInPanel) const;
    Cell getEarliestSelected() const;

    void selectCell(Cell c, bool clearFirst = true);
    void toggleCell(Cell c);
    void selectRange(Cell a, Cell b);
    void updateSlotVisuals();

    void handleSlotMouseDown(BankSlotComponent* src, const juce::MouseEvent& e);
    void handleSlotMouseDrag(BankSlotComponent* src, const juce::MouseEvent& e);
    void handleSlotMouseUp  (BankSlotComponent* src, const juce::MouseEvent& e);

    void wireRowCallbacks(BankRowComponent* row, int bankIdx);
    void notifyPreviewForSelection();   // play if single filled cell; stop otherwise

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankEditorPanel)
};
