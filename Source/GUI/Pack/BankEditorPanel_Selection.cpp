// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Selection + keyboard navigation for the Pack 5×14 grid: cell select/toggle,
// range select, focus movement, paste, select-all/clear, etc.

#include "BankEditorPanel.h"
#include "BankEditorPanel_Private.h"
#include "UIColours.h"

using namespace BankEditorImpl;

void BankEditorPanel::selectCell(Cell c, bool clearFirst)
{
    if (clearFirst) selection.clear();
    if (!selection.contains(c)) selection.add(c);
    focusCell = c;
    updateSlotVisuals();
}

void BankEditorPanel::toggleCell(Cell c)
{
    int idx = selection.indexOf(c);
    if (idx >= 0) selection.remove(idx);
    else          selection.add(c);
    focusCell = c;
    updateSlotVisuals();
}

void BankEditorPanel::selectRange(Cell a, Cell b)
{
    // Build a visual rectangle from a to b and select all cells within it
    int vrA = a.row * 2 + a.col / 7, vcA = a.col % 7;
    int vrB = b.row * 2 + b.col / 7, vcB = b.col % 7;

    int vr0 = std::min(vrA, vrB), vr1 = std::max(vrA, vrB);
    int vc0 = std::min(vcA, vcB), vc1 = std::max(vcA, vcB);

    selection.clear();
    for (int vr = vr0; vr <= vr1; ++vr)
        for (int vc = vc0; vc <= vc1; ++vc)
            selection.add({ vr / 2, (vr % 2) * 7 + vc });
    focusCell = b;
    updateSlotVisuals();
}

void BankEditorPanel::setFocusCellAndSelect(Cell cell)
{
    focusCell = cell;
    selection.clear();
    selection.add(cell);
    updateSlotVisuals();
}

void BankEditorPanel::clearSelection()
{
    selection.clear();
    updateSlotVisuals();
}

void BankEditorPanel::selectAll()
{
    selection.clear();
    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
            selection.add({ b, s });
    focusCell = { 0, 0 };
    updateSlotVisuals();
    if (onPreviewStop) onPreviewStop();
}

void BankEditorPanel::updateSlotVisuals()
{
    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
            if (auto* slot = getSlotAt(b, s))
            {
                Cell c = {b, s};
                slot->setSelected(selection.contains(c));
                slot->setFocused(!selection.isEmpty() && c == focusCell);
            }
}

// ─── Selection navigation ─────────────────────────────────────────────────────

void BankEditorPanel::notifyPreviewForSelection()
{
    if (selection.size() == 1)
    {
        auto* slot = getSlotAt(focusCell.row, focusCell.col);
        if (slot && slot->hasSample())
            playFocused();
        else if (onPreviewStop)
            onPreviewStop();
    }
    else
    {
        if (onPreviewStop) onPreviewStop();
    }
}

void BankEditorPanel::moveFocus(int dr, int dc)
{
    Cell anchor = (selection.size() > 1) ? getEarliestSelected() : focusCell;

    // Convert to visual coords: 10 rows × 7 cols (2 sub-rows per bank)
    int vr = anchor.row * 2 + anchor.col / 7;
    int vc = anchor.col % 7;

    // Move in visual space — no wrapping at the 7/8 slot boundary
    vr = juce::jlimit(0, LunchBoxNamer::NUM_BANKS * 2 - 1, vr + dr);
    vc = juce::jlimit(0, 6, vc + dc);

    Cell next = { vr / 2, (vr % 2) * 7 + vc };
    selection.clear();
    selection.add(next);
    focusCell = next;
    updateSlotVisuals();
    notifyPreviewForSelection();
}

