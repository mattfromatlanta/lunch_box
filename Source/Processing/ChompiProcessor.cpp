// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ChompiProcessor.h"
#include "../FileSystemHelper.h"

ChompiProcessor::ChompiProcessor(Logger& logger)
    : logger(logger)
{
}

bool ChompiProcessor::processSamples(const AudioConfiguration& config,
                                     juce::AudioFormatManager& formatManager)
{
    AudioConverter converter(logger);

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
                                      ChompiNamer::Category::Cubbi,
                                      formatManager,
                                      converter);
        if (!result.success) overallSuccess = false;
    }

    if (config.hasJammi)
    {
        auto result = processCategory(config.jammiFolder,
                                      config.outputFolder,
                                      ChompiNamer::Category::Jammi,
                                      formatManager,
                                      converter);
        if (!result.success) overallSuccess = false;
    }

    logger.logLine("CHOMPI processing complete!");

    return overallSuccess;
}

ChompiProcessor::ProcessingResult ChompiProcessor::processCategory(
    const juce::File& sourceFolder,
    const juce::File& outputFolder,
    ChompiNamer::Category category,
    juce::AudioFormatManager& formatManager,
    AudioConverter& converter)
{
    juce::String categoryName = ChompiNamer::categoryToString(category);
    logger.logLine("Processing " + categoryName + " samples...");
    logger.logLine("");

    BankFolderParser parser(logger);
    juce::Array<BankFolderParser::BankAssignment> assignments =
        parser.parseFolderStructure(sourceFolder, category);

    return runConversions(assignments, outputFolder, category, formatManager, converter);
}

ChompiProcessor::ProcessingResult ChompiProcessor::processCategoryFromAssignments(
    const juce::Array<BankFolderParser::BankAssignment>& assignments,
    const juce::File& outputFolder,
    ChompiNamer::Category category,
    juce::AudioFormatManager& formatManager,
    AudioConverter& converter)
{
    juce::String categoryName = ChompiNamer::categoryToString(category);
    logger.logLine("Processing " + categoryName + " samples (advanced mode)...");
    logger.logLine("");

    return runConversions(assignments, outputFolder, category, formatManager, converter);
}

ChompiProcessor::ProcessingResult ChompiProcessor::runConversions(
    const juce::Array<BankFolderParser::BankAssignment>& assignments,
    const juce::File& outputFolder,
    ChompiNamer::Category category,
    juce::AudioFormatManager& formatManager,
    AudioConverter& converter)
{
    ProcessingResult result;
    juce::String categoryName   = ChompiNamer::categoryToString(category);
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

            juce::File baseOutput = outputFolder.getChildFile(outputFileName);
            juce::String doubleName = outputFileName.replace(".wav", "_double.wav");
            juce::File doubleOutput = outputFolder.getChildFile(doubleName);

            if (converter.generateOptimizedSample(baseOutput, doubleOutput, formatManager))
                result.optimizedGenerated++;
        }
        else if (convResult.skipped)
        {
            result.filesSkipped++;
        }
        else
        {
            result.errors++;
        }

        logger.logLine("");
    }

    logger.logLine(categoryName + " conversion complete: " +
                  juce::String(result.filesConverted) + " files converted, " +
                  juce::String(result.optimizedGenerated) + " optimized versions generated, " +
                  juce::String(result.errors) + " errors");
    logger.logLine("");

    if (result.errors > 0)
        result.success = false;

    return result;
}
