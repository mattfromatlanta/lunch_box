#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Logger.h"
#include "AudioScanner.h"

//==============================================================================
int main(int argc, char* argv[])
{
    // Initialize logger
    Logger logger;

    // Initialize application and audio format manager
    juce::AudioFormatManager formatManager;
    juce::File targetFolder;
    juce::Array<juce::File> wavFiles;

    // Run initialization - exit early if it fails
    if (!initializeApplication(argc, argv, logger, formatManager, targetFolder, wavFiles))
    {
        return 1;
    }

    // Process all audio files
    processAudioFiles(wavFiles, targetFolder, formatManager, logger);

    logger.logLine("Scan complete!");

    return 0;
}
