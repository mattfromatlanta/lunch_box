#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Logger.h"
#include "AudioScanner.h"
#include "AudioConverter.h"
#include "FileSystemHelper.h"
#include "ChompiNamer.h"

//==============================================================================
int main(int argc, char* argv[])
{
    // Initialize logger
    Logger logger;

    // Initialize application and audio format manager
    juce::AudioFormatManager formatManager;
    AudioConfiguration config;

    // Run initialization - exit early if it fails
    if (!initializeApplication(argc, argv, logger, formatManager, config))
    {
        return 1;
    }

    // Handle different operation modes
    if (config.mode == OperationMode::Scan)
    {
        // Legacy scan-only mode
        processAudioFiles(config.wavFiles, config.targetFolder, formatManager, logger);
        logger.logLine("Scan complete!");
    }
    else if (config.mode == OperationMode::Convert)
    {
        // Legacy conversion mode
        processAudioFiles(config.wavFiles, config.targetFolder, formatManager, logger);
        logger.logLine("Scan complete!");

        juce::File outputFolder = FileSystemHelper::getDefaultOutputDirectory();
        AudioConverter converter(logger);
        converter.convertFiles(config.wavFiles, config.targetFolder, outputFolder, formatManager);
    }
    else if (config.mode == OperationMode::Chompi)
    {
        // CHOMPI mode - process cubbi and jammi samples
        ChompiNamer namer(logger);
        AudioConverter converter(logger);

        // Ensure output folder exists
        if (!FileSystemHelper::ensureDirectoryExists(config.outputFolder, logger))
        {
            logger.logLine("Error: Could not create output directory");
            return 1;
        }

        // Process cubbi samples if specified
        if (config.hasCubbi)
        {
            logger.logLine("Processing Cubbi samples...");
            logger.logLine("");

            juce::Array<ChompiNamer::FileMapping> cubbiMappings =
                namer.processCategory(config.cubbiFolder, ChompiNamer::Category::Cubbi);

            logger.logLine("");
            logger.logLine("=== Converting Cubbi Files ===");
            logger.logLine("");

            // Convert each file with CHOMPI naming
            int successCount = 0;
            int errorCount = 0;

            for (const auto& mapping : cubbiMappings)
            {
                AudioConverter::ConversionResult result =
                    converter.convertFileWithName(mapping.sourceFile,
                                                  config.outputFolder,
                                                  mapping.outputFileName,
                                                  formatManager);

                if (result.success)
                    successCount++;
                else if (!result.skipped)
                    errorCount++;

                logger.logLine("");
            }

            logger.logLine("Cubbi conversion complete: " +
                          juce::String(successCount) + " files converted, " +
                          juce::String(errorCount) + " errors");
            logger.logLine("");
        }

        // Process jammi samples if specified
        if (config.hasJammi)
        {
            logger.logLine("Processing Jammi samples...");
            logger.logLine("");

            juce::Array<ChompiNamer::FileMapping> jammiMappings =
                namer.processCategory(config.jammiFolder, ChompiNamer::Category::Jammi);

            logger.logLine("");
            logger.logLine("=== Converting Jammi Files ===");
            logger.logLine("");

            // Convert each file with CHOMPI naming
            int successCount = 0;
            int errorCount = 0;

            for (const auto& mapping : jammiMappings)
            {
                AudioConverter::ConversionResult result =
                    converter.convertFileWithName(mapping.sourceFile,
                                                  config.outputFolder,
                                                  mapping.outputFileName,
                                                  formatManager);

                if (result.success)
                    successCount++;
                else if (!result.skipped)
                    errorCount++;

                logger.logLine("");
            }

            logger.logLine("Jammi conversion complete: " +
                          juce::String(successCount) + " files converted, " +
                          juce::String(errorCount) + " errors");
            logger.logLine("");
        }

        logger.logLine("CHOMPI processing complete!");
    }

    logger.logLine("");
    logger.logLine("All operations complete!");

    return 0;
}
