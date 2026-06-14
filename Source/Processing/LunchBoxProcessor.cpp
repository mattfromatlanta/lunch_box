// SPDX-License-Identifier: AGPL-3.0-or-later
#include "LunchBoxProcessor.h"
#include "../FileSystemHelper.h"

LunchBoxProcessor::LunchBoxProcessor(Logger& loggerToUse)
    : logger(loggerToUse)
{
}

bool LunchBoxProcessor::processSamples(const AudioConfiguration& config,
                                     juce::AudioFormatManager& formatManager)
{
    AudioConverter converter(logger, config.normalize);

    // Ensure output folder exists
    if (!FileSystemHelper::ensureDirectoryExists(config.outputFolder, logger))
    {
        logger.logLine("Error: Could not create output directory");
        return false;
    }

    bool overallSuccess = true;

    if (config.hasCubbi)
    {
        auto result = processCategory(config.cubbiFolder,
                                      config.outputFolder,
                                      LunchBoxNamer::Category::Cubbi,
                                      formatManager,
                                      converter);
        if (!result.success) overallSuccess = false;
    }

    if (config.hasJammi)
    {
        auto result = processCategory(config.jammiFolder,
                                      config.outputFolder,
                                      LunchBoxNamer::Category::Jammi,
                                      formatManager,
                                      converter);
        if (!result.success) overallSuccess = false;
    }

    logger.logLine("CHOMPI processing complete!");

    return overallSuccess;
}

LunchBoxProcessor::ProcessingResult LunchBoxProcessor::processCategory(
    const juce::File& sourceFolder,
    const juce::File& outputFolder,
    LunchBoxNamer::Category category,
    juce::AudioFormatManager& formatManager,
    AudioConverter& converter)
{
    juce::String categoryName = LunchBoxNamer::categoryToString(category);
    logger.logLine("Processing " + categoryName + " samples...");
    logger.logLine("");

    BankFolderParser parser(logger);
    juce::Array<BankFolderParser::BankAssignment> assignments =
        parser.parseFolderStructure(sourceFolder, category);

    return runConversions(assignments, outputFolder, category, formatManager, converter);
}

LunchBoxProcessor::ProcessingResult LunchBoxProcessor::processCategoryFromAssignments(
    const juce::Array<BankFolderParser::BankAssignment>& assignments,
    const juce::File& outputFolder,
    LunchBoxNamer::Category category,
    juce::AudioFormatManager& formatManager,
    AudioConverter& converter,
    ProgressCallback onFileProcessed,
    CancelCallback shouldCancel)
{
    juce::String categoryName = LunchBoxNamer::categoryToString(category);
    logger.logLine("Processing " + categoryName + " samples (advanced mode)...");
    logger.logLine("");

    return runConversions(assignments, outputFolder, category, formatManager, converter,
                          std::move(onFileProcessed), std::move(shouldCancel));
}

LunchBoxProcessor::ProcessingResult LunchBoxProcessor::runConversions(
    const juce::Array<BankFolderParser::BankAssignment>& assignments,
    const juce::File& outputFolder,
    LunchBoxNamer::Category category,
    juce::AudioFormatManager& formatManager,
    AudioConverter& converter,
    ProgressCallback onFileProcessed,
    CancelCallback shouldCancel)
{
    ProcessingResult result;
    juce::String categoryName   = LunchBoxNamer::categoryToString(category);
    juce::String categoryPrefix = categoryName.toLowerCase();

    result.filesProcessed = assignments.size();

    if (assignments.isEmpty())
    {
        logger.logLine("");
        return result;
    }

    logger.logLine("=== Converting " + categoryName + " Files ===");
    logger.logLine("");

    for (const auto& assignment : assignments)
    {
        if (shouldCancel && shouldCancel())
        {
            result.cancelled = true;
            logger.logLine(categoryName + " conversion cancelled.");
            return result;
        }

        juce::String outputFileName = categoryPrefix + "_" +
                                      juce::String::charToString(assignment.bankLetter) +
                                      juce::String(assignment.slotNumber) + ".wav";

        auto convResult = converter.convertFileWithName(assignment.sourceFile,
                                                        outputFolder,
                                                        outputFileName,
                                                        formatManager);
        if (convResult.success)
        {
            result.filesConverted++;
        }
        else if (convResult.skipped)
        {
            result.filesSkipped++;
        }
        else
        {
            result.errors++;
        }

        if (onFileProcessed) onFileProcessed();

        logger.logLine("");
    }

    logger.logLine(categoryName + " conversion complete: " +
                  juce::String(result.filesConverted) + " files converted, " +
                  juce::String(result.errors) + " errors");
    logger.logLine("");

    if (result.errors > 0)
        result.success = false;

    return result;
}
