// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_core/juce_core.h>
#include "../Source/GUI/Common/DragModel.h"

using namespace LunchBoxDrag;

namespace
{
    // Build a grid accessor backed by a simple 2D vector. Files are encoded as
    // juce::File paths "/X/<bank>_<slot>" so the test can identify which file
    // landed where without touching the filesystem.
    struct FakeGrid
    {
        GridDims dims;
        juce::Array<juce::Array<juce::File>> data;   // [bank][slot]

        FakeGrid(GridDims d) : dims(d)
        {
            for (int b = 0; b < d.numBanks; ++b)
            {
                juce::Array<juce::File> row;
                for (int s = 0; s < d.slotsPerBank; ++s) row.add(juce::File{});
                data.add(row);
            }
        }

        void put(int b, int s, const juce::String& tag)
        {
            data.getReference(b).set(s, juce::File("/X/" + tag));
        }

        GridAccessor accessor() const
        {
            return [this](GridCell c) -> juce::File
            {
                if (! dims.inBounds(c)) return {};
                return data[c.bank][c.slot];
            };
        }

        void apply(const DragResult& writes)
        {
            for (const auto& w : writes)
                data.getReference(w.cell.bank).set(w.cell.slot, w.file);
        }

        juce::String tagAt(int b, int s) const
        {
            auto f = data[b][s];
            return f == juce::File{} ? juce::String("_")
                                     : f.getFileName();
        }

        int filledCount() const
        {
            int n = 0;
            for (int b = 0; b < dims.numBanks; ++b)
                for (int s = 0; s < dims.slotsPerBank; ++s)
                    if (data[b][s] != juce::File{}) ++n;
            return n;
        }
    };
}

class DragModelTests : public juce::UnitTest
{
public:
    DragModelTests() : juce::UnitTest ("DragModel") {}

