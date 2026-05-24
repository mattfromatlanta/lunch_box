// SPDX-License-Identifier: AGPL-3.0-or-later
#include "BankFocusPanel.h"
#include "UIColours.h"
#include "UIConstants.h"
#include "ChompiFonts.h"
#include "../FileSystemHelper.h"

namespace
{
    const juce::Colour panelBg       = ChompiColours::DARK_GREY;
    const juce::Colour bankColBg     { 0xff0f1420 };

    // Slot-cell palette (matches BankSlotComponent)
    const juce::Colour slotEmptyBg   { 0xff1d2228 };
    const juce::Colour slotFilledBg  { 0xff3a5060 };
    const juce::Colour slotBorder    { 0xff3a4a5a };
    const juce::Colour slotFocusBdr  { 0xff99aaff };
    const juce::Colour slotNumCol    = ChompiColours::WHITE_CREAM.withAlpha(0.3f);

    static juce::Colour bankColourForIndex(int idx)
    {
        switch (idx)
        {
            case 0: return ChompiColours::RED;
            case 1: return ChompiColours::PINK_SALMON;
            case 2: return ChompiColours::YELLOW;
            case 3: return ChompiColours::TEAL;
            case 4: return ChompiColours::PURPLE;
            default: return ChompiColours::WHITE_CREAM;
        }
    }

    // LookAndFeel for bank selector buttons
    struct SlotStyleLAF : public juce::LookAndFeel_V4
    {
        void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                                  const juce::Colour& backgroundColour,
                                  bool isHighlighted, bool) override
        {
            auto bounds = btn.getLocalBounds().reduced(2).toFloat();
            const juce::Colour fill = btn.getToggleState()
                ? ChompiColours::getFocused(backgroundColour)
                : backgroundColour;
            g.setColour(isHighlighted ? fill.brighter(0.1f) : fill);
            g.fillRoundedRectangle(bounds, ChompiConstants::CORNER_RADIUS);
            g.setColour(ChompiColours::WHITE_CREAM.withAlpha(0.3f));
            g.drawRoundedRectangle(bounds, ChompiConstants::CORNER_RADIUS, ChompiConstants::BORDER_WIDTH_ACTIVE);
        }

        void drawButtonText(juce::Graphics& g, juce::TextButton& btn, bool, bool) override
        {
            const bool active = btn.getToggleState();

            if (active)
            {
                const float circleR = 14.0f;
                auto centre = btn.getLocalBounds().getCentre().toFloat();
                g.setColour(ChompiColours::getLabelBg(btn.findColour(juce::TextButton::buttonOnColourId)));
                g.fillEllipse(centre.x - circleR, centre.y - circleR, circleR * 2.0f, circleR * 2.0f);
                g.setColour(ChompiColours::WHITE_CREAM.withAlpha(0.5f));
                g.drawEllipse(centre.x - circleR, centre.y - circleR, circleR * 2.0f, circleR * 2.0f, 1.5f);
            }

            g.setColour(btn.findColour(active ? juce::TextButton::textColourOnId
                                              : juce::TextButton::textColourOffId));
            g.setFont(ChompiFonts::h3());
            g.drawText(btn.getButtonText(), btn.getLocalBounds(), juce::Justification::centred);
        }
    };

    // Compute the new slot arrangement after a multi-row drag.
    // Selected populated files move as a contiguous block to the destination;
    // non-selected populated files fill the remaining slots in their original order.
    //
    // currentSlots : SLOTS_PER_BANK current files (actual data, not preview)
    // sortedSel    : selected row indices, sorted ascending
    // from         : the row being physically dragged (must be in sortedSel)
    // to           : drag destination row index
    juce::Array<juce::File> computeMultiRowDrag(
        const juce::Array<juce::File>& currentSlots,
        const juce::Array<int>&        sortedSel,
        int from, int to)
    {
        const int N = currentSlots.size();

        // Populated files from the selection, in order
        juce::Array<juce::File> group;
        for (int i : sortedSel)
            if (i < N && currentSlots[i] != juce::File{})
                group.add(currentSlots[i]);

        if (group.isEmpty())
            return currentSlots;

        // Offset of the dragged file within the group
        int dragPosInGroup = 0;
        for (int i : sortedSel)
        {
            if (i == from) break;
            if (i < N && currentSlots[i] != juce::File{})
                ++dragPosInGroup;
        }

        // Target start: clamp so the whole block fits inside [0, N-1]
        int targetStart = juce::jlimit(0, N - group.size(), to - dragPosInGroup);

        // Populated non-selected files, in order
        juce::Array<juce::File> remaining;
        for (int i = 0; i < N; ++i)
            if (!sortedSel.contains(i) && currentSlots[i] != juce::File{})
                remaining.add(currentSlots[i]);

        // Build result: place group contiguously, then fill empty slots with remaining
        juce::Array<juce::File> result;
        for (int i = 0; i < N; ++i)
            result.add(juce::File{});

        for (int i = 0; i < group.size(); ++i)
            result.set(targetStart + i, group[i]);

        int ri = 0;
        for (int i = 0; i < N && ri < remaining.size(); ++i)
            if (result[i] == juce::File{})
                result.set(i, remaining[ri++]);

        return result;
    }
}

