// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_core/juce_core.h>
#include "../Source/GUI/Common/DragModel.h"

using namespace LunchBoxDrag;

namespace
{
    struct FakeGrid
    {
        GridDims dims;
        juce::Array<juce::Array<juce::File>> data;

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

        beginTest ("computeDragResult: forward drag displaces backward toward source");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A");
            g.put(0, 5, "B");

            DragOp op;
            op.sourceCells = { GridCell{0, 0} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{0, 5};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Source A1 (index 0) < dest A6 (index 5): displaced B flows BACKWARD.
            // First empty available cell below index 5 is A5 (index 4).
            expectEquals(g.tagAt(0, 0), juce::String("_"));
            expectEquals(g.tagAt(0, 4), juce::String("B"));
            expectEquals(g.tagAt(0, 5), juce::String("A"));
        }

        beginTest ("computeDragResult: A1 drag to B1 displaces B1 to A14");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A");   // A1
            g.put(1, 0, "B");   // B1

            DragOp op;
            op.sourceCells = { GridCell{0, 0} };   // A1 index 0
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{1, 0};        // B1 index 14
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Source A1 (0) < dest B1 (14): B1 flows backward.
            // First empty available cell with index < 14 is A14 (index 13).
            expectEquals(g.tagAt(0, 0), juce::String("_"));
            expectEquals(g.tagAt(0, 13), juce::String("B"));  // A14
            expectEquals(g.tagAt(1, 0), juce::String("A"));   // B1
        }

        beginTest ("computeDragResult: backward drag displaces forward");
        {
            FakeGrid g(dims);
            g.put(0, 5, "A");
            g.put(0, 1, "B");

            DragOp op;
            op.sourceCells = { GridCell{0, 5} };
            op.pickupCell  = GridCell{0, 5};
            op.dropCell    = GridCell{0, 1};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Source A6 (5) > dest A2 (1): B flows FORWARD.
            // First empty above index 1 is A3 (index 2).
            expectEquals(g.tagAt(0, 5), juce::String("_"));
            expectEquals(g.tagAt(0, 1), juce::String("A"));
            expectEquals(g.tagAt(0, 2), juce::String("B"));
        }

        beginTest ("computeDragResult: backward cascade: displaced pushes filled cells backward");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A");
            g.put(0, 3, "B");
            g.put(0, 4, "C");

            DragOp op;
            op.sourceCells = { GridCell{0, 0} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{0, 3};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Source A1 (0) < dest A4 (3): B flows backward from A4.
            // First available below index 3: A3 (2) empty → B lands there.
            // A5 (C) not displaced — it wasn't at the destination.
            expectEquals(g.tagAt(0, 0), juce::String("_"));
            expectEquals(g.tagAt(0, 2), juce::String("B"));
            expectEquals(g.tagAt(0, 3), juce::String("A"));
            expectEquals(g.tagAt(0, 4), juce::String("C"));
        }

        beginTest ("computeDragResult: source-only cell absorbs backward cascade");
        {
            FakeGrid g(dims);
            // Fill A2..A6 so backward cascade from A6 must push through them,
            // eventually consuming A1 (the vacated source).
            g.put(0, 0, "A");
            for (int s = 1; s < 6; ++s) g.put(0, s, "F" + juce::String(s));

            DragOp op;
            op.sourceCells = { GridCell{0, 0} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{0, 5};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // A lands at A6. Displaced F5 flows backward: hits F4 at A5 (pushes to
            // A4), then F3 at A4 (pushes to A3), then F2 at A3 (pushes to A2), then
            // F1 at A2 (pushes to A1 = source-only, empty → absorbs).
            expectEquals(g.tagAt(0, 5), juce::String("A"));
            expectEquals(g.filledCount(), 6);  // unchanged from before drag
        }

        beginTest ("computeDragResult: cross-bank single drag forward");
        {
            FakeGrid g(dims);
            g.put(0, 5, "A");
            g.put(2, 5, "B");

            DragOp op;
            op.sourceCells = { GridCell{0, 5} };
            op.pickupCell  = GridCell{0, 5};
            op.dropCell    = GridCell{2, 5};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // A at (0,5) idx=5, dest (2,5) idx=33. Forward drag → B displaced BACKWARD.
            // First available with index < 33 is (2,4) = C5 (empty).
            expectEquals(g.tagAt(0, 5), juce::String("_"));
            expectEquals(g.tagAt(2, 4), juce::String("B"));
            expectEquals(g.tagAt(2, 5), juce::String("A"));
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

            // All drags are forward (src idx 3,4,5 < dest idx 17,18,19).
            // Each Bx flows BACKWARD. First available < 17 is (1,2)=B3-slot (idx 16, empty).
            // B1 → (1,2). B2 → (1,2), cascades B1 → (1,1). B3 → (1,2), cascades B2→(1,1), B1→(1,0).
            // Order preserved: B1 at (1,0), B2 at (1,1), B3 at (1,2).
            expectEquals(g.tagAt(0, 3), juce::String("_"));
            expectEquals(g.tagAt(0, 4), juce::String("_"));
            expectEquals(g.tagAt(0, 5), juce::String("_"));
            expectEquals(g.tagAt(1, 3), juce::String("A1"));
            expectEquals(g.tagAt(1, 4), juce::String("A2"));
            expectEquals(g.tagAt(1, 5), juce::String("A3"));
            expectEquals(g.tagAt(1, 0), juce::String("B1"));
            expectEquals(g.tagAt(1, 1), juce::String("B2"));
            expectEquals(g.tagAt(1, 2), juce::String("B3"));
        }

        beginTest ("computeDragResult: empty target cells don't cause displacement");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A");
            // A6 is empty
            DragOp op;
            op.sourceCells = { GridCell{0, 0} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{0, 5};
            g.apply(computeDragResult(g.accessor(), op, dims));

            expectEquals(g.filledCount(), 1);
            expectEquals(g.tagAt(0, 5), juce::String("A"));
        }

        beginTest ("computeDragResult: empty cells in move set travel");
        {
            FakeGrid g(dims);
            g.put(0, 0, "A");
            g.put(0, 2, "C");
            g.put(1, 0, "X"); g.put(1, 1, "Y"); g.put(1, 2, "Z");

            DragOp op;
            op.sourceCells = { GridCell{0, 0}, GridCell{0, 1}, GridCell{0, 2} };
            op.pickupCell  = GridCell{0, 0};
            op.dropCell    = GridCell{1, 0};
            g.apply(computeDragResult(g.accessor(), op, dims));

            // Block (A,_,C) lands at row 1 cols 0-2 (with the empty in middle).
            expectEquals(g.tagAt(1, 0), juce::String("A"));
            expectEquals(g.tagAt(1, 1), juce::String("_"));
            expectEquals(g.tagAt(1, 2), juce::String("C"));
            // X,Y,Z displaced forward — first non-dest cell at higher index is
            // (1,3),(1,4),(1,5) — empties absorb each.
            // (Y at (1,1) — its target (1,1) is dest with empty content, so Y is
            //  the displaced file; flows forward from (1,1) → (1,3).
            //  But the forward cascade from each origin processes in order.)
            // Source cells (0,0)-(0,2) end up empty.
            expectEquals(g.tagAt(0, 0), juce::String("_"));
            expectEquals(g.tagAt(0, 1), juce::String("_"));
            expectEquals(g.tagAt(0, 2), juce::String("_"));
            expectEquals(g.filledCount(), 5);
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
