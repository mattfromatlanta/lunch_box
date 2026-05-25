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
        return;

    const auto dropCellData = rawDrop.isValid() ? rawDrop : lastDropCell;

    // Clamp in VISUAL space — the user perceives a row/col grid that may not
    // match the bank/slot data axes (Pack wraps 14 slots into 7×2 visually).
    const auto visualDims = host.getVisualDims();
    DragOp visualOp;
    visualOp.pickupCell = host.toVisual(pickupCell);
    visualOp.dropCell   = host.toVisual(dropCellData);
    visualOp.sourceCells.ensureStorageAllocated(sourceCells.size());
    for (const auto& s : sourceCells) visualOp.sourceCells.add(host.toVisual(s));

    visualOp = clampOpToGrid(visualOp, visualDims, 0, visualDims.numBanks - 1);

    const auto clampedDataDrop = host.fromVisual(visualOp.dropCell);
    juce::ignoreUnused(cursorPanelPt);   // proxy disabled

    if (clampedDataDrop == lastDropCell) return;
    lastDropCell = clampedDataDrop;

    rebuildPreviewsFor(visualOp.pickupCell, visualOp.dropCell);
}

DragController::StepwiseResult
DragController::computeStepwiseFor(GridCell visualPickup, GridCell visualDrop) const
{
    StepwiseResult result;

    const auto dataDims  = host.getGridDims();
    const auto keyOf     = [&](GridCell c) { return c.bank * dataDims.slotsPerBank + c.slot; };

    auto stateRead = [&](GridCell c) -> juce::File
    {
        auto it = result.finalState.find(keyOf(c));
        return it != result.finalState.end() ? it->second : host.getFileAt(c);
    };

    auto stateWrite = [&](GridCell c, const juce::File& f)
    {
        const auto k    = keyOf(c);
        const auto orig = host.getFileAt(c);
        if (f == orig) result.finalState.erase(k);
        else           result.finalState[k] = f;
    };

    // Iterate one-cell steps from pickup toward drop, in visual space.
    juce::Array<GridCell> curSources = sourceCells;
    GridCell remaining { visualDrop.bank - visualPickup.bank,
                         visualDrop.slot - visualPickup.slot };

    auto sign = [](int x) { return (x > 0) - (x < 0); };

    while (! (remaining == GridCell{0, 0}))
    {
        GridCell step { sign(remaining.bank), sign(remaining.slot) };

        // Next sources via visual step
        juce::Array<GridCell> nextSources;
        nextSources.ensureStorageAllocated(curSources.size());
        for (const auto& s : curSources)
            nextSources.add(host.fromVisual(host.toVisual(s) + step));

        // One-step reverse-shift against the working state
        DragOp stepOp;
        stepOp.sourceCells = curSources;
        stepOp.destCells   = nextSources;

        auto stepWrites = computeDragResult(stateRead, stepOp, dataDims);
        for (const auto& w : stepWrites)
            stateWrite(w.cell, w.file);

        curSources = nextSources;
        remaining = GridCell{ remaining.bank - step.bank, remaining.slot - step.slot };
    }

    result.finalSourceCells = curSources;
    return result;
}

void DragController::rebuildPreviewsFor(GridCell visualPickup, GridCell visualDrop)
{
    host.clearAllCellPreviews();

    const auto stepwise = computeStepwiseFor(visualPickup, visualDrop);
    const auto& finalDests = stepwise.finalSourceCells;

    auto containsCellLocal = [](const juce::Array<GridCell>& arr, GridCell c)
    {
        for (const auto& x : arr) if (x == c) return true;
        return false;
    };

    const auto dataDims = host.getGridDims();

    // Vacated source cells: any initial source not in finalDests.
    for (const auto& s : sourceCells)
    {
        if (containsCellLocal(finalDests, s)) continue;
        host.setCellPreview(s, juce::File{});
        host.setCellDragRoleSource(s, true);
    }

    // Render every cell that the stepwise pass modified.
    for (const auto& [key, file] : stepwise.finalState)
    {
        GridCell c { key / dataDims.slotsPerBank, key % dataDims.slotsPerBank };
        host.setCellPreview(c, file);

        if (containsCellLocal(finalDests, c))
        {
            host.setCellDragRoleDestination(c, true);
            host.setCellDragRoleSource     (c, false);
        }
        else if (file != juce::File{})
        {
            // Cell received displaced content along the cascade chain.
            host.setCellDragRoleDisplace(c, true);
            host.setCellDragRoleSource  (c, false);
        }
    }
}

#if 0
void DragController::rebuildPreviewsForLegacy(const DragOp& op)
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
#endif

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

    // Compute the stepwise result against the real grid, then commit it.
    const auto stepwise = computeStepwiseFor(visualOp.pickupCell, visualOp.dropCell);

    host.onDragCommitWillBegin();

    const auto dataDims = host.getGridDims();
    for (const auto& [key, file] : stepwise.finalState)
    {
        GridCell c { key / dataDims.slotsPerBank, key % dataDims.slotsPerBank };
        host.setFileAt(c, file);
    }

    juce::Array<GridCell> newSelection = stepwise.finalSourceCells;

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
