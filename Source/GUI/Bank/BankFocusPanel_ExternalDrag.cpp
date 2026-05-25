// SPDX-License-Identifier: AGPL-3.0-or-later
//
// FileDragAndDropTarget implementation — accepts file drags from outside the
// app (Finder) and routes them into the focused bank's rows.

#include "BankFocusPanel.h"
#include "../FileSystemHelper.h"

bool BankFocusPanel::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (dragController.isDragging()) return false;
    if (files.isEmpty() || files.size() > LunchBoxNamer::SLOTS_PER_BANK) return false;
    for (const auto& f : files)
    {
        auto ext = "*" + juce::File(f).getFileExtension().toLowerCase();
        if (!FileSystemHelper::getSupportedAudioExtensions().contains(ext)) return false;
    }
    return true;
}

void BankFocusPanel::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    externalDragFiles = files;
    updateExternalDragHighlight(x, y);
}

void BankFocusPanel::fileDragMove(const juce::StringArray& files, int x, int y)
{
    externalDragFiles = files;
    updateExternalDragHighlight(x, y);
}

void BankFocusPanel::fileDragExit(const juce::StringArray&)
{
    externalDragFiles.clear();
    clearAllCellPreviews();
}

void BankFocusPanel::filesDropped(const juce::StringArray& files, int x, int y)
{
    int startRow = rowAtPoint(x, y);

    externalDragFiles.clear();
    clearAllCellPreviews();

    if (onBeforeChange) onBeforeChange();

    const int catIdx = (activeCategory == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    for (int i = 0; i < files.size() && startRow + i < LunchBoxNamer::SLOTS_PER_BANK; ++i)
    {
        juce::File f(files[i]);
        if (f.existsAsFile())
            slots[catIdx][activeBank][startRow + i] = f;
    }

    populateRowsFromStorage();
    if (onAssignmentsChanged) onAssignmentsChanged();
}

void BankFocusPanel::updateExternalDragHighlight(int x, int y)
{
    clearAllCellPreviews();

    int startRow = rowAtPoint(x, y);
    if (startRow < 0 || startRow >= rows.size()) return;

    for (int i = 0; i < externalDragFiles.size() && startRow + i < rows.size(); ++i)
    {
        rows[startRow + i]->setDragSource(true);
        rows[startRow + i]->setPreviewSample(juce::File(externalDragFiles[i]));
    }
}