BankFocusPanel::BankFocusPanel(juce::AudioFormatManager& fmt,
                                juce::AudioThumbnailCache& cache)
    : formatManager(fmt), thumbnailCache(cache)
{
    // Bank selector buttons A-E
    const char bankLetters[] = { 'A', 'B', 'C', 'D', 'E' };
    for (int i = 0; i < ChompiNamer::NUM_BANKS; ++i)
    {
        bankButtons[i].setButtonText(juce::String::charToString(bankLetters[i]));
        bankButtons[i].setTooltip("Bank " + juce::String::charToString(bankLetters[i]) + " (slots 1-14)");
        bankButtons[i].onClick = [this, i] { switchToBank(i); };
        addAndMakeVisible(bankButtons[i]);
    }

    // Create 14 slot rows
    for (int i = 0; i < ChompiNamer::SLOTS_PER_BANK; ++i)
    {
        auto* row = rows.add(new FocusedSlotRow(i + 1, formatManager, thumbnailCache));
        wireRowCallbacks(row, i);
        addAndMakeVisible(row);
    }

    // Initialise selection to row 0
    selection.add(0);
    updateRowVisuals();

    // Set initial bank colour on all rows
    for (auto* row : rows)
        row->setBankColour(bankColourForIndex(activeBank));

    bankButtonLAF.reset(new SlotStyleLAF());
    for (int i = 0; i < ChompiNamer::NUM_BANKS; ++i)
        bankButtons[i].setLookAndFeel(bankButtonLAF.get());

    setWantsKeyboardFocus(true);
    updateBankButtonStyles();
}

BankFocusPanel::~BankFocusPanel()
{
    for (int i = 0; i < ChompiNamer::NUM_BANKS; ++i)
        bankButtons[i].setLookAndFeel(nullptr);
}

// ─── Data access ──────────────────────────────────────────────────────────────

juce::Array<BankFolderParser::BankAssignment>
BankFocusPanel::getAssignments(ChompiNamer::Category cat)
{
    if (!isPopulating) flushRowsToStorage();

    const int catIdx = (cat == ChompiNamer::Category::Cubbi) ? 0 : 1;
    const char bankLetters[] = { 'a', 'b', 'c', 'd', 'e' };
    juce::Array<BankFolderParser::BankAssignment> result;

    for (int b = 0; b < ChompiNamer::NUM_BANKS; ++b)
    {
        for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
        {
            const auto& f = slots[catIdx][b][s];
            if (f != juce::File{})
            {
                BankFolderParser::BankAssignment a;
                a.sourceFile     = f;
                a.bankLetter     = bankLetters[b];
                a.slotNumber     = s + 1;
                a.fromBankFolder = false;
                result.add(a);
            }
        }
    }
    return result;
}

void BankFocusPanel::setSlot(ChompiNamer::Category cat, int bankIdx, int slotIdx,
                              const juce::File& file)
{
    const int catIdx = (cat == ChompiNamer::Category::Cubbi) ? 0 : 1;
    if (bankIdx < 0 || bankIdx >= ChompiNamer::NUM_BANKS)    return;
    if (slotIdx < 0 || slotIdx >= ChompiNamer::SLOTS_PER_BANK) return;
    slots[catIdx][bankIdx][slotIdx] = file;

    // Refresh visible rows if this is the active bank/category
    if (cat == activeCategory && bankIdx == activeBank)
        populateRowsFromStorage();
}

