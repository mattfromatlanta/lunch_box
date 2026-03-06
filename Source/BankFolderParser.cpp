#include "BankFolderParser.h"

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
    ChompiNamer::Category category)
{
    juce::Array<BankAssignment> assignments;
    juce::String categoryName = ChompiNamer::categoryToString(category);

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

        for (const auto& file : files)
        {
            if (count >= SLOTS_PER_BANK)
            {
                logger.logLine("Warning: Bank " + juce::String::charToString(bank).toUpperCase() +
                              " has more than " + juce::String(SLOTS_PER_BANK) +
                              " files. Extras skipped.");
                break;
            }

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

    // Step 4: Fill remaining slots with unsorted files (bank A first, then B, C, D, E)
    int unsortedIndex = 0;
    for (char bank = 'a'; bank <= 'e' && unsortedIndex < unsortedFiles.size(); ++bank)
    {
        int used = slotsUsed[bank];
        int available = SLOTS_PER_BANK - used;

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

    // Warn if unsorted files exceeded capacity
    if (unsortedIndex < unsortedFiles.size())
    {
        int skipped = unsortedFiles.size() - unsortedIndex;
        logger.logLine("Warning: " + juce::String(skipped) +
                      " files skipped (" + juce::String(MAX_FILES) + " file limit reached)");
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
                      juce::String(SLOTS_PER_BANK) + " slots filled)" + detail);
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
    for (const auto& pattern : juce::StringArray{"*.wav", "*.aiff", "*.aif", "*.mp3", "*.flac"})
    {
        folder.findChildFiles(results, juce::File::findFiles, recursive, pattern);
    }
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
