#include "BankFocusPanel.h"
#include "../FileSystemHelper.h"

namespace
{
    const juce::Colour panelBg       { 0xff151a26 };
    const juce::Colour bankColBg     { 0xff0f1420 };
    const juce::Colour activeTabBg   { 0xff2a5a7a };
    const juce::Colour activeTabTxt  { 0xffffffff };
    const juce::Colour inactiveTabBg { 0xff1a2030 };
    const juce::Colour inactiveTabTxt{ 0xff6677aa };
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
        bankButtons[i].onClick = [this, i] { switchToBank(i); };
        addAndMakeVisible(bankButtons[i]);
    }

    // Action buttons
    addAndMakeVisible(sortButton);
    addAndMakeVisible(autoFillButton);
    addAndMakeVisible(clearButton);

    sortButton.onClick = [this]
    {
        flushRowsToStorage();
        sortActiveAlphabetically();
        populateRowsFromStorage();
        if (onAssignmentsChanged) onAssignmentsChanged();
    };

    autoFillButton.onClick = [this]
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

                juce::Array<juce::File> files;
                for (const auto& pattern : FileSystemHelper::getSupportedAudioExtensions())
                    folder.findChildFiles(files, juce::File::findFiles, false, pattern);
                if (files.isEmpty()) return;

                flushRowsToStorage();
                autoFillActiveFromFiles(files);
                populateRowsFromStorage();
                if (onAssignmentsChanged) onAssignmentsChanged();
            });
    };

    clearButton.onClick = [this]
    {
        clearActiveBank();
        populateRowsFromStorage();
        if (onAssignmentsChanged) onAssignmentsChanged();
    };

    // Create 14 slot rows
    for (int i = 0; i < ChompiNamer::SLOTS_PER_BANK; ++i)
    {
        auto* row = rows.add(new FocusedSlotRow(i + 1, formatManager, thumbnailCache));
        wireRowCallbacks(row, i);
        addAndMakeVisible(row);
    }

    updateBankButtonStyles();
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

void BankFocusPanel::sortActiveAlphabetically()
{
    const int catIdx = (activeCategory == ChompiNamer::Category::Cubbi) ? 0 : 1;
    auto& bank = slots[catIdx][activeBank];

    juce::Array<juce::File> filled;
    for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
        if (bank[s] != juce::File{}) filled.add(bank[s]);

    if (filled.isEmpty()) return;

    struct NameComp
    {
        int compareElements(const juce::File& a, const juce::File& b) const
        {
            return a.getFileName().compareNatural(b.getFileName());
        }
    } comp;
    filled.sort(comp);

    for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
        bank[s] = (s < filled.size()) ? filled[s] : juce::File{};
}

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

// ─── Bank / category switching ────────────────────────────────────────────────

void BankFocusPanel::switchToBank(int bankIdx)
{
    if (bankIdx == activeBank) return;
    flushRowsToStorage();
    activeBank = bankIdx;
    populateRowsFromStorage();
    updateBankButtonStyles();
}

