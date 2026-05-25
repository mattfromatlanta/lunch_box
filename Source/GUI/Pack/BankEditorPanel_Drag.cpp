// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Drag-move + swap state machine for the Pack 5×14 grid. Mouse handlers,
// commit, preview/highlight visuals, and modifier-key handling.

#include "BankEditorPanel.h"
#include "BankEditorPanel_Private.h"
#include "UIColours.h"

using namespace BankEditorImpl;

void BankEditorPanel::handleSlotMouseDown(BankSlotComponent* src, const juce::MouseEvent& e)
{
    Cell c = getCellFor(src);
    if (!c.isValid()) return;

    mouseDownCell = c;
    mouseDownOnSelected = false;

    if (e.mods.isShiftDown())
    {
        selectRange(focusCell, c);
        notifyPreviewForSelection();
    }
    else if (e.mods.isCommandDown())
    {
        toggleCell(c);
        notifyPreviewForSelection();
    }
    else if (selection.contains(c))
    {
        // Defer: could become a drag-move or a click-to-collapse
        mouseDownOnSelected = true;
    }
    else
    {
        selectCell(c, true);
        mouseDownOnSelected = true;  // plain drag from here = move, not box select
        // Preview fires on mouseUp (via mouseDownOnSelected path), not here
    }

    grabKeyboardFocus();
}

void BankEditorPanel::handleSlotMouseDrag(BankSlotComponent* /*src*/, const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;
    if (e.getDistanceFromDragStart() < 5) return;

    auto panelPt = e.getEventRelativeTo(this).getPosition();
    Cell hover   = getCellAtPoint(panelPt);

    // All drags move the selection
    if (!isSelectionDragging)
    {
        isSelectionDragging = true;
        selectionDragStart  = mouseDownCell;
        mouseDownOnSelected = false;
    }

    if (isSelectionDragging)
    {
        if (!hover.isValid() || !selectionDragStart.isValid()) return;

        // Unclamped delta: how far the cursor has moved from the drag-start cell
        int dr = hover.row - selectionDragStart.row;
        int dc = hover.col - selectionDragStart.col;

        // Bounding box of the current selection
        int minRow = LunchBoxNamer::NUM_BANKS,      maxRow = -1;
        int minCol = LunchBoxNamer::SLOTS_PER_BANK, maxCol = -1;
        for (const auto& c : selection)
        {
            minRow = std::min(minRow, c.row);  maxRow = std::max(maxRow, c.row);
            minCol = std::min(minCol, c.col);  maxCol = std::max(maxCol, c.col);
        }

        // Clamp so entire selection stays within the grid
        dr = juce::jlimit(-minRow, (LunchBoxNamer::NUM_BANKS      - 1) - maxRow, dr);
        dc = juce::jlimit(-minCol, (LunchBoxNamer::SLOTS_PER_BANK - 1) - maxCol, dc);

        dragTargetCells.clear();
        for (const auto& c : selection)
            dragTargetCells.add({ c.row + dr, c.col + dc });

        updateDragTargetVisuals();
        updateDragPreviews();
    }
}

void BankEditorPanel::handleSlotMouseUp(BankSlotComponent*, const juce::MouseEvent& /*e*/)
{
    if (isSelectionDragging && !dragTargetCells.isEmpty())
    {
        commitSelectionDrag();
        if (onPreviewStop) onPreviewStop();  // no auto-play on drag release
    }
    else if (mouseDownOnSelected)
    {
        // Simple click on a selected cell — collapse selection to just that cell
        selectCell(mouseDownCell, true);
        notifyPreviewForSelection();
    }

    clearDragState();
}

