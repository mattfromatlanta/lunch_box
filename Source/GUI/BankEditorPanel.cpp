// SPDX-License-Identifier: AGPL-3.0-or-later
#include "BankEditorPanel.h"
#include "UIColours.h"
#include "UIConstants.h"
#include "../FileSystemHelper.h"

namespace
{
    const juce::Colour panelBg     = ChompiColours::DARK_GREY;
    const juce::Colour accentCol   { 0xff4caf50 };
    const juce::Colour buttonCol   { 0xff2a3a4a };
    const juce::Colour buttonTxt   { 0xffaabbcc };

    struct CellFile
    {
        BankEditorPanel::Cell c;
        juce::File f;
    };

    // Sort an array of CellFile by (row, col) ascending — used for stable swap pairing
    static void sortCellsRowMajor(juce::Array<CellFile>& arr)
    {
        for (int i = 0; i < arr.size() - 1; ++i)
            for (int j = i + 1; j < arr.size(); ++j)
            {
                auto a = arr[i], b = arr[j];
                if (b.c.row < a.c.row || (b.c.row == a.c.row && b.c.col < a.c.col))
                    arr.swap(i, j);
            }
    }
}

BankEditorPanel::BankEditorPanel(ChompiNamer::Category cat)
    : category(cat)
{
    const char letters[] = {'a', 'b', 'c', 'd', 'e'};
    for (int i = 0; i < ChompiNamer::NUM_BANKS; ++i)
    {
        auto* row = banks.add(new BankRowComponent(letters[i]));
        wireRowCallbacks(row, i);
        addAndMakeVisible(row);
    }

    setWantsKeyboardFocus(true);
}

void BankEditorPanel::wireRowCallbacks(BankRowComponent* row, int bankIdx)
{
    row->onAssignmentsChanged = [this]() { if (onAssignmentsChanged) onAssignmentsChanged(); };
    row->getStartDirectory    = [this]() -> juce::File { return (getStartDirectory) ? getStartDirectory() : juce::File{}; };
    row->onFolderBrowsed      = [this](juce::File dir)  { if (onFolderBrowsed) onFolderBrowsed(dir); };

    for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
    {
        if (auto* slot = row->getSlotComponent(s))
        {
            slot->onBeforeChange = [this]() { if (onBeforeChange) onBeforeChange(); };
            slot->onSlotClicked = [this, bankIdx, s](BankSlotComponent* src)
            {
                if (onSlotClicked) onSlotClicked({ bankIdx, s }, src->getSample());
            };
            slot->onSlotMouseDown = [this](BankSlotComponent* src, const juce::MouseEvent& e)
            {
                handleSlotMouseDown(src, e);
            };
            slot->onSlotMouseDrag = [this](BankSlotComponent* src, const juce::MouseEvent& e)
            {
                handleSlotMouseDrag(src, e);
            };
            slot->onSlotMouseUp = [this](BankSlotComponent* src, const juce::MouseEvent& e)
            {
                handleSlotMouseUp(src, e);
            };
            slot->onSlotDoubleClicked = [this](BankSlotComponent* src)
            {
                auto c = getCellFor(src);
                if (c.isValid()) { selectCell(c, true); grabKeyboardFocus(); }
                src->browseForFile();  // onBeforeChange fires inside browseForFile()
            };
        }
    }
}

juce::Array<BankFolderParser::BankAssignment> BankEditorPanel::getAssignments() const
{
    juce::Array<BankFolderParser::BankAssignment> all;
    for (const auto* bank : banks)
        all.addArray(bank->getAssignments());
    return all;
}

int BankEditorPanel::getFilledCount() const
{
    int total = 0;
    for (const auto* bank : banks)
        total += bank->getFilledCount();
    return total;
}

void BankEditorPanel::clearAllBanks()
{
    for (auto* bank : banks)
        bank->clearAllSlots();
}

void BankEditorPanel::setSlotFile(int bankIdx, int slotIdx, const juce::File& file)
{
    if (bankIdx >= 0 && bankIdx < banks.size())
        banks[bankIdx]->setSlot(slotIdx, file);
}

