#include "BankEditorPanel.h"

namespace
{
    const juce::Colour panelBg     { 0xff1a1f2e };
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

    autoFillButton.setButtonText("Auto-Fill from Folder...");
    autoFillButton.setColour(juce::TextButton::buttonColourId,  buttonCol);
    autoFillButton.setColour(juce::TextButton::textColourOffId, buttonTxt);
    autoFillButton.onClick = [this] { autoFillFromFolder(juce::File{}); };
    addAndMakeVisible(autoFillButton);

    clearAllButton.setButtonText("Clear All");
    clearAllButton.setColour(juce::TextButton::buttonColourId,  buttonCol);
    clearAllButton.setColour(juce::TextButton::textColourOffId, buttonTxt);
    clearAllButton.onClick = [this] { clearAllBanks(); };
    addAndMakeVisible(clearAllButton);

    sortAllButton.setButtonText("Sort A-Z");
    sortAllButton.setColour(juce::TextButton::buttonColourId,  buttonCol);
    sortAllButton.setColour(juce::TextButton::textColourOffId, buttonTxt);
    sortAllButton.onClick = [this] { sortAllAlphabetically(); };
    addAndMakeVisible(sortAllButton);

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

void BankEditorPanel::sortAllAlphabetically()
{
    for (auto* bank : banks)
        bank->sortSlotsAlphabetically();
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
    selection.clear();
    int r0 = juce::jmin(a.row, b.row), r1 = juce::jmax(a.row, b.row);
    int c0 = juce::jmin(a.col, b.col), c1 = juce::jmax(a.col, b.col);
    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c)
            selection.add({r, c});
    focusCell = b;
    updateSlotVisuals();
}

void BankEditorPanel::clearSelection()
{
    selection.clear();
    updateSlotVisuals();
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

void BankEditorPanel::moveFocus(int dr, int dc)
{
    Cell anchor = (selection.size() > 1) ? getEarliestSelected() : focusCell;
    Cell next = {
        juce::jlimit(0, ChompiNamer::NUM_BANKS      - 1, anchor.row + dr),
        juce::jlimit(0, ChompiNamer::SLOTS_PER_BANK - 1, anchor.col + dc)
    };
    selection.clear();
    selection.add(next);
    focusCell = next;
    updateSlotVisuals();
}

void BankEditorPanel::expandSelection(int dRow, int dCol)
{
    if (selection.isEmpty()) return;

    // Compute bounding box of current selection
    int minRow = ChompiNamer::NUM_BANKS,      maxRow = -1;
    int minCol = ChompiNamer::SLOTS_PER_BANK, maxCol = -1;
    for (const auto& c : selection)
    {
        minRow = std::min(minRow, c.row); maxRow = std::max(maxRow, c.row);
        minCol = std::min(minCol, c.col); maxCol = std::max(maxCol, c.col);
    }

    // Expand bbox by 1 in the requested direction, clamped to grid bounds
    int newMinRow = minRow, newMaxRow = maxRow;
    int newMinCol = minCol, newMaxCol = maxCol;

    if      (dRow > 0) newMaxRow = std::min(maxRow + 1, ChompiNamer::NUM_BANKS      - 1);
    else if (dRow < 0) newMinRow = std::max(minRow - 1, 0);
    if      (dCol > 0) newMaxCol = std::min(maxCol + 1, ChompiNamer::SLOTS_PER_BANK - 1);
    else if (dCol < 0) newMinCol = std::max(minCol - 1, 0);

    // Already at the border — nothing to do
    if (newMinRow == minRow && newMaxRow == maxRow &&
        newMinCol == minCol && newMaxCol == maxCol)
        return;

    // Rebuild selection as the full expanded rectangle
    selection.clear();
    for (int r = newMinRow; r <= newMaxRow; ++r)
        for (int c = newMinCol; c <= newMaxCol; ++c)
            selection.add({ r, c });

    // Focus tracks the expanding edge
    if      (dRow > 0) focusCell = { newMaxRow, focusCell.col };
    else if (dRow < 0) focusCell = { newMinRow, focusCell.col };
    else if (dCol > 0) focusCell = { focusCell.row, newMaxCol };
    else if (dCol < 0) focusCell = { focusCell.row, newMinCol };

    updateSlotVisuals();
}

void BankEditorPanel::tabFocus()
{
    Cell next = { (focusCell.row + 1) % ChompiNamer::NUM_BANKS, focusCell.col };
    selection.clear();
    selection.add(next);
    focusCell = next;
    updateSlotVisuals();
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
        slot->browseForFile();
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
    }
    else if (e.mods.isCommandDown())
    {
        toggleCell(c);
    }
    else if (selection.contains(c))
    {
        // Defer: could become a drag-move or a click-to-collapse
        mouseDownOnSelected = true;
    }
    else
    {
        selectCell(c, true);
        dragAnchor = c;
    }

    grabKeyboardFocus();
}

void BankEditorPanel::handleSlotMouseDrag(BankSlotComponent* src, const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;
    if (e.getDistanceFromDragStart() < 5) return;

    auto panelPt = e.getEventRelativeTo(this).getPosition();
    Cell hover   = getCellAtPoint(panelPt);

    // Decide drag mode on first move past threshold
    if (!isDragSelecting && !isSelectionDragging)
    {
        if (mouseDownOnSelected)
        {
            isSelectionDragging   = true;
            selectionDragStart    = mouseDownCell;
            mouseDownOnSelected   = false;
        }
        else
        {
            isDragSelecting = true;
            dragAnchor = getCellFor(src);
            if (!dragAnchor.isValid()) dragAnchor = hover.isValid() ? hover : Cell{0,0};
        }
    }

    if (isDragSelecting)
    {
        if (!hover.isValid()) return;
        selectRange(dragAnchor, hover);
    }
    else if (isSelectionDragging)
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
        updateDragPreviews(e.mods.isCommandDown());
    }
}

