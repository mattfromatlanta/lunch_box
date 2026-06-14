// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "DragModel.h"
#include "DragHost.h"
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// DragController — drives a single drag operation on a DragHost panel.
//
// Owns the in-flight DragOp and the bookkeeping needed to translate cursor
// motion into live previews + a final commit. Identical behavior for Pack
// and Bank — the only difference is what the host returns from
// getBankClampRange() and cellAtPoint().
//
// Lifecycle: begin() at drag start, update() on mouseDrag, commit() or
// cancel() on mouseUp / Esc.
//==============================================================================
class DragController
{
public:
    explicit DragController(DragHost& host);

    bool isDragging() const { return state == State::Active; }

    // Begin a drag. `selection` is the cells being moved (must include
    // `pickupCell`).
    void begin(juce::Array<LunchBoxDrag::GridCell> selection,
               LunchBoxDrag::GridCell              pickupCell);

    // Cursor moved during a drag. Recomputes drop cell and cell-level previews.
    void update(juce::Point<int> cursorPanelPt);

    // Apply the drag — fires host's onDragCommitWillBegin, writes all cells,
    // fires onDragCommitFinished with the new selection. Clears previews.
    void commit();

    // Abort the drag without writing. Clears previews. Used for
    // Esc-key, focus loss, panel hide, etc.
    void cancel();

private:
    enum class State { Idle, Active };

    void rebuildPreviewsFor(const LunchBoxDrag::DragOp& op);

    DragHost&  host;
    State      state = State::Idle;

    juce::Array<LunchBoxDrag::GridCell> sourceCells;
    LunchBoxDrag::GridCell              pickupCell;
    LunchBoxDrag::GridCell              lastDropCell;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragController)
};