juce::File BankFocusPanel::getSlotFile(ChompiNamer::Category cat, int bankIdx, int slotIdx)
{
    if (!isPopulating) flushRowsToStorage();
    const int catIdx = (cat == ChompiNamer::Category::Cubbi) ? 0 : 1;
    if (bankIdx < 0 || bankIdx >= ChompiNamer::NUM_BANKS)    return juce::File{};
    if (slotIdx < 0 || slotIdx >= ChompiNamer::SLOTS_PER_BANK) return juce::File{};
    return slots[catIdx][bankIdx][slotIdx];
}

void BankFocusPanel::clearAll()
{
    for (int c = 0; c < 2; ++c)
        for (int b = 0; b < ChompiNamer::NUM_BANKS; ++b)
            for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
                slots[c][b][s] = juce::File{};
    populateRowsFromStorage();
}

int BankFocusPanel::getFilledCount(ChompiNamer::Category cat)
{
    if (!isPopulating) flushRowsToStorage();

    const int catIdx = (cat == ChompiNamer::Category::Cubbi) ? 0 : 1;
    int count = 0;
    for (int b = 0; b < ChompiNamer::NUM_BANKS; ++b)
        for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
            if (slots[catIdx][b][s] != juce::File{}) ++count;
    return count;
}

// ─── Bulk operations ──────────────────────────────────────────────────────────

void BankFocusPanel::autoFillActiveFromFiles(const juce::Array<juce::File>& files)
{
    const int catIdx = (activeCategory == ChompiNamer::Category::Cubbi) ? 0 : 1;
    auto& bank = slots[catIdx][activeBank];

    int fileIdx = 0;
    for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK && fileIdx < files.size(); ++s)
    {
        if (bank[s] == juce::File{})
        {
            bank[s] = files[fileIdx++];
        }
    }

    int overflow = files.size() - fileIdx;
    if (overflow > 0 && onLog)
        onLog(juce::String(overflow) + " file(s) did not fit in this bank.");
}

void BankFocusPanel::clearActiveBank()
{
    const int catIdx = (activeCategory == ChompiNamer::Category::Cubbi) ? 0 : 1;
    for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
        slots[catIdx][activeBank][s] = juce::File{};
}

void BankFocusPanel::triggerAutoFill()
{
    juce::File startDir = getStartDirectory ? getStartDirectory() : juce::File{};
    if (startDir == juce::File{})
        startDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Folder to Auto-Fill From", startDir);

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this](const juce::FileChooser& chooser)
        {
            auto folder = chooser.getResult();
            if (folder == juce::File{} || !folder.isDirectory()) return;

            if (onFolderBrowsed) onFolderBrowsed(folder);
            if (onBeforeChange) onBeforeChange();

            juce::Array<juce::File> files;
            for (const auto& pattern : FileSystemHelper::getSupportedAudioExtensions())
                folder.findChildFiles(files, juce::File::findFiles, false, pattern);
            if (files.isEmpty()) return;

            flushRowsToStorage();
            autoFillActiveFromFiles(files);
            populateRowsFromStorage();
            if (onAssignmentsChanged) onAssignmentsChanged();
        });
}

void BankFocusPanel::triggerClear()
{
    clearActiveBank();
    populateRowsFromStorage();
    if (onAssignmentsChanged) onAssignmentsChanged();
}

// ─── Bank / category switching ────────────────────────────────────────────────

void BankFocusPanel::switchToBank(int bankIdx)
{
    if (bankIdx == activeBank) return;
    flushRowsToStorage();
    activeBank = bankIdx;
    focusedRowIdx = 0;  selectionAnchor = 0;
    selection.clear();  selection.add(0);
    populateRowsFromStorage();
    updateBankButtonStyles();
    updateRowVisuals();
}

void BankFocusPanel::switchToCategory(ChompiNamer::Category cat)
{
    if (cat == activeCategory) return;
    flushRowsToStorage();
    activeCategory = cat;
    focusedRowIdx = 0;  selectionAnchor = 0;
    selection.clear();  selection.add(0);
    populateRowsFromStorage();
    updateRowVisuals();
}

// ─── Storage ↔ row sync ───────────────────────────────────────────────────────

void BankFocusPanel::flushRowsToStorage()
{
    const int catIdx = (activeCategory == ChompiNamer::Category::Cubbi) ? 0 : 1;
    for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
        slots[catIdx][activeBank][s] = rows[s]->getSample();
}

