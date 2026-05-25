// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_core/juce_core.h>
#include "../Source/GUI/Common/DragModel.h"

using namespace LunchBoxDrag;

namespace
{
    // Build a grid accessor backed by a simple 2D vector. Files are encoded as
    // juce::File paths "/X/<tag>" so the test can identify which file landed
    // where without touching the filesystem.
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
            return f == juce::File{} ? juce::String("_") : f.getFileName();
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

        beginTest ("computeDragResult: single-cell swap via reverse-shift");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A");
            g.put(0, 5, "B");

            DragOp op;
            op.sourceCells = { GridCell{0, 0} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{0, 5};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Reverse-shift: displaced B flows back into the vacated source cell.
            expectEquals(g.tagAt(0, 0), juce::String("B"));
            expectEquals(g.tagAt(0, 5), juce::String("A"));
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

            // B flows back to A's vacated cell (reverse-shift across banks).
            expectEquals(g.tagAt(0, 5), juce::String("B"));
            expectEquals(g.tagAt(2, 5), juce::String("A"));
        }

        beginTest ("computeDragResult: cross-bank block swap");
        {
            FakeGrid g(dims);
            g.put(0, 3, "A1"); g.put(0, 4, "A2"); g.put(0, 5, "A3");
            g.put(1, 3, "B1"); g.put(1, 4, "B2"); g.put(1, 5, "B3");

            DragOp op;
            op.sourceCells = { GridCell{0, 3}, GridCell{0, 4}, GridCell{0, 5} };
            op.pickupCell  = GridCell{0, 3};
            op.dropCell    = GridCell{1, 3};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // A1..A3 land at bank 1. B1..B3 reverse-shift to bank 0 (each B[i] back
            // to where the corresponding A[i] came from). Clean block swap.
            expectEquals(g.tagAt(0, 3), juce::String("B1"));
            expectEquals(g.tagAt(0, 4), juce::String("B2"));
            expectEquals(g.tagAt(0, 5), juce::String("B3"));
            expectEquals(g.tagAt(1, 3), juce::String("A1"));
            expectEquals(g.tagAt(1, 4), juce::String("A2"));
            expectEquals(g.tagAt(1, 5), juce::String("A3"));
        }

        beginTest ("computeDragResult: discontiguous selection preserves spatial swap");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A"); g.put(2, 5, "B");
            g.put(1, 1, "X"); g.put(3, 6, "Y");

            DragOp op;
            op.sourceCells = { GridCell{0, 0}, GridCell{2, 5} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{1, 1};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Each displaced file flows back to its paired source position.
            expectEquals(g.tagAt(0, 0), juce::String("X"));
            expectEquals(g.tagAt(2, 5), juce::String("Y"));
            expectEquals(g.tagAt(1, 1), juce::String("A"));
            expectEquals(g.tagAt(3, 6), juce::String("B"));
        }

        beginTest ("computeDragResult: empty cells in move set absorb displaced files");
        {
            FakeGrid g(dims);
            // Source: A, _, C at (0,0)-(0,2). Target row 1 fully filled with X,Y,Z.
            g.put(0, 0, "A");
            g.put(0, 2, "C");
            g.put(1, 0, "X"); g.put(1, 1, "Y"); g.put(1, 2, "Z");

            DragOp op;
            op.sourceCells = { GridCell{0, 0}, GridCell{0, 1}, GridCell{0, 2} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{1, 0};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Moving block (with its middle empty) lands at row 1.
            expectEquals(g.tagAt(1, 0), juce::String("A"));
            expectEquals(g.tagAt(1, 1), juce::String("_"));
            expectEquals(g.tagAt(1, 2), juce::String("C"));
            // Displaced X,Y,Z reverse-shift back to row 0. The empty middle
            // source cell collapses to receive Y.
            expectEquals(g.tagAt(0, 0), juce::String("X"));
            expectEquals(g.tagAt(0, 1), juce::String("Y"));
            expectEquals(g.tagAt(0, 2), juce::String("Z"));
        }

        beginTest ("computeDragResult: overlapping move uses row-major fallback");
        {
            FakeGrid g(dims);
            g.put(0, 1, "A"); g.put(0, 2, "B"); g.put(0, 3, "C"); g.put(0, 4, "D");

            DragOp op;
            op.sourceCells = { GridCell{0, 1}, GridCell{0, 2}, GridCell{0, 3} };
            op.pickupCell  = GridCell{0, 1};
            op.dropCell    = GridCell{0, 2};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Block A,B,C shifts right by 1. D at (0,4) is target-only; its paired
            // source (0,3) is overlap, so D falls to row-major fallback into the
            // only source-only cell (0,1).
            expectEquals(g.tagAt(0, 1), juce::String("D"));
            expectEquals(g.tagAt(0, 2), juce::String("A"));
            expectEquals(g.tagAt(0, 3), juce::String("B"));
            expectEquals(g.tagAt(0, 4), juce::String("C"));
        }

        beginTest ("computeDragResult: filled-count invariant");
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
