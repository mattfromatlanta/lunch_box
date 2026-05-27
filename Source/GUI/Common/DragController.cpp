// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DragController.h"

using namespace LunchBoxDrag;

DragController::DragController(DragHost& h) : host(h) {}

void DragController::begin(juce::Array<GridCell> selection, GridCell pickup)
{
    if (selection.isEmpty() || !pickup.isValid()) return;

    sourceCells  = std::move(selection);
    pickupCell   = pickup;
    lastDropCell = GridCell{};

    // Mark source cells as vacated so they render dim from drag start.
    for (const auto& s : sourceCells)
    {
        host.setCellPreview(s, juce::File{});
        host.setCellDragRoleSource(s, true);
    }

    state = State::Active;
}

// Build a DragOp in DATA coords with destCells precomputed via the host's
// visual mapping (accounts for Pack's 7-col visual wrap).
static DragOp buildDataOp(const DragHost& host,
                          const juce::Array<GridCell>& sourceCells,
                          GridCell pickupCell,
                          GridCell rawDropCellData)
{
    const auto visualDims = host.getVisualDims();
    DragOp visualOp;
    visualOp.pickupCell = host.toVisual(pickupCell);
    visualOp.dropCell   = host.toVisual(rawDropCellData);
    visualOp.sourceCells.ensureStorageAllocated(sourceCells.size());
    for (const auto& s : sourceCells) visualOp.sourceCells.add(host.toVisual(s));

    visualOp = clampOpToGrid(visualOp, visualDims, 0, visualDims.numBanks - 1);

    const auto vDelta = visualOp.delta();
    DragOp op;
    op.sourceCells = sourceCells;
    op.pickupCell  = pickupCell;
    op.dropCell    = host.fromVisual(visualOp.dropCell);
    op.destCells.ensureStorageAllocated(sourceCells.size());
    for (const auto& vs : visualOp.sourceCells)
        op.destCells.add(host.fromVisual(vs + vDelta));
    return op;
}

void DragController::update(juce::Point<int> cursorPanelPt)
{
    if (state != State::Active) return;

    const auto rawDrop = host.cellAtPoint(cursorPanelPt);
    if (! rawDrop.isValid() && ! lastDropCell.isValid())
        return;

    const auto dropCellData = rawDrop.isValid() ? rawDrop : lastDropCell;

    auto op = buildDataOp(host, sourceCells, pickupCell, dropCellData);

    if (op.dropCell == lastDropCell) return;
    lastDropCell = op.dropCell;

    rebuildPreviewsFor(op);
}

void DragController::rebuildPreviewsFor(const DragOp& op)
{
    host.clearAllCellPreviews();

    const auto& destCells = op.destCells;

    auto containsCell = [](const juce::Array<GridCell>& arr, GridCell c)
    {
        for (const auto& x : arr) if (x == c) return true;
        return false;
    };

    // Mark vacated source cells (initial sources not in destCells) as source role.
    for (const auto& s : sourceCells)
    {
        if (containsCell(destCells, s)) continue;
        host.setCellPreview(s, juce::File{});
        host.setCellDragRoleSource(s, true);
    }

    // One-shot direction-aware stack-insert against the current real grid.
    auto accessor = [this](GridCell c) { return host.getFileAt(c); };
    auto writes   = computeDragResult(accessor, op, host.getGridDims());

    for (const auto& w : writes)
    {
        host.setCellPreview(w.cell, w.file);

        if (containsCell(destCells, w.cell))
        {
            host.setCellDragRoleDestination(w.cell, true);
            host.setCellDragRoleSource     (w.cell, false);
        }
        else if (w.file != juce::File{})
        {
            host.setCellDragRoleDisplace(w.cell, true);
            host.setCellDragRoleSource  (w.cell, false);
        }
    }
}

void DragController::commit()
{
    if (state != State::Active) { cancel(); return; }

    if (!lastDropCell.isValid() || lastDropCell == pickupCell)
    {
        cancel();
        return;
    }

    auto op = buildDataOp(host, sourceCells, pickupCell, lastDropCell);
    if (op.dropCell == pickupCell)
    {
        cancel();
        return;
    }

    host.onDragCommitWillBegin();

    auto accessor = [this](GridCell c) { return host.getFileAt(c); };
    auto writes   = computeDragResult(accessor, op, host.getGridDims());

    for (const auto& w : writes)
        host.setFileAt(w.cell, w.file);

    juce::Array<GridCell> newSelection = op.destCells;

    host.clearAllCellPreviews();
    proxy.finish();
    state = State::Idle;
    sourceCells.clear();

    host.onDragCommitFinished(newSelection);
}

void DragController::cancel()
{
    host.clearAllCellPreviews();
    proxy.finish();
    state = State::Idle;
    sourceCells.clear();
    lastDropCell = GridCell{};
}
