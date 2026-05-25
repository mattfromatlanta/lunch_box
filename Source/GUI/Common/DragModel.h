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
    // Algorithm — stack-insertion through available cells:
    //   1. The moving block: each source cell's file is written to source+delta
    //      (empties travel with the selection, geometry preserved).
    //   2. "Available cells" = every cell NOT in the destination set, walked in
    //      global-index order with wrap. Source-only cells are conceptually empty
    //      (their content moved to the destination) and are part of available.
    //   3. Each filled target-only cell's original content is "displaced". Sorted
    //      by original global index, each displaced file is stack-inserted into
    //      available, starting at the first available cell whose global index is
    //      >= the displaced cell's origin (wrapping to 0 if past the end). If the
    //      target available cell is empty, the file lands there; if filled, the
    //      file is inserted at that position and the existing files shift forward
    //      through the available chain until an empty cell absorbs the cascade.
    //
    // Caller must clamp the op first via clampOpToGrid(). Filled-count is invariant.
    DragResult computeDragResult(const GridAccessor& grid, const DragOp& op, GridDims dims);
}