    void runTest() override
    {
        const GridDims dims { 5, 14 };

        beginTest ("clampOpToGrid: unconstrained delta passes through");
        {
            DragOp op;
            op.sourceCells = { GridCell{1, 5} };
            op.pickupCell  = GridCell{1, 5};
            op.dropCell    = GridCell{2, 7};
            auto c = clampOpToGrid(op, dims, 0, 4);
            expect(c.dropCell == GridCell{2, 7});
        }

        beginTest ("clampOpToGrid: single cell can't leave the grid");
        {
            DragOp op;
            op.sourceCells = { GridCell{0, 0} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{-3, -3};
            auto c = clampOpToGrid(op, dims, 0, 4);
            expect(c.dropCell == GridCell{0, 0});
        }

        beginTest ("clampOpToGrid: tall block clamped so bbox stays in bounds");
        {
            DragOp op;
            op.sourceCells = { GridCell{0, 5}, GridCell{1, 5}, GridCell{2, 5} };
            op.pickupCell  = GridCell{0, 5};
            op.dropCell    = GridCell{5, 5};
            auto c = clampOpToGrid(op, dims, 0, 4);
            expect(c.dropCell == GridCell{2, 5});
        }

        beginTest ("clampOpToGrid: discontiguous bbox blocks motion");
        {
            DragOp op;
            op.sourceCells = { GridCell{0, 0}, GridCell{4, 13} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{1, 1};
            auto c = clampOpToGrid(op, dims, 0, 4);
            expect(c.dropCell == GridCell{0, 0});
        }

        beginTest ("clampOpToGrid: Bank-mode bank-range pin (lo==hi)");
        {
            DragOp op;
            op.sourceCells = { GridCell{1, 5} };
            op.pickupCell  = GridCell{1, 5};
            op.dropCell    = GridCell{3, 8};
            auto c = clampOpToGrid(op, dims, 1, 1);
            expect(c.dropCell == GridCell{1, 8});
        }

        beginTest ("computeDragResult: move to empty target");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A");

            DragOp op;
            op.sourceCells = { GridCell{0, 0} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{0, 5};
            g.apply(computeDragResult(g.accessor(), op, dims));

            expectEquals(g.tagAt(0, 0), juce::String("_"));
            expectEquals(g.tagAt(0, 5), juce::String("A"));
        }

        beginTest ("computeDragResult: single-cell drag onto filled cell pushes displaced forward");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A");
            g.put(0, 5, "B");

            DragOp op;
            op.sourceCells = { GridCell{0, 0} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{0, 5};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Stack insertion: A lands at (0,5). B's home was (0,5), it pushes forward
            // to the first empty available cell — (0,6).
            expectEquals(g.tagAt(0, 0), juce::String("_"));   // source vacated, no swap-back
            expectEquals(g.tagAt(0, 5), juce::String("A"));
            expectEquals(g.tagAt(0, 6), juce::String("B"));
        }

        beginTest ("computeDragResult: cascade when next slot is filled");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A");
            g.put(0, 3, "B");
            g.put(0, 4, "C");   // immediately after the drop target — must shift forward

            DragOp op;
            op.sourceCells = { GridCell{0, 0} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{0, 3};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // A lands at (0,3). B inserts at (0,4) (was C's slot) — pushes C to (0,5).
            expectEquals(g.tagAt(0, 0), juce::String("_"));
            expectEquals(g.tagAt(0, 3), juce::String("A"));
            expectEquals(g.tagAt(0, 4), juce::String("B"));
            expectEquals(g.tagAt(0, 5), juce::String("C"));
        }

        beginTest ("computeDragResult: cross-bank single drag");
        {
            FakeGrid g(dims);
            g.put(0, 5, "A");
            g.put(2, 5, "B");

            DragOp op;
            op.sourceCells = { GridCell{0, 5} };
            op.pickupCell  = GridCell{0, 5};
            op.dropCell    = GridCell{2, 5};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // A lands at (2,5). B pushes forward to (2,6).
            expectEquals(g.tagAt(0, 5), juce::String("_"));
            expectEquals(g.tagAt(2, 5), juce::String("A"));
            expectEquals(g.tagAt(2, 6), juce::String("B"));
        }

        beginTest ("computeDragResult: cross-bank multi-cell drag with cascading displacement");
        {
            FakeGrid g(dims);
            g.put(0, 3, "A1"); g.put(0, 4, "A2"); g.put(0, 5, "A3");
            g.put(1, 3, "B1"); g.put(1, 4, "B2"); g.put(1, 5, "B3");

            DragOp op;
            op.sourceCells = { GridCell{0, 3}, GridCell{0, 4}, GridCell{0, 5} };
            op.pickupCell  = GridCell{0, 3};
            op.dropCell    = GridCell{1, 3};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Source bank-0 cells vacate. Bank-1 receives A1,A2,A3. The original B1,B2,B3
            // each stack-insert forward from their origin — B1 lands at (1,6), then B2
            // cascades it to (1,7), then B3 cascades both to (1,8).
            expectEquals(g.tagAt(0, 3), juce::String("_"));
            expectEquals(g.tagAt(0, 4), juce::String("_"));
            expectEquals(g.tagAt(0, 5), juce::String("_"));
            expectEquals(g.tagAt(1, 3), juce::String("A1"));
            expectEquals(g.tagAt(1, 4), juce::String("A2"));
            expectEquals(g.tagAt(1, 5), juce::String("A3"));
            expectEquals(g.tagAt(1, 6), juce::String("B3"));
            expectEquals(g.tagAt(1, 7), juce::String("B2"));
            expectEquals(g.tagAt(1, 8), juce::String("B1"));
        }

        beginTest ("computeDragResult: empty cells in move set travel with selection");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A");
            g.put(0, 2, "C");                     // (0,1) intentionally empty
            g.put(1, 0, "X"); g.put(1, 1, "Y"); g.put(1, 2, "Z");

            DragOp op;
            op.sourceCells = { GridCell{0, 0}, GridCell{0, 1}, GridCell{0, 2} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{1, 0};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Bank 1 receives the moved row including the empty in the middle.
            expectEquals(g.tagAt(1, 0), juce::String("A"));
            expectEquals(g.tagAt(1, 1), juce::String("_"));
            expectEquals(g.tagAt(1, 2), juce::String("C"));
            // Source cells fully vacate.
            expectEquals(g.tagAt(0, 0), juce::String("_"));
            expectEquals(g.tagAt(0, 1), juce::String("_"));
            expectEquals(g.tagAt(0, 2), juce::String("_"));
            // Displaced X,Y,Z cascade forward into bank 1 slots 3+, in reverse-insertion order.
            expectEquals(g.tagAt(1, 3), juce::String("Z"));
            expectEquals(g.tagAt(1, 4), juce::String("Y"));
            expectEquals(g.tagAt(1, 5), juce::String("X"));
        }

        beginTest ("computeDragResult: overlapping move shifts block + cascades one cell");
        {
            FakeGrid g(dims);
            g.put(0, 1, "A"); g.put(0, 2, "B"); g.put(0, 3, "C"); g.put(0, 4, "D");

            DragOp op;
            op.sourceCells = { GridCell{0, 1}, GridCell{0, 2}, GridCell{0, 3} };
            op.pickupCell  = GridCell{0, 1};
            op.dropCell    = GridCell{0, 2};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Block A,B,C shifts right by 1. D was at (0,4); it pushes forward to (0,5).
            // Source-only cell (0,1) vacates.
            expectEquals(g.tagAt(0, 1), juce::String("_"));
            expectEquals(g.tagAt(0, 2), juce::String("A"));
            expectEquals(g.tagAt(0, 3), juce::String("B"));
            expectEquals(g.tagAt(0, 4), juce::String("C"));
            expectEquals(g.tagAt(0, 5), juce::String("D"));
        }

        beginTest ("computeDragResult: filled-count invariant under stack insertion");
        {
            FakeGrid g(dims);
            g.put(0, 0, "F0"); g.put(0, 5, "F1"); g.put(1, 2, "F2");
            g.put(2, 7, "F3"); g.put(3, 1, "F4"); g.put(4, 9, "F5"); g.put(4, 13, "F6");

            DragOp op;
            op.sourceCells = { GridCell{1, 2}, GridCell{1, 7}, GridCell{2, 2}, GridCell{2, 7} };
            op.pickupCell  = GridCell{1, 2};
            op.dropCell    = GridCell{2, 2};
            g.apply(computeDragResult(g.accessor(), op, dims));

            expectEquals(g.filledCount(), 7);
        }

        beginTest ("computeDragResult: zero-delta drop is a no-op");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A"); g.put(0, 1, "B");

            DragOp op;
            op.sourceCells = { GridCell{0, 0}, GridCell{0, 1} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{0, 0};
            g.apply(computeDragResult(g.accessor(), op, dims));

            expectEquals(g.tagAt(0, 0), juce::String("A"));
            expectEquals(g.tagAt(0, 1), juce::String("B"));
        }
    }
};

static DragModelTests dragModelTests;
