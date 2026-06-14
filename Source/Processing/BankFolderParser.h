// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_core/juce_core.h>
#include <map>
#include <optional>
#include "../Logger.h"
#include "LunchBoxNamer.h"

//==============================================================================
// BankFolderParser - Detects bank subfolders and assigns samples to CHOMPI banks
//==============================================================================
// Supports folders named A-E, bank_a, Bank A, etc.
// Files in bank folders go to that bank; unsorted files fill remaining slots.
// Falls back to sequential assignment if no bank folders found.
//==============================================================================

class BankFolderParser
{
public:
    struct BankAssignment
    {
        juce::File sourceFile;
        char bankLetter;      // 'a' - 'e'
        int slotNumber;       // 1 - 14
        bool fromBankFolder;  // true = from A-E subfolder, false = unsorted
    };

    explicit BankFolderParser(Logger& loggerToUse);

    // Parse folder structure and return bank-assigned list ready for conversion.
    // If no bank subfolders exist, assigns sequentially (backward compatible).
    juce::Array<BankAssignment> parseFolderStructure(const juce::File& sourceFolder,
                                                      LunchBoxNamer::Category category);

    // Returns the bank letter ('a'-'e') if folderName names a CHOMPI bank
    // ("a", "Bank C", "bank_e", ...), or nullopt otherwise.
    static std::optional<char> parseBankFolder(const juce::String& folderName);

private:
    Logger& logger;

    // Find all supported audio files in a folder
    static void findAudioFiles(const juce::File& folder,
                               juce::Array<juce::File>& results,
                               bool recursive);

    // Sort files alphabetically (case-insensitive natural order)
    static void sortFilesAlphabetically(juce::Array<juce::File>& files);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankFolderParser)
};
