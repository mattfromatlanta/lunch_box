#include "ChompiProcessor.h"
#include "FileSystemHelper.h"

ChompiProcessor::ChompiProcessor(Logger& logger)
    : logger(logger)
{
}

bool ChompiProcessor::processSamples(const AudioConfiguration& config,
                                     juce::AudioFormatManager& formatManager)
{
    // Create CHOMPI processing components
    ChompiNamer namer(logger);
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
                       namer,
                       converter);
    }

    // Process jammi samples if specified
    if (config.hasJammi)
    {
        processCategory(config.jammiFolder,
                       config.outputFolder,
                       ChompiNamer::Category::Jammi,
                       formatManager,
                       namer,
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
    ChompiNamer& namer,
    AudioConverter& converter)
{
    ProcessingResult result;

    // Get category name for logging
    juce::String categoryName = getCategoryName(category);

    // Log start of processing
    logger.logLine("Processing " + categoryName + " samples...");
    logger.logLine("");

    // Generate file mappings using ChompiNamer
    juce::Array<ChompiNamer::FileMapping> mappings =
        namer.processCategory(sourceFolder, category);

    result.filesProcessed = mappings.size();

    // If no files found, return early
    if (mappings.isEmpty())
    {
        logger.logLine("");
        return result;
    }

    // Log conversion phase
    logger.logLine("");
    logger.logLine("=== Converting " + categoryName + " Files ===");
    logger.logLine("");

    // Convert each file with CHOMPI naming
    for (const auto& mapping : mappings)
    {
        AudioConverter::ConversionResult conversionResult =
            converter.convertFileWithName(mapping.sourceFile,
                                         outputFolder,
                                         mapping.outputFileName,
                                         formatManager);

        if (conversionResult.success)
        {
            result.filesConverted++;
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
