// SPDX-License-Identifier: AGPL-3.0-or-later
//
// BankFocusPanel — core: ctor, data accessors, focus/category switching, row
// wiring, paint/resized. Drag-reorder, selection, and external file drag live
// in sibling translation units:
//   BankFocusPanel_Drag.cpp
//   BankFocusPanel_Selection.cpp
//   BankFocusPanel_ExternalDrag.cpp

#include "BankFocusPanel.h"
#include "BankFocusPanel_Private.h"
#include "UIColours.h"
#include "UIConstants.h"
#include "LunchBoxFonts.h"
#include "LabelStrings.h"
#include "../FileSystemHelper.h"

using namespace BankFocusImpl;


BankFocusPanel::BankFocusPanel(juce::AudioFormatManager& fmt,
                                juce::AudioThumbnailCache& cache)
    : formatManager(fmt), thumbnailCache(cache)
{
    // Bank selector buttons A-E
    const char bankLetters[] = { 'A', 'B', 'C', 'D', 'E' };
    for (int i = 0; i < LunchBoxNamer::NUM_BANKS; ++i)
    {
        bankButtons[i].setButtonText(juce::String::charToString(bankLetters[i]));
        bankButtons[i].setTooltip("Bank " + juce::String::charToString(bankLetters[i]) + " (slots 1-14)");
        bankButtons[i].onClick = [this, i] { switchToBank(i); };
        addAndMakeVisible(bankButtons[i]);
    }

    // Create 14 slot rows
    for (int i = 0; i < LunchBoxNamer::SLOTS_PER_BANK; ++i)
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
    for (int i = 0; i < LunchBoxNamer::NUM_BANKS; ++i)
        bankButtons[i].setLookAndFeel(bankButtonLAF.get());

    setWantsKeyboardFocus(true);
    updateBankButtonStyles();

    addAndMakeVisible(dragController.getProxy());
}

BankFocusPanel::~BankFocusPanel()
{
    for (int i = 0; i < LunchBoxNamer::NUM_BANKS; ++i)
        bankButtons[i].setLookAndFeel(nullptr);
}

// ─── Data access ──────────────────────────────────────────────────────────────

