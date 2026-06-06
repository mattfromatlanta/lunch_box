// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Pack-grid drag-move: thin wiring from mouse events into the shared
// DragController. Click-collapse on an unmoved mouseDown stays here since
// that's selection-collapse semantics, not drag.

#include "BankEditorPanel.h"
#include "BankEditorPanel_Private.h"
#include "UIColours.h"

using namespace BankEditorImpl;
using LunchBoxDrag::GridCell;

namespace
{
    GridCell toGridCell(BankEditorPanel::Cell c) { return { c.row, c.col }; }
    BankEditorPanel::Cell toCell(GridCell g)     { return { g.bank, g.slot }; }

    juce::Array<GridCell> toGridCells(const juce::Array<BankEditorPanel::Cell>& src)
    {
        juce::Array<GridCell> out;
        out.ensureStorageAllocated(src.size());
        for (const auto& c : src) out.add(toGridCell(c));
        return out;
    }
}

void BankEditorPanel::handleSlotMouseDown(BankSlotComponent* src, const juce::MouseEvent& e)
{
    Cell c = getCellFor(src);
    if (!c.isValid()) return;

    mouseDownCell       = c;
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
    }

    grabKeyboardFocus();
}

void BankEditorPanel::handleSlotMouseDrag(BankSlotComponent* /*src*/, const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;
    if (e.getDistanceFromDragStart() < 5) return;
    if (!mouseDownCell.isValid())      return;

    const auto panelPt = e.getEventRelativeTo(this).getPosition();

    if (!dragController.isDragging())
    {
        dragController.begin(toGridCells(selection),
                             toGridCell(mouseDownCell));
        mouseDownOnSelected = false;
    }

    dragController.update(panelPt);
}

void BankEditorPanel::handleSlotMouseUp(BankSlotComponent*, const juce::MouseEvent& /*e*/)
{
    if (dragController.isDragging())
    {
        dragController.commit();
        if (onPreviewStop) onPreviewStop();   // no auto-play on drag release
    }
    else if (mouseDownOnSelected)
    {
        // Plain click on a selected cell — collapse selection to just that cell
        selectCell(mouseDownCell, true);
        notifyPreviewForSelection();
    }

    mouseDownOnSelected = false;
    mouseDownCell       = { -1, -1 };
}

void BankEditorPanel::modifierKeysChanged(const juce::ModifierKeys&)
{
    // Drag behaviour no longer depends on modifier keys held during the drag itself
}

void BankEditorPanel::mouseDown(const juce::MouseEvent&)
{
    if (dragController.isDragging())
        dragController.cancel();
    clearSelection();
    if (onBackgroundClicked) onBackgroundClicked();
}

// ─── DragHost implementation ─────────────────────────────────────────────────

LunchBoxDrag::GridDims BankEditorPanel::getGridDims() const
{
    return { LunchBoxNamer::NUM_BANKS, LunchBoxNamer::SLOTS_PER_BANK };
}

// Pack lays each bank's 14 slots out as two 7-cell visual rows. So the panel
// the user sees is 10 rows × 7 cols, not 5 × 14. Drag clamping must use this.
LunchBoxDrag::GridDims BankEditorPanel::getVisualDims() const
{
    return { LunchBoxNamer::NUM_BANKS * 2, LunchBoxNamer::SLOTS_PER_BANK / 2 };
}

LunchBoxDrag::GridCell BankEditorPanel::toVisual(LunchBoxDrag::GridCell c) const
{
    constexpr int half = LunchBoxNamer::SLOTS_PER_BANK / 2;     // 7
    return { c.bank * 2 + c.slot / half, c.slot % half };
}

LunchBoxDrag::GridCell BankEditorPanel::fromVisual(LunchBoxDrag::GridCell c) const
{
    constexpr int half = LunchBoxNamer::SLOTS_PER_BANK / 2;     // 7
    return { c.bank / 2, (c.bank % 2) * half + c.slot };
}

std::pair<int,int> BankEditorPanel::getBankClampRange() const
{
    return { 0, LunchBoxNamer::NUM_BANKS - 1 };
}

LunchBoxDrag::GridCell BankEditorPanel::cellAtPoint(juce::Point<int> panelPt) const
{
    auto c = getCellAtPoint(panelPt);
    return toGridCell(c);
}

