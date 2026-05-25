// SPDX-License-Identifier: AGPL-3.0-or-later
//
// FileDragAndDropTarget implementation — accepts file drags from outside the
// app (Finder) and routes them into the grid starting at the drop cell.

#include "BankEditorPanel.h"
#include "BankEditorPanel_Private.h"
#include "../FileSystemHelper.h"

using namespace BankEditorImpl;

bool BankEditorPanel::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (files.isEmpty() || files.size() > LunchBoxNamer::SLOTS_PER_BANK) return false;
    for (const auto& f : files)
    {
        auto ext = "*" + juce::File(f).getFileExtension().toLowerCase();
        if (!FileSystemHelper::getSupportedAudioExtensions().contains(ext)) return false;
    }
    return true;
}

void BankEditorPanel::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    externalDragFiles = files;
    updateExternalDragHighlight(x, y);
}

void BankEditorPanel::fileDragMove(const juce::StringArray& files, int x, int y)
{
    externalDragFiles = files;
    updateExternalDragHighlight(x, y);
}

void BankEditorPanel::fileDragExit(const juce::StringArray&)
{
    externalDragFiles.clear();
    dragTargetCells.clear();
    clearAllPreviews();
    updateDragTargetVisuals();
}

void BankEditorPanel::filesDropped(const juce::StringArray& files, int x, int y)
{
    Cell target  = getCellAtPoint({ x, y });
    auto targets = getExternalDropCells(target, files.size());

    externalDragFiles.clear();
    dragTargetCells.clear();
    clearAllPreviews();
    updateDragTargetVisuals();

    if (onBeforeChange) onBeforeChange();

    for (int i = 0; i < targets.size(); ++i)
        if (auto* slot = getSlotAt(targets[i].row, targets[i].col))
            slot->setSample(juce::File(files[i]));

    if (onAssignmentsChanged) onAssignmentsChanged();
}

juce::Array<BankEditorPanel::Cell> BankEditorPanel::getExternalDropCells(Cell start, int count) const
{
    juce::Array<Cell> result;
    if (!start.isValid()) return result;

    const int total       = LunchBoxNamer::NUM_BANKS * LunchBoxNamer::SLOTS_PER_BANK;
    const int startLinear = start.row * LunchBoxNamer::SLOTS_PER_BANK + start.col;

    for (int i = 0; i < count; ++i)
    {
        int linear = startLinear + i;
        if (linear >= total) break;
        result.add({ linear / LunchBoxNamer::SLOTS_PER_BANK, linear % LunchBoxNamer::SLOTS_PER_BANK });
    }
    return result;
}

void BankEditorPanel::updateExternalDragHighlight(int x, int y)
{
    Cell target   = getCellAtPoint({ x, y });
    dragTargetCells = getExternalDropCells(target, externalDragFiles.size());

    clearAllPreviews();
    for (int i = 0; i < dragTargetCells.size(); ++i)
        if (auto* slot = getSlotAt(dragTargetCells[i].row, dragTargetCells[i].col))
            slot->setPreviewSample(juce::File(externalDragFiles[i]));

    updateDragTargetVisuals();
}
