// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DragModel.h"

namespace LunchBoxDrag
{
    namespace
    {
        struct BBox
        {
            int minBank, maxBank, minSlot, maxSlot;
        };

        BBox computeBBox(const juce::Array<GridCell>& cells)
        {
            BBox b { INT_MAX, INT_MIN, INT_MAX, INT_MIN };
            for (const auto& c : cells)
            {
                b.minBank = std::min(b.minBank, c.bank);
                b.maxBank = std::max(b.maxBank, c.bank);
                b.minSlot = std::min(b.minSlot, c.slot);
                b.maxSlot = std::max(b.maxSlot, c.slot);
            }
            return b;
        }
    }

    DragOp clampOpToGrid(DragOp op, GridDims dims, int bankLo, int bankHi)
    {
        // dropCell may be out-of-bounds; clamping is what fixes it. Only bail if there's
        // nothing to clamp (empty selection) or no anchor to measure delta from.
        if (op.sourceCells.isEmpty() || !op.pickupCell.isValid())
            return op;

        auto delta = op.delta();
        auto bb    = computeBBox(op.sourceCells);

        // Allowed delta range so [min+delta..max+delta] stays in the slot axis [0..slotsPerBank-1]
        // and in the bank axis [bankLo..bankHi].
        int minDeltaBank = bankLo - bb.minBank;
        int maxDeltaBank = bankHi - bb.maxBank;
        int minDeltaSlot = 0     - bb.minSlot;
        int maxDeltaSlot = (dims.slotsPerBank - 1) - bb.maxSlot;

        delta.bank = juce::jlimit(minDeltaBank, maxDeltaBank, delta.bank);
        delta.slot = juce::jlimit(minDeltaSlot, maxDeltaSlot, delta.slot);

        op.dropCell = op.pickupCell + delta;
        return op;
    }

    DragResult computeDragResult(const GridAccessor& grid, const DragOp& op, GridDims dims)
    {
        DragResult writes;
        if (op.sourceCells.isEmpty()) return writes;

        // Use precomputed destCells if the caller supplied them (controller path —
        // accounts for visual-coord wrapping). Otherwise fall back to data-space
        // translation by `delta` (test path, single-coord-system grids).
        juce::Array<GridCell> destCells;
        if (op.destCells.size() == op.sourceCells.size())
        {
            destCells = op.destCells;
        }
        else
        {
            const auto delta = op.delta();
            destCells.ensureStorageAllocated(op.sourceCells.size());
            for (const auto& s : op.sourceCells)
                destCells.add(s + delta);
        }

        auto contains = [](const juce::Array<GridCell>& arr, GridCell c)
        {
            for (const auto& x : arr) if (x == c) return true;
            return false;
        };

        // ── Step 1: Moving block. Each dest cell receives the file from its
        //    corresponding source (empties in the source travel as empties).
        for (int i = 0; i < op.sourceCells.size(); ++i)
        {
            const auto d = destCells[i];
            if (!dims.inBounds(d)) continue;
            writes.add({ d, grid(op.sourceCells[i]) });
        }

        // ── Step 2: Source-only cells (vacated by the move). These are where
        //    displaced files will land via reverse-shift.
        juce::Array<GridCell> remainingSourceOnly;
        for (const auto& s : op.sourceCells)
            if (! contains(destCells, s))
                remainingSourceOnly.add(s);

        // ── Step 3: Reverse-shift each filled target-only cell to its paired
        //    source cell. Pairing comes from the index-aligned sourceCells/destCells
        //    arrays. Files for overlap-source pairs (where the paired source is
        //    itself in destCells) are collected for the row-major fallback.
        juce::Array<juce::File> unplacedFiles;

        for (int i = 0; i < op.sourceCells.size(); ++i)
        {
            const auto d = destCells[i];
            if (contains(op.sourceCells, d)) continue;   // overlap, not target-only
            const auto file = grid(d);
            if (file == juce::File{}) continue;           // not filled, nothing to displace

            const auto s = op.sourceCells[i];

            int idx = -1;
            for (int k = 0; k < remainingSourceOnly.size(); ++k)
                if (remainingSourceOnly[k] == s) { idx = k; break; }

            if (idx >= 0)
            {
                writes.add({ s, file });
                remainingSourceOnly.remove(idx);
            }
            else
            {
                // s is overlap (also in destCells). The displaced file can't land
                // there. Defer to row-major fallback among remaining source-only.
                unplacedFiles.add(file);
            }
        }

        // ── Step 4: Row-major fallback for any unplaced (overlap) displaced files.
        std::sort(remainingSourceOnly.begin(), remainingSourceOnly.end(),
                  [&](GridCell a, GridCell b)
                  { return dims.globalIndex(a) < dims.globalIndex(b); });

        const int pairs = juce::jmin(unplacedFiles.size(), remainingSourceOnly.size());
        for (int k = 0; k < pairs; ++k)
            writes.add({ remainingSourceOnly[k], unplacedFiles[k] });

        // ── Step 5: Any source-only cell that didn't receive a displaced file
        //    gets an explicit empty write (it was filled originally; vacated now).
        for (int k = pairs; k < remainingSourceOnly.size(); ++k)
            writes.add({ remainingSourceOnly[k], juce::File{} });

        return writes;
    }
}