void BankFocusPanel::populateRowsFromStorage()
{
    const int catIdx = (activeCategory == ChompiNamer::Category::Cubbi) ? 0 : 1;
    const juce::Colour bankCol = bankColourForIndex(activeBank);
    isPopulating = true;
    for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
    {
        rows[s]->setBankColour(bankCol);
        rows[s]->setSample(slots[catIdx][activeBank][s]);
    }
    isPopulating = false;
    if (onAssignmentsChanged) onAssignmentsChanged();
}

// ─── Row callbacks ────────────────────────────────────────────────────────────

void BankFocusPanel::wireRowCallbacks(FocusedSlotRow* row, int /*rowIdx*/)
{
    row->onSampleChanged = [this](FocusedSlotRow*)
    {
        if (!isPopulating && onAssignmentsChanged) onAssignmentsChanged();
    };

    row->onSlotClicked = [this](FocusedSlotRow* r)
    {
        if (onSlotClicked) onSlotClicked(r->getSample());
    };

    row->onRowDoubleClicked = [this](FocusedSlotRow* r)
    {
        if (onBeforeChange) onBeforeChange();
        r->browseForFile();
    };

    row->getStartDirectory = [this]() -> juce::File
    {
        return getStartDirectory ? getStartDirectory() : juce::File{};
    };

    row->onFolderBrowsed = [this](juce::File dir)
    {
        if (onFolderBrowsed) onFolderBrowsed(dir);
    };

    row->onRowMouseDown = [this](FocusedSlotRow* r, const juce::MouseEvent& e)
    {
        handleRowMouseDown(r, e);
    };

    row->onRowMouseDrag = [this](FocusedSlotRow* r, const juce::MouseEvent& e)
    {
        handleRowMouseDrag(r, e);
    };

    row->onRowMouseUp = [this](FocusedSlotRow* r, const juce::MouseEvent& e)
    {
        handleRowMouseUp(r, e);
    };
}

// ─── Drag-to-reorder ──────────────────────────────────────────────────────────

int BankFocusPanel::rowIndexFor(FocusedSlotRow* row) const
{
    for (int i = 0; i < rows.size(); ++i)
        if (rows[i] == row) return i;
    return -1;
}

int BankFocusPanel::rowAtPoint(int /*x*/, int y) const
{
    if (rows.isEmpty()) return 0;

    for (int i = 0; i < rows.size(); ++i)
        if (rows[i]->getY() <= y && y < rows[i]->getBottom())
            return i;

    if (y < rows[0]->getY()) return 0;
    return rows.size() - 1;
}

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
        const int catIdx = (activeCategory == ChompiNamer::Category::Cubbi) ? 0 : 1;
        const int N      = ChompiNamer::SLOTS_PER_BANK;

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
    const int catIdx = (activeCategory == ChompiNamer::Category::Cubbi) ? 0 : 1;
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
    const int N    = ChompiNamer::SLOTS_PER_BANK;

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

// ─── Selection helpers ────────────────────────────────────────────────────────

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
        rows[i]->setSelected(selection.contains(i));
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
    const int catIdx = (activeCategory == ChompiNamer::Category::Cubbi) ? 0 : 1;

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
    const int catIdx = (activeCategory == ChompiNamer::Category::Cubbi) ? 0 : 1;
    for (int i = 0; i < files.size(); ++i)
    {
        int slot = focusedRowIdx + i;
        if (slot >= ChompiNamer::SLOTS_PER_BANK) break;
        slots[catIdx][activeBank][slot] = files[i];
    }
    populateRowsFromStorage();
    if (onAssignmentsChanged) onAssignmentsChanged();
}

// ─── Public keyboard-navigation API ──────────────────────────────────────────

void BankFocusPanel::selectAll()
{
    selection.clear();
    for (int i = 0; i < ChompiNamer::SLOTS_PER_BANK; ++i)
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
    int next = juce::jlimit(0, ChompiNamer::SLOTS_PER_BANK - 1, focusedRowIdx + delta);
    setFocusedRow(next, true);
}

