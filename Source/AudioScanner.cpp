#include "AudioScanner.h"

bool initializeApplication(int argc, char* argv[],
                          Logger& logger,
                          juce::AudioFormatManager& formatManager,
                          juce::File& targetFolder,
                          juce::Array<juce::File>& wavFiles)
{
    logger.logLine("==================================");
    logger.logLine("Chompi Pack - Audio File Scanner");
    logger.logLine("==================================");
    logger.logLine("");

    // Register audio formats
    formatManager.registerBasicFormats();

    logger.logLine("JUCE modules loaded successfully!");
    logger.logLine("Registered audio formats: " + juce::String(formatManager.getNumKnownFormats()));
    logger.logLine("");

    // Check command-line arguments
    if (argc < 2)
    {
        logger.logLine("Usage: chompi_pack <folder_path>");
        logger.logLine("Example: chompi_pack /path/to/audio/folder");
        return false;
    }

    juce::String folderPath = argv[1];
    logger.logLine("Target folder: " + folderPath);

    // Verify folder exists
    targetFolder = juce::File(folderPath);
    if (!targetFolder.exists())
    {
        logger.logLine("Error: Folder does not exist: " + folderPath);
        return false;
    }

    if (!targetFolder.isDirectory())
    {
        logger.logLine("Error: Path is not a directory: " + folderPath);
        return false;
    }

    logger.logLine("Folder validated successfully!");
    logger.logLine("");

    // Scan for WAV files recursively
    logger.logLine("Scanning for WAV files...");
    logger.logLine("");

    targetFolder.findChildFiles(wavFiles,
                                juce::File::findFiles,
                                true,  // search recursively
                                "*.wav");

    // Check if any files were found
    if (wavFiles.isEmpty())
    {
        logger.logLine("No WAV files found in the specified directory.");
        return false;
    }

    logger.logLine("Found " + juce::String(wavFiles.size()) + " WAV file(s):");
    logger.logLine("");

    return true;
}

void processAudioFiles(const juce::Array<juce::File>& wavFiles,
                      const juce::File& targetFolder,
                      juce::AudioFormatManager& formatManager,
                      Logger& logger)
{
    for (int i = 0; i < wavFiles.size(); ++i)
    {
        const juce::File& file = wavFiles[i];

        // Get relative path from target folder for cleaner display
        juce::String relativePath = file.getRelativePathFrom(targetFolder);
        if (relativePath.isEmpty())
            relativePath = file.getFileName();

        logger.logLine(juce::String(i + 1) + ". " + relativePath);

        // Analyze audio file properties
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

        if (reader != nullptr)
        {
            // Extract audio properties
            int numChannels = reader->numChannels;
            int bitsPerSample = reader->bitsPerSample;
            double sampleRate = reader->sampleRate;

            // Format channel information
            juce::String channelInfo;
            if (numChannels == 1)
                channelInfo = "Mono (1)";
            else if (numChannels == 2)
                channelInfo = "Stereo (2)";
            else
                channelInfo = "Multi-channel (" + juce::String(numChannels) + ")";

            // Display audio properties
            logger.logLine("   Channels: " + channelInfo);
            logger.logLine("   Bit Depth: " + juce::String(bitsPerSample) + "-bit");
            logger.logLine("   Sample Rate: " + juce::String((int)sampleRate) + " Hz");

            // Display file size
            juce::int64 fileSize = file.getSize();
            double fileSizeMB = fileSize / (1024.0 * 1024.0);
            logger.logLine("   Size: " + juce::String(fileSizeMB, 2) + " MB");
        }
        else
        {
            // Could not read file
            logger.logLine("   Error: Unable to read audio file (corrupted or unsupported format)");
        }

        logger.logLine("");
    }
}