juce::File BankEditorPanel::getSlotFile(int bankIdx, int slotIdx) const
{
    if (auto* slot = getSlotAt(bankIdx, slotIdx))
        return slot->getSample();
    return juce::File{};
}

void BankEditorPanel::autoFillFromFolder(const juce::File&)
{
    juce::File startDir = (getStartDirectory) ? getStartDirectory()
                        : juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Folder to Auto-Fill", startDir, "", true);

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
    {
        juce::File folder = chooser.getResult();
        if (folder == juce::File{} || !folder.isDirectory())
            return;

        // Callback-only logger (no log file) — forwards messages to the GUI
        Logger tempLogger(false);
        tempLogger.onLog = [this](const juce::String& msg)
        {
            if (onLog) onLog(msg);
        };

        // Use BankFolderParser so bank subfolders and overflow are handled identically
        // to how the exporter assigns samples
        BankFolderParser parser(tempLogger);
        auto assignments = parser.parseFolderStructure(folder, category);

        if (assignments.isEmpty()) return;

        if (onBeforeChange) onBeforeChange();
        clearAllBanks();

        for (const auto& a : assignments)
        {
            int bankIdx = (int)(a.bankLetter - 'a');
            if (bankIdx >= 0 && bankIdx < ChompiNamer::NUM_BANKS)
                banks[bankIdx]->setSlot(a.slotNumber - 1, a.sourceFile);
        }

        if (onFolderBrowsed) onFolderBrowsed(folder);
    });
}

// ─── Selection helpers ────────────────────────────────────────────────────────

BankSlotComponent* BankEditorPanel::getSlotAt(int b, int s) const
{
    if (b >= 0 && b < banks.size()) return banks[b]->getSlotComponent(s);
    return nullptr;
}

BankEditorPanel::Cell BankEditorPanel::getCellFor(BankSlotComponent* slot) const
{
    for (int b = 0; b < banks.size(); ++b)
        for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
            if (banks[b]->getSlotComponent(s) == slot)
                return {b, s};
    return {-1, -1};
}

BankEditorPanel::Cell BankEditorPanel::getCellAtPoint(juce::Point<int> pt) const
{
    for (int b = 0; b < banks.size(); ++b)
    {
        auto* bank = banks[b];
        if (!bank->getBounds().contains(pt)) continue;
        auto local = pt - bank->getBounds().getPosition();
        for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
            if (auto* slot = bank->getSlotComponent(s))
                if (slot->getBounds().contains(local))
                    return {b, s};
    }
    return {-1, -1};
}

BankEditorPanel::Cell BankEditorPanel::getEarliestSelected() const
{
    if (selection.isEmpty()) return focusCell;
    Cell earliest = selection[0];
    for (const auto& c : selection)
        if (c.row < earliest.row || (c.row == earliest.row && c.col < earliest.col))
            earliest = c;
    return earliest;
}

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

void BankEditorPanel::clearSelection()
{
    selection.clear();
    updateSlotVisuals();
}

void BankEditorPanel::selectAll()
{
    selection.clear();
    for (int b = 0; b < ChompiNamer::NUM_BANKS; ++b)
        for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
            selection.add({ b, s });
    focusCell = { 0, 0 };
    updateSlotVisuals();
    if (onPreviewStop) onPreviewStop();
}

