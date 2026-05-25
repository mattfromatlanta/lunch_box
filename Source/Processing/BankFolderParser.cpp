// SPDX-License-Identifier: AGPL-3.0-or-later
#include "BankFolderParser.h"
#include "../FileSystemHelper.h"

BankFolderParser::BankFolderParser(Logger& logger)
    : logger(logger)
{
}

bool BankFolderParser::isBankFolder(const juce::String& folderName, char& bankLetter)
{
    auto lower = folderName.toLowerCase().trim();

    // Single letter: "a", "b", "c", "d", "e"
    if (lower.length() == 1 && lower[0] >= 'a' && lower[0] <= 'e')
    {
        bankLetter = lower[0];
        return true;
    }

    // Pattern: "bank_a", "bank-a", "bank a", "Bank A", etc.
    if (lower.startsWith("bank"))
    {
        auto suffix = lower.fromFirstOccurrenceOf("bank", false, false)
                          .removeCharacters("_- ").trim();
        if (suffix.length() == 1 && suffix[0] >= 'a' && suffix[0] <= 'e')
        {
            bankLetter = suffix[0];
            return true;
        }
    }

    return false;
}

juce::Array<BankFolderParser::BankAssignment> BankFolderParser::parseFolderStructure(
    const juce::File& sourceFolder,
    LunchBoxNamer::Category category)
{
    juce::Array<BankAssignment> assignments;
    juce::String categoryName = LunchBoxNamer::categoryToString(category);

    // Step 1: Find top-level bank subfolders (A-E) and collect their files
    std::map<char, juce::Array<juce::File>> bankFiles;
    juce::Array<juce::File> bankFolderDirs;

    juce::Array<juce::File> subdirs;
    sourceFolder.findChildFiles(subdirs, juce::File::findDirectories, false);

    for (const auto& dir : subdirs)
    {
        char bankLetter;
        if (isBankFolder(dir.getFileName(), bankLetter))
        {
            juce::Array<juce::File> files;
            findAudioFiles(dir, files, true);
            sortFilesAlphabetically(files);
            bankFiles[bankLetter] = files;
            bankFolderDirs.add(dir);

            logger.logLine("Found bank folder: " + dir.getFileName() +
                          " (" + juce::String(files.size()) + " files)");
        }
    }

    // Step 2: Find unsorted files (not inside any recognized bank folder)
    juce::Array<juce::File> allFiles;
    findAudioFiles(sourceFolder, allFiles, true);

    juce::Array<juce::File> unsortedFiles;
    for (const auto& file : allFiles)
    {
        bool inBankFolder = false;
        for (const auto& bankDir : bankFolderDirs)
        {
            if (file.isAChildOf(bankDir))
            {
                inBankFolder = true;
                break;
            }
        }
        if (!inBankFolder)
            unsortedFiles.add(file);
    }
    sortFilesAlphabetically(unsortedFiles);

    // Log file counts
    if (!bankFolderDirs.isEmpty())
    {
        logger.logLine("Found " + juce::String(unsortedFiles.size()) + " unsorted files");
    }
    else
    {
        logger.logLine("Found " + juce::String(unsortedFiles.size()) + " audio files in " +
                      categoryName + " folder");
    }

    // Step 3: Assign bank folder files to their banks (A first, then B, C, D, E)
    std::map<char, int> slotsUsed;
    for (char b = 'a'; b <= 'e'; ++b)
        slotsUsed[b] = 0;

    for (char bank = 'a'; bank <= 'e'; ++bank)
    {
        if (bankFiles.count(bank) == 0) continue;

        const auto& files = bankFiles.at(bank);
        int count = 0;

        int overflow = files.size() - LunchBoxNamer::SLOTS_PER_BANK;
        if (overflow > 0)
        {
            logger.logLine("Warning: Bank " + juce::String::charToString(bank).toUpperCase() +
                          " has " + juce::String(files.size()) + " files, only " +
                          juce::String(LunchBoxNamer::SLOTS_PER_BANK) + " slots available. " +
                          juce::String(overflow) + " ignored:");
            for (int i = LunchBoxNamer::SLOTS_PER_BANK; i < files.size(); ++i)
                logger.logLine("  Ignored: " + files[i].getFileName());
        }

        for (const auto& file : files)
        {
            if (count >= LunchBoxNamer::SLOTS_PER_BANK)
                break;

            BankAssignment a;
            a.sourceFile = file;
            a.bankLetter = bank;
            a.slotNumber = count + 1;
            a.fromBankFolder = true;
            assignments.add(a);
            count++;
        }

        slotsUsed[bank] = count;
    }

    // Step 4a: Fill empty banks (no input folder) with unsorted files first
    int unsortedIndex = 0;
    for (char bank = 'a'; bank <= 'e' && unsortedIndex < unsortedFiles.size(); ++bank)
    {
        if (bankFiles.count(bank) > 0) continue; // skip banks that had folders

        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK && unsortedIndex < unsortedFiles.size(); ++s)
        {
            BankAssignment a;
            a.sourceFile = unsortedFiles[unsortedIndex];
            a.bankLetter = bank;
            a.slotNumber = s + 1;
            a.fromBankFolder = false;
            assignments.add(a);
            slotsUsed[bank]++;
            unsortedIndex++;
        }
    }

    // Step 4b: Fill remaining space in banks that had folders
    for (char bank = 'a'; bank <= 'e' && unsortedIndex < unsortedFiles.size(); ++bank)
    {
        if (bankFiles.count(bank) == 0) continue; // skip empty banks (already handled)

        int used = slotsUsed[bank];
        int available = LunchBoxNamer::SLOTS_PER_BANK - used;

        for (int s = 0; s < available && unsortedIndex < unsortedFiles.size(); ++s)
        {
            BankAssignment a;
            a.sourceFile = unsortedFiles[unsortedIndex];
            a.bankLetter = bank;
            a.slotNumber = used + s + 1;
            a.fromBankFolder = false;
            assignments.add(a);
            unsortedIndex++;
        }
    }

    // Log any unsorted files that couldn't be placed
    if (unsortedIndex < unsortedFiles.size())
    {
        int skipped = unsortedFiles.size() - unsortedIndex;
        logger.logLine("Warning: " + juce::String(skipped) +
                      " unsorted files ignored (no remaining slots):");
        for (int i = unsortedIndex; i < unsortedFiles.size(); ++i)
            logger.logLine("  Ignored: " + unsortedFiles[i].getFileName());
    }

    // Log bank assignment summary
    logger.logLine("");
    logger.logLine("=== " + categoryName + " Bank Assignment ===");

    std::map<char, int> totalPerBank;
    std::map<char, int> fromFolderPerBank;
    for (const auto& a : assignments)
    {
        totalPerBank[a.bankLetter]++;
        if (a.fromBankFolder)
            fromFolderPerBank[a.bankLetter]++;
    }

    bool anyBank = false;
    for (char bank = 'a'; bank <= 'e'; ++bank)
    {
        if (totalPerBank.count(bank) == 0) continue;

        int total = totalPerBank[bank];
        int fromFolder = fromFolderPerBank.count(bank) ? fromFolderPerBank[bank] : 0;
        int fromUnsorted = total - fromFolder;

        juce::String bankStr = juce::String::charToString(bank).toUpperCase();
        juce::String detail = "";

        if (!bankFolderDirs.isEmpty())
        {
            if (fromFolder > 0 && fromUnsorted > 0)
                detail = " (" + juce::String(fromFolder) + " from folder, " +
                         juce::String(fromUnsorted) + " unsorted)";
            else if (fromFolder > 0)
                detail = " (from bank folder)";
            else
                detail = " (unsorted)";
        }

        logger.logLine("Bank " + bankStr + " (" + juce::String(total) + "/" +
                      juce::String(LunchBoxNamer::SLOTS_PER_BANK) + " slots filled)" + detail);
        anyBank = true;
    }

    if (!anyBank)
        logger.logLine("No files to assign");

    logger.logLine("");

    return assignments;
}

void BankFolderParser::findAudioFiles(const juce::File& folder,
                                      juce::Array<juce::File>& results,
                                      bool recursive)
{
    for (const auto& pattern : FileSystemHelper::getSupportedAudioExtensions())
        folder.findChildFiles(results, juce::File::findFiles, recursive, pattern);
}

void BankFolderParser::sortFilesAlphabetically(juce::Array<juce::File>& files)
{
    struct FileNameComparator
    {
        int compareElements(const juce::File& first, const juce::File& second) const
        {
            return first.getFileName().compareNatural(second.getFileName());
        }
    };
    FileNameComparator comparator;
    files.sort(comparator);
}
