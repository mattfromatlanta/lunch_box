// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Bank-focus drag-move: thin wiring from row mouse events into the shared
// DragController. Bank mode is intra-bank — the controller clamps the drag
// to the active bank via getBankClampRange().

#include "BankFocusPanel.h"
#include "BankFocusPanel_Private.h"
#include "UIColours.h"
#include "UIConstants.h"

using namespace BankFocusImpl;
using LunchBoxDrag::GridCell;

void BankFocusPanel::handleRowMouseDown(FocusedSlotRow* row, const juce::MouseEvent& e)
{
    int idx = rowIndexFor(row);
    if (idx < 0) return;

    grabKeyboardFocus();
    mouseDownRowIdx  = idx;
    deferredDeselect = false;

    if (e.mods.isShiftDown())
    {
        selectRowRange(selectionAnchor, idx);
        focusedRowIdx = idx;
        updateRowVisuals();
        notifyPreviewForFocused();
    }
    else if (e.mods.isCommandDown())
    {
        toggleRowInSelection(idx);
        focusedRowIdx = idx;
        updateRowVisuals();
        notifyPreviewForFocused();
    }
    else if (selection.size() > 1 && selection.contains(idx))
    {
        // Plain click on an already-selected row in a multi-selection:
        // keep the selection intact so a drag will move the whole group.
        // Collapse to single-select on mouseUp only if no drag occurred.
        focusedRowIdx    = idx;
        deferredDeselect = true;
    }
    else
    {
        setFocusedRow(idx, true);
    }
}

void BankFocusPanel::handleRowMouseDrag(FocusedSlotRow* row, const juce::MouseEvent& e)
{
    if (mouseDownRowIdx < 0) return;
    if (e.getDistanceFromDragStart() < 6) return;

    // Convert row-relative mouse point to panel coords.
    const auto panelPt = juce::Point<int>(row->getX() + e.getPosition().x,
                                          row->getY() + e.getPosition().y);

    if (!dragController.isDragging())
    {
        // Build the source-cell list from the current selection (or just the
        // mouseDown row if it isn't part of a multi-selection).
        juce::Array<GridCell> sourceCells;
        if (selection.size() > 1 && selection.contains(mouseDownRowIdx))
        {
            for (int i : selection) sourceCells.add({ activeBank, i });
        }
        else
        {
            sourceCells.add({ activeBank, mouseDownRowIdx });
        }

        // The controller reads file contents via getFileAt(), which (per
        // DragHost) returns the row's current sample — keep it that way for
        // consistency, no flushRowsToStorage() needed here.
        dragController.begin(sourceCells,
                             { activeBank, mouseDownRowIdx });
        deferredDeselect = false;
    }

    dragController.update(panelPt);
}

void BankFocusPanel::handleRowMouseUp(FocusedSlotRow*, const juce::MouseEvent& /*e*/)
{
    if (dragController.isDragging())
    {
        dragController.commit();
        if (onPreviewStop) onPreviewStop();
    }
    else if (deferredDeselect)
    {
        setFocusedRow(mouseDownRowIdx, true);
    }

    deferredDeselect = false;
    mouseDownRowIdx  = -1;
}

// ─── DragHost implementation ─────────────────────────────────────────────────

LunchBoxDrag::GridDims BankFocusPanel::getGridDims() const
{
    return { LunchBoxNamer::NUM_BANKS, LunchBoxNamer::SLOTS_PER_BANK };
}

// Bank mode shows one bank as a single-column vertical list of 14 rows. The
// visual grid is therefore 14 rows × 1 col. Data bank stays at activeBank.
LunchBoxDrag::GridDims BankFocusPanel::getVisualDims() const
{
    return { LunchBoxNamer::SLOTS_PER_BANK, 1 };
}

LunchBoxDrag::GridCell BankFocusPanel::toVisual(LunchBoxDrag::GridCell c) const
{
    // c.bank is expected to equal activeBank during a drag.
    return { c.slot, 0 };
}

LunchBoxDrag::GridCell BankFocusPanel::fromVisual(LunchBoxDrag::GridCell c) const
{
    return { activeBank, c.bank };
}

