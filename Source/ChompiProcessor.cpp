#include "ChompiProcessor.h"
#include "FileSystemHelper.h"

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

    // Process cubbi samples if specified
    if (config.hasCubbi)
    {
        auto result = processCategory(config.cubbiFolder,
                                      config.outputFolder,
                                      ChompiNamer::Category::Cubbi,
                                      formatManager,
                                      converter);
        if (!result.success) overallSuccess = false;
    }

    // Process jammi samples if specified
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

ChompiProcessor::ProcessingResult ChompiProcessor::processCategoryFromAssignments(
    const juce::Array<BankFolderParser::BankAssignment>& assignments,
    const juce::File& outputFolder,
    ChompiNamer::Category category,
    juce::AudioFormatManager& formatManager,
    AudioConverter& converter)
{
    ProcessingResult result;
    juce::String categoryName   = ChompiNamer::categoryToString(category);
    juce::String categoryPrefix = categoryName.toLowerCase();

    logger.logLine("Processing " + categoryName + " samples (advanced mode)...");
    logger.logLine("");

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

ChompiProcessor::ProcessingResult ChompiProcessor::processCategory(
    const juce::File& sourceFolder,
    const juce::File& outputFolder,
    ChompiNamer::Category category,
    juce::AudioFormatManager& formatManager,
    AudioConverter& converter)
{
    ProcessingResult result;

    // Get category name for logging
    juce::String categoryName = ChompiNamer::categoryToString(category);

    // Log start of processing
    logger.logLine("Processing " + categoryName + " samples...");
    logger.logLine("");

    // Use BankFolderParser to discover files and assign to banks
    BankFolderParser parser(logger);
    juce::Array<BankFolderParser::BankAssignment> assignments =
        parser.parseFolderStructure(sourceFolder, category);

    result.filesProcessed = assignments.size();

    // If no files found, return early
    if (assignments.isEmpty())
    {
        logger.logLine("");
        return result;
    }

    // Log conversion phase
    logger.logLine("=== Converting " + categoryName + " Files ===");
    logger.logLine("");

    juce::String categoryPrefix = categoryName.toLowerCase();

    // Convert each file with CHOMPI naming derived from bank assignment
    for (const auto& assignment : assignments)
    {
        // Build CHOMPI output filename: e.g. cubbi_a1.wav
        juce::String outputFileName = categoryPrefix + "_" +
                                      juce::String::charToString(assignment.bankLetter) +
                                      juce::String(assignment.slotNumber) + ".wav";

        AudioConverter::ConversionResult conversionResult =
            converter.convertFileWithName(assignment.sourceFile,
                                         outputFolder,
                                         outputFileName,
                                         formatManager);

        if (conversionResult.success)
        {
            result.filesConverted++;

            // Generate optimized (_double) version from the converted base file
            juce::File baseOutput = outputFolder.getChildFile(outputFileName);
            juce::String doubleName = outputFileName.replace(".wav", "_double.wav");
            juce::File doubleOutput = outputFolder.getChildFile(doubleName);

            if (converter.generateOptimizedSample(baseOutput, doubleOutput, formatManager))
            {
                result.optimizedGenerated++;
            }
        }
        else if (conversionResult.skipped)
        {
            result.filesSkipped++;
        }
        else
        {
            result.errors++;
        }

        logger.logLine("");
    }

    // Log completion summary
    logger.logLine(categoryName + " conversion complete: " +
                  juce::String(result.filesConverted) + " files converted, " +
                  juce::String(result.optimizedGenerated) + " optimized versions generated, " +
                  juce::String(result.errors) + " errors");
    logger.logLine("");

    // Mark as failed if there were errors
    if (result.errors > 0)
    {
        result.success = false;
    }

    return result;
}

