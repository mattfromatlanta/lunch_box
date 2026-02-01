#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Logger.h"
#include "AudioScanner.h"
#include "AudioConverter.h"
#include "FileSystemHelper.h"
#include "ChompiProcessor.h"

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
        ChompiProcessor processor(logger);

        if (!processor.processSamples(config, formatManager))
        {
            return 1;
        }
    }

    logger.logLine("");
    logger.logLine("All operations complete!");

    return 0;
}
