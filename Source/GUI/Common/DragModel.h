// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_core/juce_core.h>
#include <functional>

//==============================================================================
// DragModel — pure types and functions for the unified drag-and-drop system.
//
// No JUCE Component dependencies. Both BankEditorPanel (Pack mode, 5×14 grid)
// and BankFocusPanel (Bank mode, single-bank 14-row strip) drive their drag
// behavior through this module so they share one displacement algorithm,
// clamp rule, and write resolution.
//==============================================================================
namespace LunchBoxDrag
{
    struct GridCell
    {
        int bank = -1;
        int slot = -1;

        bool isValid()    const                  { return bank >= 0 && slot >= 0; }
        bool operator==(const GridCell& o) const { return bank == o.bank && slot == o.slot; }
        bool operator!=(const GridCell& o) const { return !(*this == o); }
        bool operator< (const GridCell& o) const { return bank < o.bank || (bank == o.bank && slot < o.slot); }

        GridCell operator+(const GridCell& d) const { return { bank + d.bank, slot + d.slot }; }
        GridCell operator-(const GridCell& d) const { return { bank - d.bank, slot - d.slot }; }
    };

    struct GridDims
    {
        int numBanks     = 0;
        int slotsPerBank = 0;

        bool inBounds(GridCell c) const
        {
            return c.bank >= 0 && c.bank < numBanks
                && c.slot >= 0 && c.slot < slotsPerBank;
        }

        // Global reading-order index. Used to sort cells for the displacement fallback.
        int globalIndex(GridCell c) const { return c.bank * slotsPerBank + c.slot; }
    };

    struct DragOp
    {
        juce::Array<GridCell> sourceCells;  // cells the user is moving (data coords)
        juce::Array<GridCell> destCells;    // 1:1 with sourceCells; computed in visual space then mapped back

        GridCell pickupCell;                // cell the mouseDown started on (data coords)
        GridCell dropCell;                  // cell the cursor is over now (data coords)

        GridCell delta() const { return dropCell - pickupCell; }
    };

    struct Write
    {
        GridCell cell;
        juce::File file;  // empty File ⇒ explicit clear
    };

    using DragResult = juce::Array<Write>;

    // Clamp the op's effective delta so the source-cell bounding box, translated,
    // stays inside [bankLo..bankHi] × [0..slotsPerBank-1].
    //   Pack mode: bankLo = 0,          bankHi = numBanks-1
    //   Bank mode: bankLo = activeBank, bankHi = activeBank   (delta.bank pinned to 0)
    // Returns a new DragOp with `dropCell` adjusted; sourceCells/pickupCell unchanged.
    DragOp clampOpToGrid(DragOp op, GridDims dims, int bankLo, int bankHi);

    // GridAccessor returns the file currently at a given cell. Out-of-bounds reads
    // should return an empty File{}.
    using GridAccessor = std::function<juce::File(GridCell)>;

    // Compute every write needed to commit this drag.
    //
    // Algorithm — reverse-shift with row-major fallback:
    //   1. The moving block: each source cell's file is written to its
    //      corresponding destCell (empties travel — geometry preserved).
    //   2. Each filled target-only cell is "displaced". Its file flows BACK
    //      against the drag direction: the displaced file at destCells[i]
    //      lands at sourceCells[i] (the cell that emptied to make room).
    //      Source cells that were empty in the source still receive displaced
    //      content — "empties collapse" into the incoming files.
    //   3. Overlap fallback: if sourceCells[i] is also in destCells (so it's
    //      not source-only and can't hold the displaced file), the file goes
    //      to the next available source-only cell in global-index order.
    //   4. Source-only cells that didn't receive any displaced file get an
    //      explicit empty write (they were vacated by the move).
    //
    // Caller must clamp the op first via clampOpToGrid(). Filled-count is invariant.
    DragResult computeDragResult(const GridAccessor& grid, const DragOp& op, GridDims dims);
}