void BankEditorPanel::expandSelection(int dRow, int dCol)
{
    if (selection.isEmpty()) return;

    const int maxVisRow = LunchBoxNamer::NUM_BANKS * 2 - 1;
    const int maxVisCol = 6;

    // Compute bounding box in visual coords
    int minVR = maxVisRow + 1, maxVR = -1;
    int minVC = maxVisCol + 1, maxVC = -1;
    for (const auto& c : selection)
    {
        int vr = c.row * 2 + c.col / 7;
        int vc = c.col % 7;
        minVR = std::min(minVR, vr); maxVR = std::max(maxVR, vr);
        minVC = std::min(minVC, vc); maxVC = std::max(maxVC, vc);
    }

    // Expand by 1 in the requested direction, clamped
    int newMinVR = minVR, newMaxVR = maxVR;
    int newMinVC = minVC, newMaxVC = maxVC;

    if      (dRow > 0) newMaxVR = std::min(maxVR + 1, maxVisRow);
    else if (dRow < 0) newMinVR = std::max(minVR - 1, 0);
    if      (dCol > 0) newMaxVC = std::min(maxVC + 1, maxVisCol);
    else if (dCol < 0) newMinVC = std::max(minVC - 1, 0);

    if (newMinVR == minVR && newMaxVR == maxVR &&
        newMinVC == minVC && newMaxVC == maxVC)
        return;

    // Rebuild selection from the expanded visual rectangle
    selection.clear();
    for (int vr = newMinVR; vr <= newMaxVR; ++vr)
        for (int vc = newMinVC; vc <= newMaxVC; ++vc)
            selection.add({ vr / 2, (vr % 2) * 7 + vc });

    // Focus tracks the expanding edge
    int focusVR = focusCell.row * 2 + focusCell.col / 7;
    int focusVC = focusCell.col % 7;
    if      (dRow > 0) focusVR = newMaxVR;
    else if (dRow < 0) focusVR = newMinVR;
    else if (dCol > 0) focusVC = newMaxVC;
    else if (dCol < 0) focusVC = newMinVC;
    focusCell = { focusVR / 2, (focusVR % 2) * 7 + focusVC };

    updateSlotVisuals();
    if (onPreviewStop) onPreviewStop();
}

void BankEditorPanel::playFocused()
{
    if (selection.size() != 1) return;
    auto* slot = getSlotAt(focusCell.row, focusCell.col);
    if (slot && slot->hasSample())
        if (onSlotClicked) onSlotClicked(focusCell, slot->getSample());
}

void BankEditorPanel::clearSelectedCells()
{
    for (const auto& c : selection)
        if (auto* slot = getSlotAt(c.row, c.col))
            slot->clearSample();
}

void BankEditorPanel::browseForFocused()
{
    if (auto* slot = getSlotAt(focusCell.row, focusCell.col))
        slot->browseForFile();  // onBeforeChange fires inside browseForFile()
}

// ─── Copy / cut / paste ───────────────────────────────────────────────────────

juce::Array<ClipboardEntry> BankEditorPanel::getSelectedClipboard() const
{
    // Sort selected cells in visual row-major order (top-left → bottom-right)
    juce::Array<Cell> sorted = selection;
    for (int i = 0; i < sorted.size() - 1; ++i)
        for (int j = i + 1; j < sorted.size(); ++j)
        {
            int vri = sorted[i].row * 2 + sorted[i].col / 7, vci = sorted[i].col % 7;
            int vrj = sorted[j].row * 2 + sorted[j].col / 7, vcj = sorted[j].col % 7;
            if (vrj < vri || (vrj == vri && vcj < vci))
                sorted.swap(i, j);
        }

    // Anchor is the earliest cell; record each cell's offset relative to it
    const Cell anchor = sorted.isEmpty() ? Cell{0,0} : sorted[0];
    juce::Array<ClipboardEntry> entries;
    for (const auto& c : sorted)
        if (auto* slot = getSlotAt(c.row, c.col))
            entries.add({ slot->getSample(), c.row - anchor.row, c.col - anchor.col });
    return entries;
}

void BankEditorPanel::pasteClipboard(const juce::Array<ClipboardEntry>& entries)
{
    if (entries.isEmpty()) return;
    for (const auto& e : entries)
    {
        int r = focusCell.row + e.rowOffset;
        int c = focusCell.col + e.colOffset;
        if (r < 0 || r >= LunchBoxNamer::NUM_BANKS)   continue;
        if (c < 0 || c >= LunchBoxNamer::SLOTS_PER_BANK) continue;
        if (auto* slot = getSlotAt(r, c))
            slot->setSample(e.file);
    }
    if (onAssignmentsChanged) onAssignmentsChanged();
}
