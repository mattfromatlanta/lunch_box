// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Selection + keyboard navigation for the Bank focus list: focus/select rows,
// paste, select-all/clear, arrow-key movement and range expansion, etc.

#include "BankFocusPanel.h"
#include <algorithm>

void BankFocusPanel::setFocusedRow(int idx, bool clearOthers)
{
    focusedRowIdx = idx;
    if (clearOthers)
    {
        selection.clear();
        selection.add(idx);
        selectionAnchor = idx;
    }
    else if (!selection.contains(idx))
    {
        selection.add(idx);
    }
    updateRowVisuals();
    notifyPreviewForFocused();
}

void BankFocusPanel::notifyPreviewForFocused()
{
    if (selection.size() == 1 && focusedRowIdx >= 0 && focusedRowIdx < rows.size())
    {
        auto* row = rows[focusedRowIdx];
        if (row->hasSample()) { if (onSlotClicked) onSlotClicked(row->getSample()); }
        else                  { if (onPreviewStop) onPreviewStop(); }
    }
    else if (selection.size() > 1)
    {
        if (onPreviewStop) onPreviewStop();
    }
}

void BankFocusPanel::updateRowVisuals()
{
    for (int i = 0; i < rows.size(); ++i)
    {
        const bool sel = selection.contains(i);
        rows[i]->setSelected(sel);
        rows[i]->setFocused(sel && i == focusedRowIdx);
    }
}

void BankFocusPanel::toggleRowInSelection(int idx)
{
    int pos = selection.indexOf(idx);
    if (pos >= 0) selection.remove(pos);
    else          selection.add(idx);
}

void BankFocusPanel::selectRowRange(int from, int to)
{
    selection.clear();
    int lo = juce::jmin(from, to), hi = juce::jmax(from, to);
    for (int i = lo; i <= hi; ++i)
        selection.add(i);
}

// ─── Copy / cut / paste ───────────────────────────────────────────────────────

juce::Array<ClipboardEntry> BankFocusPanel::getSelectedClipboard()
{
    flushRowsToStorage();

    juce::Array<int> sorted = selection;
    std::sort(sorted.begin(), sorted.end());

    const int anchor = sorted.isEmpty() ? 0 : sorted[0];
    juce::Array<ClipboardEntry> entries;
    for (int idx : sorted)
        entries.add({ model.getSlot(activeCategory, activeBank, idx), idx - anchor, 0 });
    return entries;
}

void BankFocusPanel::pasteClipboard(const juce::Array<ClipboardEntry>& entries)
{
    if (entries.isEmpty()) return;
    for (const auto& e : entries)
    {
        int slot = focusedRowIdx + e.rowOffset;
        if (slot < 0 || slot >= LunchBoxNamer::SLOTS_PER_BANK) continue;
        model.setSlot(activeCategory, activeBank, slot, e.file);
    }
    populateRowsFromStorage();
    if (onAssignmentsChanged) onAssignmentsChanged();
}

// ─── Public keyboard-navigation API ──────────────────────────────────────────

void BankFocusPanel::selectAll()
{
    selection.clear();
    for (int i = 0; i < LunchBoxNamer::SLOTS_PER_BANK; ++i)
        selection.add(i);
    focusedRowIdx   = 0;
    selectionAnchor = 0;
    updateRowVisuals();
    if (onPreviewStop) onPreviewStop();
}

void BankFocusPanel::clearSelection()
{
    selection.clear();
    selection.add(focusedRowIdx);
    selectionAnchor = focusedRowIdx;
    updateRowVisuals();
}

void BankFocusPanel::moveFocusedRow(int delta)
{
    int next = juce::jlimit(0, LunchBoxNamer::SLOTS_PER_BANK - 1, focusedRowIdx + delta);
    setFocusedRow(next, true);
}

void BankFocusPanel::expandRowSelection(int delta)
{
    if (selection.isEmpty()) return;

    int minS = selection[0], maxS = selection[0];
    for (int i : selection) { minS = juce::jmin(minS, i);  maxS = juce::jmax(maxS, i); }

    if (delta < 0) minS = juce::jmax(0, minS - 1);
    else           maxS = juce::jmin(LunchBoxNamer::SLOTS_PER_BANK - 1, maxS + 1);

    selection.clear();
    for (int i = minS; i <= maxS; ++i) selection.add(i);
    focusedRowIdx = (delta < 0) ? minS : maxS;

    updateRowVisuals();
    if (onPreviewStop) onPreviewStop();  // multi-select always stops preview
}

void BankFocusPanel::playFocused()
{
    if (focusedRowIdx >= 0 && focusedRowIdx < rows.size())
    {
        auto* row = rows[focusedRowIdx];
        if (row->hasSample() && onSlotClicked)
            onSlotClicked(row->getSample());
    }
}

void BankFocusPanel::browseForFocused()
{
    if (focusedRowIdx >= 0 && focusedRowIdx < rows.size())
    {
        if (onBeforeChange) onBeforeChange();
        rows[focusedRowIdx]->browseForFile();
    }
}

void BankFocusPanel::clearFocusedRows()
{
    for (int i : selection)
        if (i >= 0 && i < rows.size())
            rows[i]->clearSample();
    flushRowsToStorage();
    if (onAssignmentsChanged) onAssignmentsChanged();
}