juce::Array<BankFolderParser::BankAssignment>
BankFocusPanel::getAssignments(LunchBoxNamer::Category cat)
{
    if (!isPopulating) flushRowsToStorage();

    const int catIdx = (cat == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    const char bankLetters[] = { 'a', 'b', 'c', 'd', 'e' };
    juce::Array<BankFolderParser::BankAssignment> result;

    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
    {
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
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

void BankFocusPanel::setSlot(LunchBoxNamer::Category cat, int bankIdx, int slotIdx,
                              const juce::File& file)
{
    const int catIdx = (cat == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    if (bankIdx < 0 || bankIdx >= LunchBoxNamer::NUM_BANKS)    return;
    if (slotIdx < 0 || slotIdx >= LunchBoxNamer::SLOTS_PER_BANK) return;
    slots[catIdx][bankIdx][slotIdx] = file;

    // Refresh visible rows if this is the active bank/category
    if (cat == activeCategory && bankIdx == activeBank)
        populateRowsFromStorage();
}

juce::File BankFocusPanel::getSlotFile(LunchBoxNamer::Category cat, int bankIdx, int slotIdx)
{
    if (!isPopulating) flushRowsToStorage();
    const int catIdx = (cat == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    if (bankIdx < 0 || bankIdx >= LunchBoxNamer::NUM_BANKS)    return juce::File{};
    if (slotIdx < 0 || slotIdx >= LunchBoxNamer::SLOTS_PER_BANK) return juce::File{};
    return slots[catIdx][bankIdx][slotIdx];
}

void BankFocusPanel::clearAll()
{
    for (int c = 0; c < 2; ++c)
        for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
            for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
                slots[c][b][s] = juce::File{};
    populateRowsFromStorage();
}

int BankFocusPanel::getFilledCount(LunchBoxNamer::Category cat)
{
    if (!isPopulating) flushRowsToStorage();

    const int catIdx = (cat == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    int count = 0;
    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
            if (slots[catIdx][b][s] != juce::File{}) ++count;
    return count;
}

// ─── Bulk operations ──────────────────────────────────────────────────────────

void BankFocusPanel::autoFillActiveFromFiles(const juce::Array<juce::File>& files)
{
    const int catIdx = (activeCategory == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    auto& bank = slots[catIdx][activeBank];

    int fileIdx = 0;
    for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK && fileIdx < files.size(); ++s)
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
    const int catIdx = (activeCategory == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
        slots[catIdx][activeBank][s] = juce::File{};
}

void BankFocusPanel::triggerAutoFill()
{
    juce::File startDir = getStartDirectory ? getStartDirectory() : juce::File{};
    if (startDir == juce::File{})
        startDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    fileChooser = std::make_unique<juce::FileChooser>(
        LunchBoxLabels::kChooseAutoFillBankFolder, startDir);

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

void BankFocusPanel::switchToCategory(LunchBoxNamer::Category cat)
{
    if (cat == activeCategory) return;
    flushRowsToStorage();

    // Persist current focus state before leaving
    const int outIdx = (activeCategory == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    perCategoryState[outIdx].activeBank    = activeBank;
    perCategoryState[outIdx].focusedRowIdx = focusedRowIdx;
    perCategoryState[outIdx].selection     = selection;

    // Restore saved focus state for the incoming category
    activeCategory  = cat;
    const int inIdx = (activeCategory == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    activeBank      = perCategoryState[inIdx].activeBank;
    focusedRowIdx   = perCategoryState[inIdx].focusedRowIdx;
    selectionAnchor = focusedRowIdx;
    selection       = perCategoryState[inIdx].selection;

    updateBankButtonStyles();
    populateRowsFromStorage();
    updateRowVisuals();
}

void BankFocusPanel::setActiveFocus(int bankIdx, int rowIdx)
{
    bankIdx = juce::jlimit(0, LunchBoxNamer::NUM_BANKS - 1, bankIdx);
    rowIdx  = juce::jlimit(0, LunchBoxNamer::SLOTS_PER_BANK - 1, rowIdx);

    if (bankIdx != activeBank)
    {
        flushRowsToStorage();
        activeBank = bankIdx;
        updateBankButtonStyles();
        populateRowsFromStorage();
    }

    focusedRowIdx   = rowIdx;
    selectionAnchor = rowIdx;
    selection.clear();
    selection.add(rowIdx);
    updateRowVisuals();
}

// ─── Storage ↔ row sync ───────────────────────────────────────────────────────

void BankFocusPanel::flushRowsToStorage()
{
    const int catIdx = (activeCategory == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
        slots[catIdx][activeBank][s] = rows[s]->getSample();
}

void BankFocusPanel::populateRowsFromStorage()
{
    const int catIdx = (activeCategory == LunchBoxNamer::Category::Cubbi) ? 0 : 1;
    const juce::Colour bankCol = bankColourForIndex(activeBank);
    isPopulating = true;
    for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
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

    // Inside a row's bounds — exact hit.
    for (int i = 0; i < rows.size(); ++i)
        if (rows[i]->getY() <= y && y < rows[i]->getBottom())
            return i;

    // Above the first row → clamp to 0.
    if (y < rows[0]->getY()) return 0;

    // Below the last row → clamp to last.
    if (y >= rows.getLast()->getBottom()) return rows.size() - 1;

    // In an inter-row gap: snap to whichever side's midpoint is closer.
    // Without this, falling through to the last-row return causes a visible
    // jump in the drag preview every time the cursor crosses a boundary.
    int best = 0;
    int bestDist = std::numeric_limits<int>::max();
    for (int i = 0; i < rows.size(); ++i)
    {
        const int centre = rows[i]->getY() + rows[i]->getHeight() / 2;
        const int dist   = std::abs(centre - y);
        if (dist < bestDist) { bestDist = dist; best = i; }
    }
    return best;
}
// ─── Styling ──────────────────────────────────────────────────────────────────

void BankFocusPanel::styleTabButton(juce::TextButton& btn, bool active, int bankIdx)
{
    const juce::Colour bankCol = bankColourForIndex(bankIdx);
    btn.setToggleState(active, juce::dontSendNotification);
    btn.setColour(juce::TextButton::buttonColourId,   LunchBoxColours::BUTTON_BG);
    btn.setColour(juce::TextButton::buttonOnColourId, bankCol);
    btn.setColour(juce::TextButton::textColourOffId,  LunchBoxColours::WHITE_CREAM.withAlpha(0.7f));
    btn.setColour(juce::TextButton::textColourOnId,   LunchBoxColours::WHITE_CREAM);
}

void BankFocusPanel::updateBankButtonStyles()
{
    for (int i = 0; i < LunchBoxNamer::NUM_BANKS; ++i)
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
    const float bankBtnHf  = ((float)area.getHeight() - (LunchBoxNamer::NUM_BANKS - 1) * LunchBoxConstants::BANK_GAP)
                             / (float)LunchBoxNamer::NUM_BANKS;
    const float totalBankHf = bankBtnHf * LunchBoxNamer::NUM_BANKS
                              + LunchBoxConstants::BANK_GAP * (LunchBoxNamer::NUM_BANKS - 1);

    const int bankColW = juce::roundToInt(bankColWf);
    auto bankCol = area.removeFromLeft(bankColW);
    area.removeFromLeft(juce::roundToInt(LunchBoxConstants::BANK_GAP));

    float y = (float)bankCol.getY();
    for (int i = 0; i < LunchBoxNamer::NUM_BANKS; ++i)
    {
        const int top = juce::roundToInt(y);
        const int bot = juce::roundToInt(y + bankBtnHf);
        bankButtons[i].setBounds(bankCol.getX(), top, bankCol.getWidth(), bot - top);
        y += bankBtnHf + LunchBoxConstants::BANK_GAP;
    }

    // Slot rows: derived from totalBankHf so they scale in exact sync with bank buttons
    const float rowHf = (totalBankHf - (LunchBoxNamer::SLOTS_PER_BANK - 1) * LunchBoxConstants::SLOT_ROW_GAP)
                        / (float)LunchBoxNamer::SLOTS_PER_BANK;

    y = (float)area.getY();
    for (int i = 0; i < LunchBoxNamer::SLOTS_PER_BANK; ++i)
    {
        const int top = juce::roundToInt(y);
        const int bot = juce::roundToInt(y + rowHf);
        rows[i]->setBounds(area.getX(), top, area.getWidth(), bot - top);
        y += rowHf + LunchBoxConstants::SLOT_ROW_GAP;
    }
}
