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

    // Process cubbi samples if specified
    if (config.hasCubbi)
    {
        processCategory(config.cubbiFolder,
                       config.outputFolder,
                       ChompiNamer::Category::Cubbi,
                       formatManager,
                       converter);
    }

    // Process jammi samples if specified
    if (config.hasJammi)
    {
        processCategory(config.jammiFolder,
                       config.outputFolder,
                       ChompiNamer::Category::Jammi,
                       formatManager,
                       converter);
    }

    logger.logLine("CHOMPI processing complete!");

    return true;
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
    juce::String categoryName = getCategoryName(category);

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

juce::String ChompiProcessor::getCategoryName(ChompiNamer::Category category) const
{
    return ChompiNamer::categoryToString(category);
}
