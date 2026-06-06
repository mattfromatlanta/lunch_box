// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DragModel.h"
#include <algorithm>

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
        // translation by `delta` (test path).
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

        // ── Step 1: The moving block. Each dest cell receives the file from
        //    its corresponding source (empties travel — geometry preserved).
        for (int i = 0; i < op.sourceCells.size(); ++i)
        {
            const auto d = destCells[i];
            if (!dims.inBounds(d)) continue;
            writes.add({ d, grid(op.sourceCells[i]) });
        }

        // ── Step 2: Build available-cells list (everything not in destCells).
        //    Iteration is row-major so the list is already in global-index order.
        juce::Array<GridCell> available;
        available.ensureStorageAllocated(dims.numBanks * dims.slotsPerBank - destCells.size());
        for (int b = 0; b < dims.numBanks; ++b)
            for (int s = 0; s < dims.slotsPerBank; ++s)
            {
                GridCell c { b, s };
                if (! contains(destCells, c)) available.add(c);
            }

        // Initial state: source-only cells are vacated (empty); other available
        // cells keep their current grid value. Cascade mutates this array.
        juce::Array<juce::File> state;
        state.resize(available.size());
        for (int i = 0; i < available.size(); ++i)
            state.set(i, contains(op.sourceCells, available[i]) ? juce::File{}
                                                                : grid(available[i]));

        // ── Step 3: Collect displaced files with per-pair direction.
        struct Displaced { GridCell origin; juce::File file; int step; };
        juce::Array<Displaced> displaced;
        for (int i = 0; i < op.sourceCells.size(); ++i)
        {
            const auto d = destCells[i];
            if (contains(op.sourceCells, d)) continue;   // overlap, not target-only
            const auto file = grid(d);
            if (file == juce::File{}) continue;           // empty target — no displacement

            const int destIdx = dims.globalIndex(d);
            const int srcIdx  = dims.globalIndex(op.sourceCells[i]);
            // Displaced file flows back toward where the drag came from.
            // drag forward (destIdx > srcIdx): step = -1 (walk backward in available list)
            // drag backward (destIdx < srcIdx): step = +1 (walk forward)
            const int step = (destIdx > srcIdx) ? -1 : +1;
            displaced.add({ d, file, step });
        }

        // ── Step 4: Sort displaced so cascades land in original relative order.
        //    Forward drag (step=-1): process ascending origin so each pushes lower.
        //    Backward drag (step=+1): process descending origin so each finds empty space
        //    before the next one arrives (avoids reversing relative order).
        if (!displaced.isEmpty())
        {
            const int step = displaced[0].step;
            std::sort(displaced.begin(), displaced.end(),
                      [&](const Displaced& a, const Displaced& b) {
                          const int ai = dims.globalIndex(a.origin);
                          const int bi = dims.globalIndex(b.origin);
                          return step < 0 ? ai < bi : ai > bi;
                      });
        }

        // ── Step 5: Stack-insert each displaced file in its direction through
        //    the available list (wrap-around at the ends).
        const int A = available.size();
        for (const auto& dp : displaced)
        {
            if (A == 0) break;

            const int originGI = dims.globalIndex(dp.origin);

            // Starting index in `available`: first cell strictly past origin
            // in the displacement direction.
            int idx;
            if (dp.step >= 0)
            {
                idx = 0;
                while (idx < A && dims.globalIndex(available[idx]) <= originGI) ++idx;
                if (idx >= A) idx = 0;   // wrap
            }
            else
            {
                idx = A - 1;
                while (idx >= 0 && dims.globalIndex(available[idx]) >= originGI) --idx;
                if (idx < 0) idx = A - 1;   // wrap
            }

            juce::File carry = dp.file;
            for (int hop = 0; hop < A; ++hop)
            {
                auto existing = state[idx];
                state.set(idx, carry);
                if (existing == juce::File{}) { carry = juce::File{}; break; }
                carry = existing;
                idx = ((idx + dp.step) % A + A) % A;   // step with wrap (handles negative)
            }
            // Filled-count invariance guarantees the cascade terminates.
        }

        // ── Step 6: Emit writes for any available cell whose final state
        //    differs from its original grid value.
        for (int i = 0; i < available.size(); ++i)
            if (state[i] != grid(available[i]))
                writes.add({ available[i], state[i] });

        return writes;
    }
}
