// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "DragModel.h"
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// DragHost — the contract a panel implements to be drag-controlled.
//
// Both BankEditorPanel (Pack) and BankFocusPanel (Bank) provide one of these so
// the DragController can drive their drag-and-drop without knowing the specific
// panel type. Keeps the controller pure C++ over a thin interface.
//==============================================================================
class DragHost
{
public:
    virtual ~DragHost() = default;

    // Data-coord grid geometry (bank × slot), used for cascade ordering during
    // displacement. 5×14 in both modes today.
    virtual LunchBoxDrag::GridDims getGridDims() const = 0;

    // Visual-coord grid geometry — the UI layout the user actually sees.
    //   Pack: 10 rows × 7 cols (each bank's 14 slots wrap into two 7-cell rows).
    //   Bank: 14 rows × 1 col (single column of slots within the active bank).
    // Clamping and per-cell translation during drag happen in this coordinate
    // system so the visible bounding box matches what the user perceives.
    virtual LunchBoxDrag::GridDims getVisualDims() const = 0;

    // Convert between data coords (bank, slot) and visual coords (row, col).
    // GridCell is reused for both — `bank` holds row in visual space.
    virtual LunchBoxDrag::GridCell toVisual  (LunchBoxDrag::GridCell dataCell) const = 0;
    virtual LunchBoxDrag::GridCell fromVisual(LunchBoxDrag::GridCell visualCell) const = 0;

    // Bank-axis clamp range for the cascade in DATA coords.
    //   Pack: { 0, numBanks-1 }
    //   Bank: { activeBank, activeBank }  (displacement stays intra-bank)
    virtual std::pair<int,int> getBankClampRange() const = 0;

    // Hit-test: map a panel-relative point to a grid cell. Invalid cell if the
    // point is outside any cell (e.g. between rows in Bank mode, on padding).
    virtual LunchBoxDrag::GridCell cellAtPoint(juce::Point<int> panelPt) const = 0;

    // The on-screen bounds of a given cell, in panel coordinates. Used by the
    // floating drag proxy to render each moving cell at its anchor offset.
    virtual juce::Rectangle<int> cellBoundsInPanel(LunchBoxDrag::GridCell c) const = 0;

    // Read the file currently committed to a cell. Used by the controller to
    // snapshot source files at drag start and to compute previews.
    virtual juce::File getFileAt(LunchBoxDrag::GridCell c) const = 0;

    // Apply one committed write (real edit, not a preview).
    virtual void setFileAt(LunchBoxDrag::GridCell c, const juce::File& f) = 0;

    // Show / clear the per-cell preview overlay (drives the in-place rendering
    // that complements the floating proxy).
    virtual void setCellPreview(LunchBoxDrag::GridCell c, const juce::File& f) = 0;

    // Per-cell drag visual roles (all reset by clearAllCellPreviews):
    //   Source     — cell content is being moved away from here; render vacated, suppress selection.
    //   Destination— moving content will land here; render with selection styling.
    //   Displace   — displaced content from elsewhere will land here; render with a thicker accent border.
    virtual void setCellDragRoleSource     (LunchBoxDrag::GridCell c, bool s) = 0;
    virtual void setCellDragRoleDestination(LunchBoxDrag::GridCell c, bool s) = 0;
    // dir: -1 = displaced content moves to a lower global index (left/up arrow)
    //       0 = clear
    //      +1 = displaced content moves to a higher global index (right/down arrow)
    virtual void setCellDragRoleDisplace   (LunchBoxDrag::GridCell c, int dir) = 0;

    virtual void clearAllCellPreviews() = 0;

    // Called once at the start of rebuildPreviewsFor(), before any per-cell role
    // calls. Hosts that need the full op context (e.g. to derive label strings)
    // can override; the default is a no-op.
    virtual void onPreviewRebuild(const LunchBoxDrag::DragOp& /*op*/) {}

    // Called once at the start of a drag commit, before any writes — gives the
    // host a chance to capture an undo snapshot.
    virtual void onDragCommitWillBegin() = 0;

    // Called once at the end of a drag commit, after all writes are applied.
    // `newSelection` is where the dragged cells landed (destCells in data coords).
    // `oldSources`   is where they came from (sourceCells in data coords).
    // The host uses both to move keyboard focus to follow the previously-focused cell.
    virtual void onDragCommitFinished(const juce::Array<LunchBoxDrag::GridCell>& newSelection,
                                      const juce::Array<LunchBoxDrag::GridCell>& oldSources) = 0;
};
