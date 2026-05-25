// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BankRowComponent.h"
#include "../Processing/LunchBoxNamer.h"
#include "../Processing/BankFolderParser.h"

//==============================================================================
// BankEditorPanel - All 5 banks (A–E) for one CHOMPI category (M8)
//==============================================================================
// Contains 5 BankRowComponents + action buttons (Auto-Fill, Clear All, Sort A-Z).
//==============================================================================

class BankEditorPanel : public juce::Component,
                        public juce::DragAndDropContainer,
                        public juce::FileDragAndDropTarget
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
    juce::Array<juce::File> getSelectedFiles() const;
    void pasteFiles(const juce::Array<juce::File>& files);

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

private:
    LunchBoxNamer::Category category;
    juce::OwnedArray<BankRowComponent> banks;  // A–E

    std::unique_ptr<juce::FileChooser> fileChooser;

    static constexpr int ROW_HEIGHT    = 65;
    static constexpr int ROW_GAP       = 0;

    // Selection state
    juce::Array<Cell> selection;
    Cell focusCell        { 0, 0 };

    // Drag-select (rubber-band)
    bool isDragSelecting  = false;
    Cell dragAnchor       { -1, -1 };

    // Drag-move (relocate selection)
    bool isSelectionDragging  = false;
    Cell selectionDragStart   { -1, -1 };   // cell within selection where drag began
    juce::Array<Cell> dragTargetCells;

    // Deferred click: clicking on an already-selected cell defers the collapse
    bool mouseDownOnSelected = false;
    Cell mouseDownCell       { -1, -1 };

    // External file drag state
    juce::StringArray externalDragFiles;

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

    void commitSelectionDrag();
    void updateDragTargetVisuals();
    void updateDragPreviews();              // live preview during drag
    void clearAllPreviews();                // clears preview + swap highlight on all slots
    void clearDragState();                  // clears drag targets + resets drag flags

    void wireRowCallbacks(BankRowComponent* row, int bankIdx);
    void notifyPreviewForSelection();   // play if single filled cell; stop otherwise

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankEditorPanel)
};
