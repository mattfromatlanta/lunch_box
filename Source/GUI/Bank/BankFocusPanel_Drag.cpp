// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Drag-to-reorder + cross-row drag state machine for the Bank focus list.
// Mouse down/drag/up handlers, commit, and preview/visual update helpers.

#include "BankFocusPanel.h"
#include "BankFocusPanel_Private.h"
#include "UIColours.h"
#include "UIConstants.h"

using namespace BankFocusImpl;

void BankFocusPanel::handleRowMouseDown(FocusedSlotRow* row, const juce::MouseEvent& e)
{
    int idx = rowIndexFor(row);
    if (idx < 0) return;

    grabKeyboardFocus();
    dragSourceIdx    = idx;
    isDragging       = false;
    deferredDeselect = false;

    if (e.mods.isShiftDown())
    {
        selectRowRange(selectionAnchor, idx);
        focusedRowIdx = idx;
        updateRowVisuals();
        notifyPreviewForFocused();
    }
    else if (e.mods.isCommandDown())
    {
        toggleRowInSelection(idx);
        focusedRowIdx = idx;
        updateRowVisuals();
        notifyPreviewForFocused();
    }
    else if (selection.size() > 1 && selection.contains(idx))
    {
        // Plain click on an already-selected row in a multi-selection:
        // keep the selection intact so a drag will move the whole group.
        // Collapse to single-select on mouseUp only if no drag occurred.
        focusedRowIdx    = idx;
        deferredDeselect = true;
    }
    else
    {
        setFocusedRow(idx, true);
    }
}

void BankFocusPanel::handleRowMouseDrag(FocusedSlotRow* row, const juce::MouseEvent& e)
{
    int idx = rowIndexFor(row);
    if (idx < 0) return;

    // Begin drag after a small threshold
    if (!isDragging && e.getDistanceFromDragStart() > 6)
        isDragging = true;

    if (!isDragging) return;

    int panelX    = row->getX() + e.getPosition().getX();
    int panelY    = row->getY() + e.getPosition().getY();
    int newInsert = rowAtPoint(panelX, panelY);

    if (newInsert != dragInsertIdx)
    {
        dragInsertIdx = newInsert;
        updateDragPreviews();
    }
}

