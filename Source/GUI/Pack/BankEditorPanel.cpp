// SPDX-License-Identifier: AGPL-3.0-or-later
//
// BankEditorPanel — core: ctor, data accessors (assignments, slot get/set,
// clear, auto-fill), cell-coordinate helpers, paint/resized. Selection,
// drag-move, and external file-drag logic live in sibling translation units:
//   BankEditorPanel_Selection.cpp
//   BankEditorPanel_Drag.cpp
//   BankEditorPanel_ExternalDrag.cpp

#include "BankEditorPanel.h"
#include "BankEditorPanel_Private.h"
#include "UIColours.h"
#include "UIConstants.h"
#include "LabelStrings.h"
#include "../FileSystemHelper.h"

using namespace BankEditorImpl;

BankEditorPanel::BankEditorPanel(LunchBoxNamer::Category cat)
    : category(cat)
{
    const char letters[] = {'a', 'b', 'c', 'd', 'e'};
    for (int i = 0; i < LunchBoxNamer::NUM_BANKS; ++i)
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

    for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
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
        LunchBoxLabels::kChooseAutoFillFolder, startDir, "", true);

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
            if (bankIdx >= 0 && bankIdx < LunchBoxNamer::NUM_BANKS)
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
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
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
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
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
void BankEditorPanel::paint(juce::Graphics& g)
{
    g.fillAll(panelBg);
}

void BankEditorPanel::resized()
{
    auto area = getLocalBounds();

    if (banks.size() == 0) return;
    const float elemH = ((float)area.getHeight() - (LunchBoxNamer::NUM_BANKS - 1) * LunchBoxConstants::BANK_GAP)
                        / (float)LunchBoxNamer::NUM_BANKS;
    float y = (float)area.getY();
    for (int i = 0; i < LunchBoxNamer::NUM_BANKS; ++i)
    {
        const int top = juce::roundToInt(y);
        const int bot = juce::roundToInt(y + elemH);
        banks[i]->setBounds(area.getX(), top, area.getWidth(), bot - top);
        y += elemH + LunchBoxConstants::BANK_GAP;
    }

}
