// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ChompiNamer.h"

ChompiNamer::ChompiNamer(Logger& logger)
    : logger(logger)
{
}

juce::String ChompiNamer::generateFileName(Category category, int fileIndex) const
{
    // Validate index range
    if (fileIndex < 0 || fileIndex >= MAX_FILES_PER_CATEGORY)
    {
        logger.logLine("Warning: File index out of range: " + juce::String(fileIndex));
        return "";
    }

    // Get bank and slot
    BankSlot bankSlot = indexToBankSlot(fileIndex);

    // Build filename: {category}_{bank}{slot}.wav
    juce::String filename = categoryToString(category).toLowerCase() + "_" +
                            bankSlotToString(bankSlot) + ".wav";

    return filename;
}

ChompiNamer::BankSlot ChompiNamer::indexToBankSlot(int index)
{
    BankSlot bankSlot;

    // Calculate bank (0-4 maps to 'a'-'e')
    int bankIndex = index / SLOTS_PER_BANK;
    bankSlot.bank = 'a' + static_cast<char>(bankIndex);

    // Calculate slot (1-14)
    bankSlot.slot = (index % SLOTS_PER_BANK) + 1;

    return bankSlot;
}

juce::String ChompiNamer::bankSlotToString(const BankSlot& bankSlot)
{
    return juce::String::charToString(bankSlot.bank) + juce::String(bankSlot.slot);
}

juce::String ChompiNamer::categoryToString(Category category)
{
    switch (category)
    {
        case Category::Cubbi:
            return "Cubbi";
        case Category::Jammi:
            return "Jammi";
        default:
            return "Unknown";
    }
}