void BankFocusPanel::handleRowMouseUp(FocusedSlotRow* row, const juce::MouseEvent& /*e*/)
{
    if (isDragging && dragSourceIdx >= 0 && dragInsertIdx >= 0
        && dragSourceIdx != dragInsertIdx)
    {
        if (onBeforeChange) onBeforeChange();
        flushRowsToStorage();
        const int catIdx = (activeCategory == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
        const int N      = LunchBoxNamer::SLOTS_PER_BANK;

        const bool isMultiDrag = (selection.size() > 1 && selection.contains(dragSourceIdx));

        if (isMultiDrag)
        {
            juce::Array<int> sortedSel = selection;
            sortedSel.sort();

            juce::Array<juce::File> current;
            for (int i = 0; i < N; ++i)
                current.add(slots[catIdx][activeBank][i]);

            auto result = computeMultiRowDrag(current, sortedSel, dragSourceIdx, dragInsertIdx);
            for (int i = 0; i < N; ++i)
                slots[catIdx][activeBank][i] = result[i];
        }
        else
        {
            bool destEmpty = (slots[catIdx][activeBank][dragInsertIdx] == juce::File{});

            if (destEmpty)
            {
                slots[catIdx][activeBank][dragInsertIdx] = slots[catIdx][activeBank][dragSourceIdx];
                slots[catIdx][activeBank][dragSourceIdx] = juce::File{};
            }
            else
            {
                commitReorder(dragSourceIdx, dragInsertIdx);
            }
        }

        // Focus collapses to the landing position — no preview trigger
        focusedRowIdx   = dragInsertIdx;
        selectionAnchor = dragInsertIdx;
        selection.clear();
        selection.add(dragInsertIdx);

        clearReorderState();
        populateRowsFromStorage();
        updateRowVisuals();
        if (onAssignmentsChanged) onAssignmentsChanged();
    }
    else
    {
        // No drag: if we deferred deselection (plain click on selected row), apply it now
        if (deferredDeselect)
            setFocusedRow(dragSourceIdx, true);

        deferredDeselect = false;
        clearReorderState();
    }

    (void)row;
}

void BankFocusPanel::commitReorder(int fromIdx, int toIdx)
{
    const int catIdx = (activeCategory == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    auto& bank = slots[catIdx][activeBank];

    juce::File dragged = bank[fromIdx];

    if (toIdx > fromIdx)  // dragging down
    {
        // Find contiguous populated block ending at toIdx, going left toward fromIdx
        int blockStart = toIdx;
        while (blockStart > fromIdx + 1 && bank[blockStart - 1] != juce::File{})
            --blockStart;

        // Shift that block one slot left (into the nearest gap or the fromIdx slot)
        for (int i = blockStart; i <= toIdx; ++i)
            bank[i - 1] = bank[i];

        bank[toIdx] = dragged;

        // If block didn't reach fromIdx+1, fromIdx was not overwritten — clear it
        if (blockStart > fromIdx + 1)
            bank[fromIdx] = juce::File{};
    }
    else  // dragging up
    {
        // Find contiguous populated block starting at toIdx, going right toward fromIdx
        int blockEnd = toIdx;
        while (blockEnd < fromIdx - 1 && bank[blockEnd + 1] != juce::File{})
            ++blockEnd;

        // Shift that block one slot right (into the nearest gap or the fromIdx slot)
        for (int i = blockEnd; i >= toIdx; --i)
            bank[i + 1] = bank[i];

        bank[toIdx] = dragged;

        // If block didn't reach fromIdx-1, fromIdx was not overwritten — clear it
        if (blockEnd < fromIdx - 1)
            bank[fromIdx] = juce::File{};
    }
}

void BankFocusPanel::updateDragPreviews()
{
    clearAllPreviews();

    if (dragSourceIdx < 0 || dragInsertIdx < 0 || dragSourceIdx == dragInsertIdx)
        return;

    const int from = dragSourceIdx;
    const int to   = dragInsertIdx;
    const int N    = LunchBoxNamer::SLOTS_PER_BANK;

    // getSample() always returns actual data (not preview), safe to read mid-drag
    juce::Array<juce::File> current;
    for (int i = 0; i < N; ++i)
        current.add(rows[i]->getSample());

    if (selection.size() > 1 && selection.contains(from))
    {
        // ── Multi-row drag ──────────────────────────────────────────────────
        juce::Array<int> sortedSel = selection;
        sortedSel.sort();

        auto result = computeMultiRowDrag(current, sortedSel, from, to);

        for (int i = 0; i < N; ++i)
            if (result[i] != current[i])
                rows[i]->setPreviewSample(result[i]);

        // Highlight every row in the landing block (all populated group files)
        int groupSize = 0;
        for (int i : sortedSel)
            if (current[i] != juce::File{}) ++groupSize;

        int dragPosInGroup = 0;
        for (int i : sortedSel)
        {
            if (i == from) break;
            if (current[i] != juce::File{}) ++dragPosInGroup;
        }

        int targetStart = juce::jlimit(0, N - groupSize, to - dragPosInGroup);
        for (int i = targetStart; i < targetStart + groupSize; ++i)
            rows[i]->setDragSource(true);
    }
    else
    {
        // ── Single-row drag ─────────────────────────────────────────────────
        juce::File dragged = current[from];

        rows[to]->setPreviewSample(dragged);
        rows[to]->setDragSource(true);

        if (current[to] == juce::File{})
        {
            // Simple move to empty slot: source becomes empty, nothing else shifts
            rows[from]->setPreviewSample(juce::File{});
        }
        else if (to > from)
        {
            int blockStart = to;
            while (blockStart > from + 1 && current[blockStart - 1] != juce::File{})
                --blockStart;

            for (int i = blockStart; i <= to; ++i)
                rows[i - 1]->setPreviewSample(current[i]);

            if (blockStart > from + 1)
                rows[from]->setPreviewSample(juce::File{});
        }
        else
        {
            int blockEnd = to;
            while (blockEnd < from - 1 && current[blockEnd + 1] != juce::File{})
                ++blockEnd;

            for (int i = blockEnd; i >= to; --i)
                rows[i + 1]->setPreviewSample(current[i]);

            if (blockEnd < from - 1)
                rows[from]->setPreviewSample(juce::File{});
        }
    }
}

void BankFocusPanel::clearAllPreviews()
{
    for (auto* row : rows)
    {
        row->clearPreviewSample();
        row->setDragSource(false);
    }
}

void BankFocusPanel::clearReorderState()
{
    clearAllPreviews();

    if (dragSourceIdx >= 0 && dragSourceIdx < rows.size())
        rows[dragSourceIdx]->setDragSource(false);

    dragSourceIdx = -1;
    dragInsertIdx = -1;
    isDragging    = false;
}
