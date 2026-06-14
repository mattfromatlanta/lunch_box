// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PackModel.h"

juce::File PackModel::getSlot(LunchBoxNamer::Category category, int bank, int slot) const
{
    if (!inRange(bank, slot)) return {};
    return slots[categoryIndex(category)][bank][slot];
}

bool PackModel::setSlot(LunchBoxNamer::Category category, int bank, int slot, const juce::File& file)
{
    if (!inRange(bank, slot)) return false;

    auto& stored = slots[categoryIndex(category)][bank][slot];
    if (stored == file) return false;

    stored = file;
    return true;
}

void PackModel::clearCategory(LunchBoxNamer::Category category)
{
    const int cat = categoryIndex(category);
    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
            slots[cat][b][s] = juce::File{};
}

void PackModel::clearAll()
{
    clearCategory(LunchBoxNamer::Category::Cubbi);
    clearCategory(LunchBoxNamer::Category::Jammi);
}

int PackModel::getFilledCount(LunchBoxNamer::Category category) const
{
    const int cat = categoryIndex(category);
    int count = 0;
    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
            if (slots[cat][b][s] != juce::File{}) ++count;
    return count;
}

juce::Array<BankFolderParser::BankAssignment>
PackModel::getAssignments(LunchBoxNamer::Category category) const
{
    const int cat = categoryIndex(category);
    const char bankLetters[] = { 'a', 'b', 'c', 'd', 'e' };
    juce::Array<BankFolderParser::BankAssignment> result;

    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
        {
            const auto& f = slots[cat][b][s];
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
    return result;
}

PackModel::Snapshot PackModel::snapshot() const
{
    Snapshot s;
    for (int c = 0; c < NUM_CATEGORIES; ++c)
        for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
            for (int slot = 0; slot < LunchBoxNamer::SLOTS_PER_BANK; ++slot)
                s.slots[c][b][slot] = slots[c][b][slot];
    return s;
}

void PackModel::restore(const Snapshot& state)
{
    for (int c = 0; c < NUM_CATEGORIES; ++c)
        for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
            for (int slot = 0; slot < LunchBoxNamer::SLOTS_PER_BANK; ++slot)
                slots[c][b][slot] = state.slots[c][b][slot];
}
