// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_core/juce_core.h>
#include "../Logger.h"

//==============================================================================
// LunchBoxNamer - Handles CHOMPI sampler naming convention
//==============================================================================
// CHOMPI uses two categories of sample banks:
// - cubbi banks: percussive, loop, or SFX samples
// - jammi banks: tuned samples to play chromatically
//
// Each category has 5 banks (A-E) with 14 slots each = 70 samples total
// Naming format: {category}_{bank}{slot}.wav (e.g., cubbi_a1.wav, jammi_e14.wav)
//==============================================================================

class LunchBoxNamer
{
public:
    // CHOMPI hardware structure constants (shared across all modules)
    static constexpr int SLOTS_PER_BANK         = 14;
    static constexpr int NUM_BANKS              = 5;
    static constexpr int MAX_FILES_PER_CATEGORY = 70;

    // Sample category types
    enum class Category
    {
        Cubbi,  // Percussive, loop, or SFX samples
        Jammi   // Tuned samples to play chromatically
    };

    // Bank and slot information
    struct BankSlot
    {
        char bank;      // 'a', 'b', 'c', 'd', 'e'
        int slot;       // 1-14
    };

    // Mapping from source file to CHOMPI output name
    struct FileMapping
    {
        juce::File sourceFile;
        juce::String outputFileName;
        BankSlot bankSlot;
        int sourceIndex;  // Position in sorted list (0-based)
    };

    explicit LunchBoxNamer(Logger& loggerToUse);

    // Generate CHOMPI filename for a given index and category
    // index: 0-69 (0 = a1, 13 = a14, 14 = b1, 69 = e14)
    juce::String generateFileName(Category category, int fileIndex) const;

    // Convert file index (0-69) to bank and slot
    static BankSlot indexToBankSlot(int index);

    // Convert bank and slot to string component (e.g., "a1", "b14")
    static juce::String bankSlotToString(const BankSlot& bankSlot);

    // Get category name as string
    static juce::String categoryToString(Category category);

private:
    Logger& logger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LunchBoxNamer)
};