juce::Rectangle<int> BankEditorPanel::cellBoundsInPanel(LunchBoxDrag::GridCell c) const
{
    if (auto* slot = getSlotAt(c.bank, c.slot))
        return getLocalArea(slot, slot->getLocalBounds());
    return {};
}

juce::File BankEditorPanel::getFileAt(LunchBoxDrag::GridCell c) const
{
    if (auto* slot = getSlotAt(c.bank, c.slot)) return slot->getSample();
    return {};
}

void BankEditorPanel::setFileAt(LunchBoxDrag::GridCell c, const juce::File& f)
{
    if (auto* slot = getSlotAt(c.bank, c.slot))
    {
        if (f != juce::File{}) slot->setSample(f);
        else                   slot->clearSample();
    }
}

void BankEditorPanel::setCellPreview(LunchBoxDrag::GridCell c, const juce::File& f)
{
    if (auto* slot = getSlotAt(c.bank, c.slot))
        slot->setPreviewSample(f);
}

void BankEditorPanel::setCellDragRoleSource(LunchBoxDrag::GridCell c, bool s)
{
    if (auto* slot = getSlotAt(c.bank, c.slot)) slot->setDragRoleSource(s);
}

void BankEditorPanel::setCellDragRoleDestination(LunchBoxDrag::GridCell c, bool s)
{
    if (auto* slot = getSlotAt(c.bank, c.slot)) slot->setDragRoleDestination(s);
}

void BankEditorPanel::setCellDragRoleDisplace(LunchBoxDrag::GridCell c, int dir)
{
    if (auto* slot = getSlotAt(c.bank, c.slot))
    {
        slot->setDragRoleDisplace(dir);

        // Compute per-cell from→to labels.
        // c is where this displaced content LANDS; from is one step opposite to dir.
        const auto dims   = getGridDims();
        const int  toIdx  = dims.globalIndex(c);
        const int  fromIdx = toIdx - dir;
        const LunchBoxDrag::GridCell fromCell { fromIdx / dims.slotsPerBank,
                                                fromIdx % dims.slotsPerBank };

        auto makeLabel = [](int bank, int s) -> juce::String
        {
            return juce::String::charToString((juce::juce_wchar)('A' + bank))
                   + juce::String(s + 1);
        };

        const juce::String fromLabel = makeLabel(fromCell.bank, fromCell.slot);
        const juce::String toLabel   = makeLabel(c.bank, c.slot);

        // Lower global index always on the left.
        if (dir > 0)
            slot->setDisplaceLabels(fromLabel, toLabel);
        else
            slot->setDisplaceLabels(toLabel, fromLabel);
    }
}

void BankEditorPanel::clearAllCellPreviews()
{
    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
            if (auto* slot = getSlotAt(b, s))
            {
                slot->clearPreviewSample();
                slot->setSwapHighlight(false);
                slot->setDragTarget(false);
                slot->setDragRoleSource(false);
                slot->setDragRoleDestination(false);
                slot->setDragRoleDisplace(0);
                slot->setDisplaceLabels({}, {});
            }
}

void BankEditorPanel::onPreviewRebuild(const LunchBoxDrag::DragOp& /*op*/) {}

void BankEditorPanel::onDragCommitWillBegin()
{
    if (onBeforeChange) onBeforeChange();
}

void BankEditorPanel::onDragCommitFinished(const juce::Array<LunchBoxDrag::GridCell>& newSelection,
                                            const juce::Array<LunchBoxDrag::GridCell>& oldSources)
{
    selection.clear();
    for (const auto& g : newSelection) selection.add(toCell(g));

    // Move focus to the destination corresponding to the previously-focused source cell.
    // Falls back to the first destination if the old focus cell wasn't in the drag.
    Cell newFocus = selection.isEmpty() ? focusCell : selection.getFirst();
    for (int i = 0; i < oldSources.size() && i < newSelection.size(); ++i)
    {
        if (toCell(oldSources[i]) == focusCell)
        {
            newFocus = toCell(newSelection[i]);
            break;
        }
    }
    focusCell = newFocus;

    updateSlotVisuals();
    if (onAssignmentsChanged) onAssignmentsChanged();
}
