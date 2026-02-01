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

juce::Array<ChompiNamer::FileMapping> ChompiNamer::processCategory(
    const juce::File& sourceFolder,
    Category category)
{
    juce::Array<FileMapping> mappings;

    // Validate folder
    if (!sourceFolder.exists() || !sourceFolder.isDirectory())
    {
        logger.logLine("Error: Invalid source folder: " + sourceFolder.getFullPathName());
        return mappings;
    }

    // Scan for WAV files
    juce::Array<juce::File> wavFiles;
    sourceFolder.findChildFiles(wavFiles,
                                juce::File::findFiles,
                                true,  // recursive
                                "*.wav");

    // Log initial count
    logger.logLine("Found " + juce::String(wavFiles.size()) + " WAV files in " +
                   categoryToString(category) + " folder");

    // Check if any files found
    if (wavFiles.isEmpty())
    {
        logger.logLine("Warning: No WAV files found in " + categoryToString(category) + " folder");
        return mappings;
    }

    // Sort alphabetically
    sortFilesAlphabetically(wavFiles);

    // Determine how many files to process
    int filesToProcess = juce::jmin(wavFiles.size(), MAX_FILES_PER_CATEGORY);

    // Warn if more than 70 files
    if (wavFiles.size() > MAX_FILES_PER_CATEGORY)
    {
        logger.logLine("Warning: " + categoryToString(category) + " has " +
                       juce::String(wavFiles.size()) + " files. Only first " +
                       juce::String(MAX_FILES_PER_CATEGORY) + " will be processed.");
        logger.logLine("Skipping " + juce::String(wavFiles.size() - MAX_FILES_PER_CATEGORY) +
                       " files (beyond 70-file limit)");
    }

    // Create mappings for first 70 files
    for (int i = 0; i < filesToProcess; ++i)
    {
        FileMapping mapping;
        mapping.sourceFile = wavFiles[i];
        mapping.outputFileName = generateFileName(category, i);
        mapping.bankSlot = indexToBankSlot(i);
        mapping.sourceIndex = i;

        mappings.add(mapping);
    }

    // Log bank summary
    logger.logLine("");
    logger.logLine("=== " + categoryToString(category) + " Bank Assignment ===");

    int completeBanks = filesToProcess / SLOTS_PER_BANK;
    int remainingSlots = filesToProcess % SLOTS_PER_BANK;

    for (int bankIndex = 0; bankIndex < NUM_BANKS; ++bankIndex)
    {
        char bankLetter = 'a' + static_cast<char>(bankIndex);
        int slotsInBank = 0;

        if (bankIndex < completeBanks)
        {
            slotsInBank = SLOTS_PER_BANK;
        }
        else if (bankIndex == completeBanks)
        {
            slotsInBank = remainingSlots;
        }

        if (slotsInBank > 0)
        {
            logger.logLine("Bank " + juce::String::charToString(bankLetter).toUpperCase() +
                           " (" + juce::String(slotsInBank) + "/" +
                           juce::String(SLOTS_PER_BANK) + " slots filled)");
        }
        else if (bankIndex == completeBanks && remainingSlots == 0)
        {
            // This bank is empty - only log for first empty bank
            logger.logLine("Bank " + juce::String::charToString(bankLetter).toUpperCase() +
                           " (0/" + juce::String(SLOTS_PER_BANK) + " slots) - incomplete");
            break;  // Don't show remaining empty banks
        }
    }

    logger.logLine("");

    return mappings;
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

void ChompiNamer::sortFilesAlphabetically(juce::Array<juce::File>& files)
{
    // Sort using case-insensitive natural comparison
    // JUCE Array::sort() requires a comparator with compareElements method
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
