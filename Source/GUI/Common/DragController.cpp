// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DragController.h"

using namespace LunchBoxDrag;

DragController::DragController(DragHost& h) : host(h) {}

void DragController::begin(juce::Array<GridCell> selection,
                           GridCell              pickup,
                           juce::Point<int>      mouseDownPanelPt,
                           juce::Rectangle<int>  panelBounds)
{
    if (selection.isEmpty() || !pickup.isValid()) return;

    sourceCells  = std::move(selection);
    pickupCell   = pickup;
    pickupAnchor = mouseDownPanelPt;
    lastDropCell = GridCell{};   // invalid → force first update to rebuild previews

    // Source cells dim out (their content has conceptually been picked up).
    // The proxy renders that content following the cursor. Cell-level previews
    // will be overlaid in update() to show where things land.
    for (const auto& s : sourceCells)
        host.setCellPreview(s, juce::File{});

    // Build the proxy snapshot from current grid state.
    juce::Array<DragProxy::ProxyCell> proxyCells;
    proxyCells.ensureStorageAllocated(sourceCells.size());
    for (const auto& s : sourceCells)
        proxyCells.add({ s, host.cellBoundsInPanel(s), host.getFileAt(s) });

    // Floating cursor proxy disabled — cell-level role highlights (destination
    // selection style, displaced-border accent, vacated source) are enough to
    // convey what's happening. Keep the proxy plumbing in place for now in case
    // we want to re-enable it.
    juce::ignoreUnused(proxyCells, mouseDownPanelPt, panelBounds);

    state = State::Active;
}

void DragController::update(juce::Point<int> cursorPanelPt)
{
    if (state != State::Active) return;

    const auto rawDrop = host.cellAtPoint(cursorPanelPt);
    if (! rawDrop.isValid() && ! lastDropCell.isValid())
    {
        juce::ignoreUnused(cursorPanelPt);   // proxy disabled
        return;
    }

    const auto dropCellData = rawDrop.isValid() ? rawDrop : lastDropCell;

    // ── Clamp in VISUAL space ───────────────────────────────────────────────
    // The drag is a pure translation in the UI grid (what the user sees). The
    // panel's visual layout may wrap the data axis (Pack: 14 slots wrap into
    // 7 cols × 2 rows per bank), so clamping in data coords gives a too-tight
    // bbox. Operate in visual space, then map back.
    const auto visualDims = host.getVisualDims();
    DragOp visualOp;
    visualOp.pickupCell = host.toVisual(pickupCell);
    visualOp.dropCell   = host.toVisual(dropCellData);
    visualOp.sourceCells.ensureStorageAllocated(sourceCells.size());
    for (const auto& s : sourceCells) visualOp.sourceCells.add(host.toVisual(s));

    visualOp = clampOpToGrid(visualOp, visualDims, 0, visualDims.numBanks - 1);

    // ── Build data-space op with explicit destCells ─────────────────────────
    const auto vDelta = visualOp.delta();
    DragOp op;
    op.sourceCells = sourceCells;
    op.pickupCell  = pickupCell;
    op.dropCell    = host.fromVisual(visualOp.dropCell);
    op.destCells.ensureStorageAllocated(sourceCells.size());
    for (const auto& vs : visualOp.sourceCells)
        op.destCells.add(host.fromVisual(vs + vDelta));

    juce::ignoreUnused(cursorPanelPt);   // proxy disabled — cell highlights suffice

    if (op.dropCell == lastDropCell) return;
    lastDropCell = op.dropCell;

    rebuildPreviewsFor(op);
}

void DragController::rebuildPreviewsFor(const DragOp& op)
{
    host.clearAllCellPreviews();

    // The op already carries destCells in data coords (computed via visual map).
    const auto& destCells = op.destCells;

    auto containsCell = [](const juce::Array<GridCell>& arr, GridCell c)
    {
        for (const auto& x : arr) if (x == c) return true;
        return false;
    };

    // Source cells (will be vacated) — flag as source so selection visuals fall away.
    for (const auto& s : sourceCells)
    {
        host.setCellPreview(s, juce::File{});
        host.setCellDragRoleSource(s, true);
    }

    // Resolve the full write list against the current real grid.
    auto accessor = [this](GridCell c) { return host.getFileAt(c); };
    auto writes   = computeDragResult(accessor, op, host.getGridDims());

    // Apply previews + classify each write target.
    //   destCells  → DragRoleDestination (selection look)
    //   otherwise  → source-only cell receiving displaced content → DragRoleDisplace
    //                (only if the write is non-empty — empties are just vacated clears)
    for (const auto& w : writes)
    {
        host.setCellPreview(w.cell, w.file);

        if (containsCell(destCells, w.cell))
        {
            // Cell is part of the moving block's new home. Selection follows.
            host.setCellDragRoleDestination(w.cell, true);
            host.setCellDragRoleSource     (w.cell, false);   // overlap with source: prefer destination role
        }
        else if (w.file != juce::File{})
        {
            // Source-only cell receiving a displaced file → thicker border accent.
            host.setCellDragRoleDisplace(w.cell, true);
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

    // Clamp + map in visual space, mirroring update()'s path.
    const auto visualDims = host.getVisualDims();
    DragOp visualOp;
    visualOp.pickupCell = host.toVisual(pickupCell);
    visualOp.dropCell   = host.toVisual(lastDropCell);
    visualOp.sourceCells.ensureStorageAllocated(sourceCells.size());
    for (const auto& s : sourceCells) visualOp.sourceCells.add(host.toVisual(s));

    visualOp = clampOpToGrid(visualOp, visualDims, 0, visualDims.numBanks - 1);

    const auto vDelta = visualOp.delta();
    if (vDelta == GridCell{0, 0})
    {
        cancel();
        return;
    }

    DragOp op;
    op.sourceCells = sourceCells;
    op.pickupCell  = pickupCell;
    op.dropCell    = host.fromVisual(visualOp.dropCell);
    op.destCells.ensureStorageAllocated(sourceCells.size());
    for (const auto& vs : visualOp.sourceCells)
        op.destCells.add(host.fromVisual(vs + vDelta));

    host.onDragCommitWillBegin();

    auto accessor = [this](GridCell c) { return host.getFileAt(c); };
    auto writes   = computeDragResult(accessor, op, host.getGridDims());

    juce::Array<juce::File> resolvedFiles;
    resolvedFiles.ensureStorageAllocated(writes.size());
    for (const auto& w : writes)
        resolvedFiles.add(w.file);

    for (int i = 0; i < writes.size(); ++i)
        host.setFileAt(writes[i].cell, resolvedFiles[i]);

    // New selection follows the moved block — same map.
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
