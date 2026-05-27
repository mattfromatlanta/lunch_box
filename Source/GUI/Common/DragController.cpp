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

// Helper: build a DragOp in DATA coords with destCells precomputed via the
// host's visual mapping (so Pack wraps correctly). Returns nullopt when the
// clamped drop equals the pickup (no movement).
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
    juce::ignoreUnused(cursorPanelPt);   // proxy disabled

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

    juce::Array<juce::File> resolvedFiles;
    resolvedFiles.ensureStorageAllocated(writes.size());
    for (const auto& w : writes) resolvedFiles.add(w.file);

    for (int i = 0; i < writes.size(); ++i)
        host.setFileAt(writes[i].cell, resolvedFiles[i]);

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
