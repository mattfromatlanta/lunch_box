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

        const int  N     = dims.numBanks * dims.slotsPerBank;

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

        // ── Step 1: The moving block. Each dest cell receives the corresponding
        //    source file (empties travel with the selection — geometry preserved).
        for (int i = 0; i < op.sourceCells.size(); ++i)
        {
            const auto d = destCells[i];
            if (!dims.inBounds(d)) continue;   // shouldn't happen after clamp
            writes.add({ d, grid(op.sourceCells[i]) });
        }

        // ── Step 2: Build the ordered list of "available" cells — every grid
        //    cell that is NOT a destination, in global-index order. The cascade
        //    walks this list with wrap-around.
        juce::Array<GridCell> available;
        available.ensureStorageAllocated(N - destCells.size());
        for (int b = 0; b < dims.numBanks; ++b)
            for (int s = 0; s < dims.slotsPerBank; ++s)
            {
                GridCell c { b, s };
                if (! contains(destCells, c))
                    available.add(c);
            }

        // Working state of each available cell. Source-only cells start as empty
        // (their content moved to the destination); other cells keep their current
        // grid value. We mutate this array as the cascade runs.
        juce::Array<juce::File> state;
        state.resize(available.size());
        for (int i = 0; i < available.size(); ++i)
            state.set(i, contains(op.sourceCells, available[i]) ? juce::File{}
                                                                : grid(available[i]));

        // ── Step 3: Displaced files = filled target-only cells, sorted by
        //    their original global index for stable cascading.
        struct Displaced { GridCell origin; juce::File file; };
        juce::Array<Displaced> displaced;
        for (const auto& t : destCells)
        {
            if (contains(op.sourceCells, t)) continue;   // overlap: not target-only
            auto f = grid(t);
            if (f != juce::File{}) displaced.add({ t, f });
        }
        std::sort(displaced.begin(), displaced.end(),
                  [&](const Displaced& a, const Displaced& b)
                  { return dims.globalIndex(a.origin) < dims.globalIndex(b.origin); });

        // ── Step 4: Stack-insert each displaced file into the available list.
        //    Start at the first available cell with global index >= origin (wrap
        //    if past end). Insert; if the slot was filled, carry that file forward
        //    and repeat until an empty available cell absorbs the cascade.
        const int A = available.size();
        for (const auto& dp : displaced)
        {
            if (A == 0) break;

            const int originGI = dims.globalIndex(dp.origin);
            int idx = 0;
            while (idx < A && dims.globalIndex(available[idx]) < originGI) ++idx;
            if (idx >= A) idx = 0;   // wrap

            juce::File carry = dp.file;
            for (int hop = 0; hop < A; ++hop)
            {
                auto existing = state[idx];
                state.set(idx, carry);
                if (existing == juce::File{}) { carry = juce::File{}; break; }
                carry = existing;
                idx = (idx + 1) % A;
            }
            // If we still carry after wrapping the whole list, filled-count was
            // violated upstream. The carry is silently dropped — should never happen.
        }

        // ── Step 5: Emit writes for any available cell whose final state differs
        //    from its original grid value.
        for (int i = 0; i < available.size(); ++i)
            if (state[i] != grid(available[i]))
                writes.add({ available[i], state[i] });

        return writes;
    }
}