void BankFocusPanel::switchToCategory(ChompiNamer::Category cat)
{
    if (cat == activeCategory) return;
    flushRowsToStorage();
    activeCategory = cat;
    populateRowsFromStorage();
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
    isPopulating = true;
    for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
        rows[s]->setSample(slots[catIdx][activeBank][s]);
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

int BankFocusPanel::rowAtY(int y) const
{
    for (int i = 0; i < rows.size(); ++i)
    {
        if (rows[i]->getBounds().contains(0, y))
            return i;
    }
    // Clamp to first/last
    if (y < rows[0]->getY())    return 0;
    return rows.size() - 1;
}

void BankFocusPanel::handleRowMouseDown(FocusedSlotRow* row, const juce::MouseEvent& e)
{
    // Single click without drag: select + preview
    int idx = rowIndexFor(row);
    if (idx < 0) return;

    // Deselect all
    for (auto* r : rows) r->setSelected(false);
    row->setSelected(true);

    dragSourceIdx = idx;
    isDragging    = false;

    if (row->hasSample())
    {
        if (onSlotClicked) onSlotClicked(row->getSample());
    }
    else
    {
        if (onPreviewStop) onPreviewStop();
    }

    (void)e;
}

void BankFocusPanel::handleRowMouseDrag(FocusedSlotRow* row, const juce::MouseEvent& e)
{
    int idx = rowIndexFor(row);
    if (idx < 0) return;

    // Begin drag after a small threshold
    if (!isDragging && std::abs(e.getDistanceFromDragStartY()) > 6)
    {
        isDragging = true;
        rows[dragSourceIdx]->setDragSource(true);
    }

    if (!isDragging) return;

    // Find insertion point from panel-space Y
    int panelY    = row->getY() + e.getPosition().getY();
    int insertIdx = rowAtY(panelY);

    if (insertIdx != dragInsertIdx)
    {
        if (dragInsertIdx >= 0) rows[dragInsertIdx]->setInsertionTarget(false);
        dragInsertIdx = insertIdx;
        if (dragInsertIdx >= 0) rows[dragInsertIdx]->setInsertionTarget(true);
    }
}

void BankFocusPanel::handleRowMouseUp(FocusedSlotRow* row, const juce::MouseEvent&)
{
    if (isDragging && dragSourceIdx >= 0 && dragInsertIdx >= 0
        && dragSourceIdx != dragInsertIdx)
    {
        flushRowsToStorage();
        commitReorder(dragSourceIdx, dragInsertIdx);
        populateRowsFromStorage();
        if (onAssignmentsChanged) onAssignmentsChanged();
    }

    clearReorderState();
    (void)row;
}

void BankFocusPanel::commitReorder(int fromIdx, int toIdx)
{
    const int catIdx = (activeCategory == ChompiNamer::Category::Cubbi) ? 0 : 1;
    auto& bank = slots[catIdx][activeBank];

    juce::File moving = bank[fromIdx];

    if (toIdx < fromIdx)
    {
        // Shift rows between toIdx and fromIdx-1 down by one
        for (int i = fromIdx; i > toIdx; --i)
            bank[i] = bank[i - 1];
    }
    else
    {
        // Shift rows between fromIdx+1 and toIdx up by one
        for (int i = fromIdx; i < toIdx; ++i)
            bank[i] = bank[i + 1];
    }
    bank[toIdx] = moving;
}

void BankFocusPanel::clearReorderState()
{
    if (dragSourceIdx >= 0 && dragSourceIdx < rows.size())
        rows[dragSourceIdx]->setDragSource(false);
    if (dragInsertIdx >= 0 && dragInsertIdx < rows.size())
        rows[dragInsertIdx]->setInsertionTarget(false);

    dragSourceIdx = -1;
    dragInsertIdx = -1;
    isDragging    = false;
}

// ─── Styling ──────────────────────────────────────────────────────────────────

void BankFocusPanel::styleTabButton(juce::TextButton& btn, bool active)
{
    btn.setColour(juce::TextButton::buttonColourId,
                  active ? activeTabBg : inactiveTabBg);
    btn.setColour(juce::TextButton::textColourOffId,
                  active ? activeTabTxt : inactiveTabTxt);
    btn.setColour(juce::TextButton::textColourOnId,
                  active ? activeTabTxt : inactiveTabTxt);
}

void BankFocusPanel::updateBankButtonStyles()
{
    for (int i = 0; i < ChompiNamer::NUM_BANKS; ++i)
        styleTabButton(bankButtons[i], i == activeBank);
}

// ─── Paint / layout ───────────────────────────────────────────────────────────

void BankFocusPanel::paint(juce::Graphics& g)
{
    g.fillAll(panelBg);

    // Left bank column background
    g.setColour(bankColBg);
    g.fillRect(0, 0, BANK_COL_WIDTH, getHeight());
}

void BankFocusPanel::resized()
{
    auto area = getLocalBounds();

    // Left column: bank selector buttons (A-E), evenly distributed vertically
    auto bankCol = area.removeFromLeft(BANK_COL_WIDTH);
    int bankBtnH = bankCol.getHeight() / ChompiNamer::NUM_BANKS;
    for (int i = 0; i < ChompiNamer::NUM_BANKS; ++i)
        bankButtons[i].setBounds(bankCol.removeFromTop(bankBtnH).reduced(2, 2));

    // Bottom: action buttons
    auto actionRow = area.removeFromBottom(ACTION_ROW_H);
    int actW = actionRow.getWidth() / 3;
    sortButton.setBounds    (actionRow.removeFromLeft(actW).reduced(2, 2));
    autoFillButton.setBounds(actionRow.removeFromLeft(actW).reduced(2, 2));
    clearButton.setBounds   (actionRow.reduced(2, 2));

    // Remaining area: 14 rows, evenly divided
    int rowH = area.getHeight() / ChompiNamer::SLOTS_PER_BANK;
    for (auto* row : rows)
        row->setBounds(area.removeFromTop(rowH));
}