void BankEditorPanel::commitSelectionDrag()
{
    if (dragTargetCells.size() != selection.size()) return;
    if (onBeforeChange) onBeforeChange();

    // 1. Snapshot all source and target files before any mutation.
    //    Sorting by position ensures geometrically stable pairing regardless
    //    of how the selection was built (drag-select vs cmd-click).
    juce::Array<juce::File> sourceFiles, targetFiles;
    for (int i = 0; i < selection.size(); ++i)
    {
        auto* s = getSlotAt(selection[i].row,       selection[i].col);
        auto* t = getSlotAt(dragTargetCells[i].row, dragTargetCells[i].col);
        sourceFiles.add(s ? s->getSample() : juce::File{});
        targetFiles.add(t ? t->getSample() : juce::File{});
    }

    // 2. Build source-only and target-only (cell, file) pairs, sorted row-major.
    //    Source-only cells will receive displaced content; overlap cells carry
    //    the moving-set content and are excluded.
    juce::Array<CellFile> srcOnly, tgtOnly;
    for (int i = 0; i < selection.size(); ++i)
        if (!dragTargetCells.contains(selection[i]))
            srcOnly.add({ selection[i], sourceFiles[i] });
    for (int j = 0; j < dragTargetCells.size(); ++j)
        if (!selection.contains(dragTargetCells[j]))
            tgtOnly.add({ dragTargetCells[j], targetFiles[j] });

    sortCellsRowMajor(srcOnly);
    sortCellsRowMajor(tgtOnly);

    // 3. Clear all source slots (overlap slots re-filled in step 4)
    for (const auto& c : selection)
        if (auto* slot = getSlotAt(c.row, c.col))
            slot->clearSample();

    // 4. Write moving-set content into every target slot
    for (int i = 0; i < dragTargetCells.size(); ++i)
    {
        if (auto* slot = getSlotAt(dragTargetCells[i].row, dragTargetCells[i].col))
        {
            if (sourceFiles[i] != juce::File{}) slot->setSample(sourceFiles[i]);
            else                                slot->clearSample();
        }
    }

    // 5. Write displaced target-only content into the vacated source-only cells
    int pairs = juce::jmin(srcOnly.size(), tgtOnly.size());
    for (int k = 0; k < pairs; ++k)
    {
        if (auto* slot = getSlotAt(srcOnly[k].c.row, srcOnly[k].c.col))
        {
            if (tgtOnly[k].f != juce::File{}) slot->setSample(tgtOnly[k].f);
            else                              slot->clearSample();
        }
    }

    // 6. Selection follows moved cells; focus tracks its relative position
    int dr = dragTargetCells[0].row - selection[0].row;
    int dc = dragTargetCells[0].col - selection[0].col;
    focusCell = { focusCell.row + dr, focusCell.col + dc };
    selection = dragTargetCells;

    dragTargetCells.clear();
    updateDragTargetVisuals();
    updateSlotVisuals();
}

void BankEditorPanel::updateDragTargetVisuals()
{
    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
            if (auto* slot = getSlotAt(b, s))
                slot->setDragTarget(dragTargetCells.contains({b, s}));
}

void BankEditorPanel::updateDragPreviews()
{
    clearAllPreviews();

    if (dragTargetCells.size() != selection.size()) return;

    // Snapshot actual (non-preview) files before setting any previews
    juce::Array<juce::File> sourceFiles, targetFiles;
    for (int i = 0; i < selection.size(); ++i)
    {
        auto* s = getSlotAt(selection[i].row,       selection[i].col);
        auto* t = getSlotAt(dragTargetCells[i].row, dragTargetCells[i].col);
        sourceFiles.add(s ? s->getSample() : juce::File{});
        targetFiles.add(t ? t->getSample() : juce::File{});
    }

    // Target cells: show the incoming content
    for (int i = 0; i < dragTargetCells.size(); ++i)
        if (auto* slot = getSlotAt(dragTargetCells[i].row, dragTargetCells[i].col))
            slot->setPreviewSample(sourceFiles[i]);

    // Source-only cells: show the displaced content that will shift back there.
    // Mirrors the row-major pairing used in commitSelectionDrag.
    juce::Array<CellFile> srcOnly, tgtOnly;
    for (int i = 0; i < selection.size(); ++i)
        if (!dragTargetCells.contains(selection[i]))
            srcOnly.add({ selection[i], sourceFiles[i] });
    for (int j = 0; j < dragTargetCells.size(); ++j)
        if (!selection.contains(dragTargetCells[j]))
            tgtOnly.add({ dragTargetCells[j], targetFiles[j] });

    sortCellsRowMajor(srcOnly);
    sortCellsRowMajor(tgtOnly);

    int pairs = juce::jmin(srcOnly.size(), tgtOnly.size());
    for (int k = 0; k < pairs; ++k)
        if (auto* slot = getSlotAt(srcOnly[k].c.row, srcOnly[k].c.col))
            slot->setPreviewSample(tgtOnly[k].f);
}

void BankEditorPanel::clearAllPreviews()
{
    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
            if (auto* slot = getSlotAt(b, s))
            {
                slot->clearPreviewSample();
                slot->setSwapHighlight(false);
            }
}

void BankEditorPanel::clearDragState()
{
    isDragSelecting      = false;
    isSelectionDragging  = false;
    mouseDownOnSelected  = false;
    dragTargetCells.clear();
    clearAllPreviews();
    updateDragTargetVisuals();
}

// ─────────────────────────────────────────────────────────────────────────────

void BankEditorPanel::modifierKeysChanged(const juce::ModifierKeys&)
{
    // Drag mode no longer depends on modifier keys
}

void BankEditorPanel::mouseDown(const juce::MouseEvent&)
{
    clearDragState();
    clearSelection();
    if (onBackgroundClicked) onBackgroundClicked();
}
