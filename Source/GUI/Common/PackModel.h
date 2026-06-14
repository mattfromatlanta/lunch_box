// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_core/juce_core.h>
#include "../../Processing/LunchBoxNamer.h"
#include "../../Processing/BankFolderParser.h"

//==============================================================================
// PackModel - Single authoritative store for all pack slot assignments.
//==============================================================================
// Holds the file assigned to every slot, indexed by [category][bank][slot]:
// two categories (Cubbi, Jammi), five banks (A-E), fourteen slots each.
//
// The Pack grid (BankEditorPanel) and the Bank focus list (BankFocusPanel) both
// read from and write to this one model, so there is a single source of truth
// and no cross-view copying. Views push edits here and repopulate their widgets
// from here when they become visible.
//==============================================================================

class PackModel
{
public:
    static constexpr int NUM_CATEGORIES = 2;  // 0 = Cubbi, 1 = Jammi

    PackModel() = default;

    // Read the file at one slot. Out-of-range coordinates return an empty File.
    juce::File getSlot(LunchBoxNamer::Category category, int bank, int slot) const;

    // Assign (or clear, when file is empty) one slot. Returns true if the stored
    // value actually changed.
    bool setSlot(LunchBoxNamer::Category category, int bank, int slot, const juce::File& file);

    // Empty every slot in one category, or in both.
    void clearCategory(LunchBoxNamer::Category category);
    void clearAll();

    // Number of filled slots in a category.
    int getFilledCount(LunchBoxNamer::Category category) const;

    // Filled slots as conversion-ready assignments (bank letter + slot number).
    juce::Array<BankFolderParser::BankAssignment> getAssignments(LunchBoxNamer::Category category) const;

    // Full-state snapshot for undo/redo and session save/restore.
    struct Snapshot
    {
        juce::File slots[NUM_CATEGORIES][LunchBoxNamer::NUM_BANKS][LunchBoxNamer::SLOTS_PER_BANK];
    };

    Snapshot snapshot() const;
    void restore(const Snapshot& state);

private:
    static int categoryIndex(LunchBoxNamer::Category category)
    {
        return category == LunchBoxNamer::Category::Cubbi ? 0 : 1;
    }

    static bool inRange(int bank, int slot)
    {
        return bank >= 0 && bank < LunchBoxNamer::NUM_BANKS
            && slot >= 0 && slot < LunchBoxNamer::SLOTS_PER_BANK;
    }

    juce::File slots[NUM_CATEGORIES][LunchBoxNamer::NUM_BANKS][LunchBoxNamer::SLOTS_PER_BANK];
};