std::pair<int,int> BankFocusPanel::getBankClampRange() const
{
    // Intra-bank only — cascade stays in active bank.
    return { activeBank, activeBank };
}

LunchBoxDrag::GridCell BankFocusPanel::cellAtPoint(juce::Point<int> panelPt) const
{
    int idx = rowAtPoint(panelPt.x, panelPt.y);
    if (idx < 0) return {};
    return { activeBank, idx };
}

juce::Rectangle<int> BankFocusPanel::cellBoundsInPanel(LunchBoxDrag::GridCell c) const
{
    if (c.bank != activeBank) return {};
    if (c.slot < 0 || c.slot >= rows.size()) return {};
    return rows[c.slot]->getBounds();
}

juce::File BankFocusPanel::getFileAt(LunchBoxDrag::GridCell c) const
{
    if (c.bank != activeBank) return {};
    if (c.slot < 0 || c.slot >= rows.size()) return {};
    return rows[c.slot]->getSample();
}

void BankFocusPanel::setFileAt(LunchBoxDrag::GridCell c, const juce::File& f)
{
    if (c.bank != activeBank) return;
    if (c.slot < 0 || c.slot >= rows.size()) return;

    if (f != juce::File{}) rows[c.slot]->setSample(f);
    else                   rows[c.slot]->clearSample();
}

void BankFocusPanel::setCellPreview(LunchBoxDrag::GridCell c, const juce::File& f)
{
    if (c.bank != activeBank) return;
    if (c.slot < 0 || c.slot >= rows.size()) return;
    rows[c.slot]->setPreviewSample(f);
}

void BankFocusPanel::setCellDragRoleSource(LunchBoxDrag::GridCell c, bool s)
{
    if (c.bank != activeBank) return;
    if (c.slot < 0 || c.slot >= rows.size()) return;
    rows[c.slot]->setDragRoleSource(s);
}

void BankFocusPanel::setCellDragRoleDestination(LunchBoxDrag::GridCell c, bool s)
{
    if (c.bank != activeBank) return;
    if (c.slot < 0 || c.slot >= rows.size()) return;
    rows[c.slot]->setDragRoleDestination(s);
}

void BankFocusPanel::setCellDragRoleDisplace(LunchBoxDrag::GridCell c, int dir)
{
    if (c.bank != activeBank) return;
    if (c.slot < 0 || c.slot >= rows.size()) return;
    rows[c.slot]->setDragRoleDisplace(dir);
}

void BankFocusPanel::clearAllCellPreviews()
{
    for (auto* row : rows)
    {
        row->clearPreviewSample();
        row->setDragSource(false);
        row->setDragRoleSource(false);
        row->setDragRoleDestination(false);
        row->setDragRoleDisplace(0);
    }
}

void BankFocusPanel::onDragCommitWillBegin()
{
    if (onBeforeChange) onBeforeChange();
    // Make sure the row UI state is mirrored into slots[] before we mutate via
    // setFileAt() — keeps cross-bank syncing consistent.
    flushRowsToStorage();
}

void BankFocusPanel::onDragCommitFinished(const juce::Array<LunchBoxDrag::GridCell>& newSelection,
                                           const juce::Array<LunchBoxDrag::GridCell>& oldSources)
{
    flushRowsToStorage();

    selection.clear();
    for (const auto& g : newSelection)
        if (g.bank == activeBank)
            selection.add(g.slot);

    // Move focus to the destination corresponding to the previously-focused row.
    int newFocus = newSelection.isEmpty() ? focusedRowIdx : newSelection.getFirst().slot;
    for (int i = 0; i < oldSources.size() && i < newSelection.size(); ++i)
    {
        if (oldSources[i].bank == activeBank && oldSources[i].slot == focusedRowIdx)
        {
            newFocus = newSelection[i].slot;
            break;
        }
    }
    focusedRowIdx   = newFocus;
    selectionAnchor = newFocus;

    populateRowsFromStorage();
    updateRowVisuals();

    if (onAssignmentsChanged) onAssignmentsChanged();
}
