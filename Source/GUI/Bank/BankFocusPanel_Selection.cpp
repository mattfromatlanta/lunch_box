// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Selection + keyboard navigation for the Bank focus list: focus/select rows,
// paste, select-all/clear, arrow-key movement and range expansion, etc.

#include "BankFocusPanel.h"

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

juce::Array<juce::File> BankFocusPanel::getSelectedFiles()
{
    flushRowsToStorage();
    const int catIdx = (activeCategory == LunchBoxNamer::Category::Cubbi) ? 0 : 1;

    juce::Array<int> sorted = selection;
    for (int i = 0; i < sorted.size() - 1; ++i)
        for (int j = i + 1; j < sorted.size(); ++j)
            if (sorted[j] < sorted[i]) sorted.swap(i, j);

    juce::Array<juce::File> files;
    for (int idx : sorted)
        files.add(slots[catIdx][activeBank][idx]);
    return files;
}

void BankFocusPanel::pasteFiles(const juce::Array<juce::File>& files)
{
    if (files.isEmpty()) return;
    const int catIdx = (activeCategory == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    for (int i = 0; i < files.size(); ++i)
    {
        int slot = focusedRowIdx + i;
        if (slot >= LunchBoxNamer::SLOTS_PER_BANK) break;
        slots[catIdx][activeBank][slot] = files[i];
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

// ─── Real-time Cmd-key toggle during drag ─────────────────────────────────────

void BankFocusPanel::modifierKeysChanged(const juce::ModifierKeys&)
{
    // Drag mode is determined by destination content, not modifier keys — nothing to do
}