void BankFocusPanel::expandRowSelection(int delta)
{
    if (selection.isEmpty()) return;

    int minS = selection[0], maxS = selection[0];
    for (int i : selection) { minS = juce::jmin(minS, i);  maxS = juce::jmax(maxS, i); }

    if (delta < 0) minS = juce::jmax(0, minS - 1);
    else           maxS = juce::jmin(ChompiNamer::SLOTS_PER_BANK - 1, maxS + 1);

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

// ─── Styling ──────────────────────────────────────────────────────────────────

void BankFocusPanel::styleTabButton(juce::TextButton& btn, bool active, int bankIdx)
{
    const juce::Colour bankCol = bankColourForIndex(bankIdx);
    btn.setToggleState(active, juce::dontSendNotification);
    btn.setColour(juce::TextButton::buttonColourId,   ChompiColours::BUTTON_BG);
    btn.setColour(juce::TextButton::buttonOnColourId, bankCol);
    btn.setColour(juce::TextButton::textColourOffId,  ChompiColours::WHITE_CREAM.withAlpha(0.7f));
    btn.setColour(juce::TextButton::textColourOnId,   ChompiColours::WHITE_CREAM);
}

void BankFocusPanel::updateBankButtonStyles()
{
    for (int i = 0; i < ChompiNamer::NUM_BANKS; ++i)
        styleTabButton(bankButtons[i], i == activeBank, i);
}

// ─── Paint / layout ───────────────────────────────────────────────────────────

void BankFocusPanel::paint(juce::Graphics& g)
{
    g.fillAll(panelBg);
}

void BankFocusPanel::resized()
{
    auto area = getLocalBounds();

    // Bank column: width = one cell width (W/7), height matches Pack bank rows exactly
    const float bankColWf  = (float)area.getWidth() / 7.0f;
    const float bankBtnHf  = ((float)area.getHeight() - (ChompiNamer::NUM_BANKS - 1) * ChompiConstants::BANK_GAP)
                             / (float)ChompiNamer::NUM_BANKS;
    const float totalBankHf = bankBtnHf * ChompiNamer::NUM_BANKS
                              + ChompiConstants::BANK_GAP * (ChompiNamer::NUM_BANKS - 1);

    const int bankColW = juce::roundToInt(bankColWf);
    auto bankCol = area.removeFromLeft(bankColW);
    area.removeFromLeft(juce::roundToInt(ChompiConstants::BANK_GAP));

    float y = (float)bankCol.getY();
    for (int i = 0; i < ChompiNamer::NUM_BANKS; ++i)
    {
        const int top = juce::roundToInt(y);
        const int bot = juce::roundToInt(y + bankBtnHf);
        bankButtons[i].setBounds(bankCol.getX(), top, bankCol.getWidth(), bot - top);
        y += bankBtnHf + ChompiConstants::BANK_GAP;
    }

    // Slot rows: derived from totalBankHf so they scale in exact sync with bank buttons
    const float rowHf = (totalBankHf - (ChompiNamer::SLOTS_PER_BANK - 1) * ChompiConstants::SLOT_ROW_GAP)
                        / (float)ChompiNamer::SLOTS_PER_BANK;

    y = (float)area.getY();
    for (int i = 0; i < ChompiNamer::SLOTS_PER_BANK; ++i)
    {
        const int top = juce::roundToInt(y);
        const int bot = juce::roundToInt(y + rowHf);
        rows[i]->setBounds(area.getX(), top, area.getWidth(), bot - top);
        y += rowHf + ChompiConstants::SLOT_ROW_GAP;
    }
}

// ─── External file drag (FileDragAndDropTarget) ───────────────────────────────

bool BankFocusPanel::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (files.isEmpty() || files.size() > ChompiNamer::SLOTS_PER_BANK) return false;
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
    clearAllPreviews();
}

void BankFocusPanel::filesDropped(const juce::StringArray& files, int x, int y)
{
    int startRow = rowAtPoint(x, y);

    externalDragFiles.clear();
    clearAllPreviews();

    if (onBeforeChange) onBeforeChange();

    const int catIdx = (activeCategory == ChompiNamer::Category::Cubbi) ? 0 : 1;
    for (int i = 0; i < files.size() && startRow + i < ChompiNamer::SLOTS_PER_BANK; ++i)
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
    clearAllPreviews();

    int startRow = rowAtPoint(x, y);
    if (startRow < 0 || startRow >= rows.size()) return;

    for (int i = 0; i < externalDragFiles.size() && startRow + i < rows.size(); ++i)
    {
        rows[startRow + i]->setDragSource(true);
        rows[startRow + i]->setPreviewSample(juce::File(externalDragFiles[i]));
    }
}