void BankEditorPanel::handleSlotMouseUp(BankSlotComponent*, const juce::MouseEvent& e)
{
    if (isSelectionDragging && !dragTargetCells.isEmpty())
    {
        commitSelectionDrag(e.mods.isCommandDown());
    }
    else if (mouseDownOnSelected)
    {
        // Simple click on a selected cell — collapse selection to just that cell
        selectCell(mouseDownCell, true);
    }

    clearDragState();
}

void BankEditorPanel::commitSelectionDrag(bool doSwap)
{
    if (dragTargetCells.size() != selection.size()) return;

    // 1. Snapshot ALL source and target files before any mutation
    juce::Array<juce::File> sourceFiles, targetFiles;
    for (int i = 0; i < selection.size(); ++i)
    {
        auto* s = getSlotAt(selection[i].row,       selection[i].col);
        auto* t = getSlotAt(dragTargetCells[i].row, dragTargetCells[i].col);
        sourceFiles.add(s ? s->getSample() : juce::File{});
        targetFiles.add(t ? t->getSample() : juce::File{});
    }

    // 2. For swap: build source-only and target-only (cell, file) pairs, sorted row-major.
    //    Sorting by position (not by selection-array order) ensures that the pairing is
    //    geometrically stable regardless of how the selection was constructed (drag-select
    //    always gives row-major order, but cmd-click can produce arbitrary order).
    //    Overlap cells carry the moving-set content through and are excluded from the swap.
    juce::Array<CellFile> srcOnly, tgtOnly;
    if (doSwap)
    {
        for (int i = 0; i < selection.size(); ++i)
            if (!dragTargetCells.contains(selection[i]))
                srcOnly.add({ selection[i], sourceFiles[i] });
        for (int j = 0; j < dragTargetCells.size(); ++j)
            if (!selection.contains(dragTargetCells[j]))
                tgtOnly.add({ dragTargetCells[j], targetFiles[j] });

        sortCellsRowMajor(srcOnly);
        sortCellsRowMajor(tgtOnly);
    }

    // 3. Clear ALL source slots (overlap slots will be re-filled in step 4)
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

    // 5. Swap: write displaced content to vacated source-only cells (row-major paired).
    if (doSwap)
    {
        int pairs = juce::jmin(srcOnly.size(), tgtOnly.size());
        for (int k = 0; k < pairs; ++k)
        {
            if (auto* slot = getSlotAt(srcOnly[k].c.row, srcOnly[k].c.col))
            {
                if (tgtOnly[k].f != juce::File{}) slot->setSample(tgtOnly[k].f);
                else                              slot->clearSample();
            }
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

void BankEditorPanel::updateDragPreviews(bool isSwap)
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

    // Target cells: show what moving-set content will land there
    for (int i = 0; i < dragTargetCells.size(); ++i)
        if (auto* slot = getSlotAt(dragTargetCells[i].row, dragTargetCells[i].col))
            slot->setPreviewSample(sourceFiles[i]);

    if (isSwap)
    {
        // Mirror the row-major sorted pairing from commitSelectionDrag
        juce::Array<CellFile> srcOnly, tgtOnly;
        for (int i = 0; i < selection.size(); ++i)
            if (!dragTargetCells.contains(selection[i]))
                srcOnly.add({ selection[i], sourceFiles[i] });
        for (int j = 0; j < dragTargetCells.size(); ++j)
            if (!selection.contains(dragTargetCells[j]))
                tgtOnly.add({ dragTargetCells[j], targetFiles[j] });

        sortCellsRowMajor(srcOnly);
        sortCellsRowMajor(tgtOnly);

        // Source-only cells: preview shows the displaced content they'll receive
        int pairs = juce::jmin(srcOnly.size(), tgtOnly.size());
        for (int k = 0; k < pairs; ++k)
            if (auto* slot = getSlotAt(srcOnly[k].c.row, srcOnly[k].c.col))
                slot->setPreviewSample(tgtOnly[k].f);

        // All selected cells: warm swap highlight
        for (const auto& c : selection)
            if (auto* slot = getSlotAt(c.row, c.col))
                slot->setSwapHighlight(true);
    }
    else
    {
        // Move mode: source-only cells preview as empty (they'll be vacated)
        for (int i = 0; i < selection.size(); ++i)
            if (!dragTargetCells.contains(selection[i]))
                if (auto* slot = getSlotAt(selection[i].row, selection[i].col))
                    slot->setPreviewSample(juce::File{});
    }
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

void BankEditorPanel::modifierKeysChanged(const juce::ModifierKeys& mods)
{
    if (isSelectionDragging)
        updateDragPreviews(mods.isCommandDown());
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

    // Bank rows
    for (auto* bank : banks)
    {
        bank->setBounds(area.removeFromTop(ROW_HEIGHT));
        area.removeFromTop(ROW_GAP);
    }

    area.removeFromTop(4);

    // Action buttons evenly across bottom
    auto btnArea = area.removeFromTop(BUTTON_HEIGHT);
    int btnW = btnArea.getWidth() / 3;
    autoFillButton.setBounds(btnArea.removeFromLeft(btnW).reduced(2, 0));
    clearAllButton.setBounds(btnArea.removeFromLeft(btnW).reduced(2, 0));
    sortAllButton.setBounds(btnArea.reduced(2, 0));
}