void BankEditorPanel::updateSlotVisuals()
{
    for (int b = 0; b < ChompiNamer::NUM_BANKS; ++b)
        for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
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
    vr = juce::jlimit(0, ChompiNamer::NUM_BANKS * 2 - 1, vr + dr);
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

    const int maxVisRow = ChompiNamer::NUM_BANKS * 2 - 1;
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

void BankEditorPanel::tabFocus()
{
    Cell next = { (focusCell.row + 1) % ChompiNamer::NUM_BANKS, focusCell.col };
    selection.clear();
    selection.add(next);
    focusCell = next;
    updateSlotVisuals();
    notifyPreviewForSelection();
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

juce::Array<juce::File> BankEditorPanel::getSelectedFiles() const
{
    // Return files ordered by visual row-major (top-left → bottom-right)
    juce::Array<Cell> sorted = selection;
    for (int i = 0; i < sorted.size() - 1; ++i)
        for (int j = i + 1; j < sorted.size(); ++j)
        {
            int vri = sorted[i].row * 2 + sorted[i].col / 7, vci = sorted[i].col % 7;
            int vrj = sorted[j].row * 2 + sorted[j].col / 7, vcj = sorted[j].col % 7;
            if (vrj < vri || (vrj == vri && vcj < vci))
                sorted.swap(i, j);
        }

    juce::Array<juce::File> files;
    for (const auto& c : sorted)
        if (auto* slot = getSlotAt(c.row, c.col))
            files.add(slot->getSample());
    return files;
}

void BankEditorPanel::pasteFiles(const juce::Array<juce::File>& files)
{
    if (files.isEmpty()) return;
    auto targets = getExternalDropCells(focusCell, files.size());
    for (int i = 0; i < targets.size(); ++i)
        if (auto* slot = getSlotAt(targets[i].row, targets[i].col))
            slot->setSample(files[i]);
    if (onAssignmentsChanged) onAssignmentsChanged();
}

// ─── Mouse-based selection ────────────────────────────────────────────────────

void BankEditorPanel::handleSlotMouseDown(BankSlotComponent* src, const juce::MouseEvent& e)
{
    Cell c = getCellFor(src);
    if (!c.isValid()) return;

    mouseDownCell = c;
    mouseDownOnSelected = false;

    if (e.mods.isShiftDown())
    {
        selectRange(focusCell, c);
        notifyPreviewForSelection();
    }
    else if (e.mods.isCommandDown())
    {
        toggleCell(c);
        notifyPreviewForSelection();
    }
    else if (selection.contains(c))
    {
        // Defer: could become a drag-move or a click-to-collapse
        mouseDownOnSelected = true;
    }
    else
    {
        selectCell(c, true);
        mouseDownOnSelected = true;  // plain drag from here = move, not box select
        // Preview fires on mouseUp (via mouseDownOnSelected path), not here
    }

    grabKeyboardFocus();
}

void BankEditorPanel::handleSlotMouseDrag(BankSlotComponent* /*src*/, const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;
    if (e.getDistanceFromDragStart() < 5) return;

    auto panelPt = e.getEventRelativeTo(this).getPosition();
    Cell hover   = getCellAtPoint(panelPt);

    // All drags move the selection
    if (!isSelectionDragging)
    {
        isSelectionDragging = true;
        selectionDragStart  = mouseDownCell;
        mouseDownOnSelected = false;
    }

    if (isSelectionDragging)
    {
        if (!hover.isValid() || !selectionDragStart.isValid()) return;

        // Unclamped delta: how far the cursor has moved from the drag-start cell
        int dr = hover.row - selectionDragStart.row;
        int dc = hover.col - selectionDragStart.col;

        // Bounding box of the current selection
        int minRow = ChompiNamer::NUM_BANKS,      maxRow = -1;
        int minCol = ChompiNamer::SLOTS_PER_BANK, maxCol = -1;
        for (const auto& c : selection)
        {
            minRow = std::min(minRow, c.row);  maxRow = std::max(maxRow, c.row);
            minCol = std::min(minCol, c.col);  maxCol = std::max(maxCol, c.col);
        }

        // Clamp so entire selection stays within the grid
        dr = juce::jlimit(-minRow, (ChompiNamer::NUM_BANKS      - 1) - maxRow, dr);
        dc = juce::jlimit(-minCol, (ChompiNamer::SLOTS_PER_BANK - 1) - maxCol, dc);

        dragTargetCells.clear();
        for (const auto& c : selection)
            dragTargetCells.add({ c.row + dr, c.col + dc });

        updateDragTargetVisuals();
        updateDragPreviews();
    }
}

void BankEditorPanel::handleSlotMouseUp(BankSlotComponent*, const juce::MouseEvent& /*e*/)
{
    if (isSelectionDragging && !dragTargetCells.isEmpty())
    {
        commitSelectionDrag();
        if (onPreviewStop) onPreviewStop();  // no auto-play on drag release
    }
    else if (mouseDownOnSelected)
    {
        // Simple click on a selected cell — collapse selection to just that cell
        selectCell(mouseDownCell, true);
        notifyPreviewForSelection();
    }

    clearDragState();
}

void BankEditorPanel::commitSelectionDrag()
{
    if (dragTargetCells.size() != selection.size()) return;
    if (onBeforeChange) onBeforeChange();

    // 1. Snapshot all source and target files before any mutation.
    //    Sorting by position ensures geometrically stable pairing regardless
    //    of how the selection was built (drag-select vs cmd-click).
    juce::Array<juce::File> sourceFiles, targetFiles;
    for (int i = 0; i < selection.size(); ++i)
    {
        auto* s = getSlotAt(selection[i].row,       selection[i].col);
        auto* t = getSlotAt(dragTargetCells[i].row, dragTargetCells[i].col);
        sourceFiles.add(s ? s->getSample() : juce::File{});
        targetFiles.add(t ? t->getSample() : juce::File{});
    }

    // 2. Build source-only and target-only (cell, file) pairs, sorted row-major.
    //    Source-only cells will receive displaced content; overlap cells carry
    //    the moving-set content and are excluded.
    juce::Array<CellFile> srcOnly, tgtOnly;
    for (int i = 0; i < selection.size(); ++i)
        if (!dragTargetCells.contains(selection[i]))
            srcOnly.add({ selection[i], sourceFiles[i] });
    for (int j = 0; j < dragTargetCells.size(); ++j)
        if (!selection.contains(dragTargetCells[j]))
            tgtOnly.add({ dragTargetCells[j], targetFiles[j] });

    sortCellsRowMajor(srcOnly);
    sortCellsRowMajor(tgtOnly);

    // 3. Clear all source slots (overlap slots re-filled in step 4)
    for (const auto& c : selection)
        if (auto* slot = getSlotAt(c.row, c.col))
            slot->clearSample();

    // 4. Write moving-set content into every target slot
    for (int i = 0; i < dragTargetCells.size(); ++i)
    {
        if (auto* slot = getSlotAt(dragTargetCells[i].row, dragTargetCells[i].col))
        {
            if (sourceFiles[i] != juce::File{}) slot->setSample(sourceFiles[i]);
            else                                slot->clearSample();
        }
    }

    // 5. Write displaced target-only content into the vacated source-only cells
    int pairs = juce::jmin(srcOnly.size(), tgtOnly.size());
    for (int k = 0; k < pairs; ++k)
    {
        if (auto* slot = getSlotAt(srcOnly[k].c.row, srcOnly[k].c.col))
        {
            if (tgtOnly[k].f != juce::File{}) slot->setSample(tgtOnly[k].f);
            else                              slot->clearSample();
        }
    }

    // 6. Selection follows moved cells; focus tracks its relative position
    int dr = dragTargetCells[0].row - selection[0].row;
    int dc = dragTargetCells[0].col - selection[0].col;
    focusCell = { focusCell.row + dr, focusCell.col + dc };
    selection = dragTargetCells;

    dragTargetCells.clear();
    updateDragTargetVisuals();
    updateSlotVisuals();
}

void BankEditorPanel::updateDragTargetVisuals()
{
    for (int b = 0; b < ChompiNamer::NUM_BANKS; ++b)
        for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
            if (auto* slot = getSlotAt(b, s))
                slot->setDragTarget(dragTargetCells.contains({b, s}));
}

void BankEditorPanel::updateDragPreviews()
{
    clearAllPreviews();

    if (dragTargetCells.size() != selection.size()) return;

    // Snapshot actual (non-preview) files before setting any previews
    juce::Array<juce::File> sourceFiles, targetFiles;
    for (int i = 0; i < selection.size(); ++i)
    {
        auto* s = getSlotAt(selection[i].row,       selection[i].col);
        auto* t = getSlotAt(dragTargetCells[i].row, dragTargetCells[i].col);
        sourceFiles.add(s ? s->getSample() : juce::File{});
        targetFiles.add(t ? t->getSample() : juce::File{});
    }

    // Target cells: show the incoming content
    for (int i = 0; i < dragTargetCells.size(); ++i)
        if (auto* slot = getSlotAt(dragTargetCells[i].row, dragTargetCells[i].col))
            slot->setPreviewSample(sourceFiles[i]);

    // Source-only cells: show the displaced content that will shift back there.
    // Mirrors the row-major pairing used in commitSelectionDrag.
    juce::Array<CellFile> srcOnly, tgtOnly;
    for (int i = 0; i < selection.size(); ++i)
        if (!dragTargetCells.contains(selection[i]))
            srcOnly.add({ selection[i], sourceFiles[i] });
    for (int j = 0; j < dragTargetCells.size(); ++j)
        if (!selection.contains(dragTargetCells[j]))
            tgtOnly.add({ dragTargetCells[j], targetFiles[j] });

    sortCellsRowMajor(srcOnly);
    sortCellsRowMajor(tgtOnly);

    int pairs = juce::jmin(srcOnly.size(), tgtOnly.size());
    for (int k = 0; k < pairs; ++k)
        if (auto* slot = getSlotAt(srcOnly[k].c.row, srcOnly[k].c.col))
            slot->setPreviewSample(tgtOnly[k].f);
}

void BankEditorPanel::clearAllPreviews()
{
    for (int b = 0; b < ChompiNamer::NUM_BANKS; ++b)
        for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
            if (auto* slot = getSlotAt(b, s))
            {
                slot->clearPreviewSample();
                slot->setSwapHighlight(false);
            }
}

void BankEditorPanel::clearDragState()
{
    isDragSelecting      = false;
    isSelectionDragging  = false;
    mouseDownOnSelected  = false;
    dragTargetCells.clear();
    clearAllPreviews();
    updateDragTargetVisuals();
}

// ─────────────────────────────────────────────────────────────────────────────

void BankEditorPanel::modifierKeysChanged(const juce::ModifierKeys&)
{
    // Drag mode no longer depends on modifier keys
}

void BankEditorPanel::mouseDown(const juce::MouseEvent&)
{
    clearDragState();
    clearSelection();
    if (onBackgroundClicked) onBackgroundClicked();
}

void BankEditorPanel::paint(juce::Graphics& g)
{
    g.fillAll(panelBg);
}

void BankEditorPanel::resized()
{
    auto area = getLocalBounds();

    if (banks.size() == 0) return;
    const float elemH = ((float)area.getHeight() - (ChompiNamer::NUM_BANKS - 1) * ChompiConstants::BANK_GAP)
                        / (float)ChompiNamer::NUM_BANKS;
    float y = (float)area.getY();
    for (int i = 0; i < ChompiNamer::NUM_BANKS; ++i)
    {
        const int top = juce::roundToInt(y);
        const int bot = juce::roundToInt(y + elemH);
        banks[i]->setBounds(area.getX(), top, area.getWidth(), bot - top);
        y += elemH + ChompiConstants::BANK_GAP;
    }

}

// ─── External file drag (FileDragAndDropTarget) ───────────────────────────────

bool BankEditorPanel::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (files.isEmpty() || files.size() > ChompiNamer::SLOTS_PER_BANK) return false;
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

    const int total       = ChompiNamer::NUM_BANKS * ChompiNamer::SLOTS_PER_BANK;
    const int startLinear = start.row * ChompiNamer::SLOTS_PER_BANK + start.col;

    for (int i = 0; i < count; ++i)
    {
        int linear = startLinear + i;
        if (linear >= total) break;
        result.add({ linear / ChompiNamer::SLOTS_PER_BANK, linear % ChompiNamer::SLOTS_PER_BANK });
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
