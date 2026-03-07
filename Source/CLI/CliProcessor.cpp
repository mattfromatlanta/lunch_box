#include "CliProcessor.h"
#include "../Processing/AudioConverter.h"
#include "../FileSystemHelper.h"
#include "../Processing/ChompiProcessor.h"

CliProcessor::CliProcessor()
{
}

int CliProcessor::run(const juce::StringArray& args)
{
    // Initialize application and parse arguments
    if (!initializeApplication(args))
    {
        return 1;
    }

    // Handle different operation modes
    if (config.mode == OperationMode::Scan)
    {
        // Legacy scan-only mode
        processAudioFiles(config.wavFiles, config.targetFolder);
        logger.logLine("Scan complete!");
    }
    else if (config.mode == OperationMode::Convert)
    {
        // Legacy conversion mode
        processAudioFiles(config.wavFiles, config.targetFolder);
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

void CliProcessor::displayUsage()
{
    logger.logLine("Usage: chompi_pack [OPTIONS]");
    logger.logLine("");
    logger.logLine("CHOMPI Mode (process samples for CHOMPI sampler):");
    logger.logLine("  --cubbi, --c <path>    Process cubbi samples (percussive/loop/SFX)");
    logger.logLine("  --jammi, --j <path>    Process jammi samples (tuned/chromatic)");
    logger.logLine("  --output, --o <path>   Output directory (default: converted/)");
    logger.logLine("");
    logger.logLine("Examples:");
    logger.logLine("  chompi_pack --cubbi /samples/cubbi --jammi /samples/jammi");
    logger.logLine("  chompi_pack --c /samples/cubbi --o /my/output");
    logger.logLine("  chompi_pack --j /samples/jammi");
    logger.logLine("");
    logger.logLine("Legacy Modes:");
    logger.logLine("  chompi_pack <folder>              Scan only");
    logger.logLine("  chompi_pack --convert <folder>    Convert without CHOMPI naming");
}

bool CliProcessor::initializeApplication(const juce::StringArray& args)
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

    // Check for minimum arguments
    if (args.isEmpty())
    {
        displayUsage();
        return false;
    }

    // Parse command-line arguments
    juce::String legacyFolderPath;
    juce::String cubbiPath;
    juce::String jammiPath;
    juce::String outputPath;
    bool hasConvertFlag = false;

    for (int i = 0; i < args.size(); ++i)
    {
        juce::String arg = args[i];

        if (arg == "--cubbi" || arg == "--c")
        {
            if (i + 1 < args.size())
            {
                cubbiPath = args[++i];
                config.hasCubbi = true;
            }
            else
            {
                logger.logLine("Error: --cubbi (--c) requires a folder path");
                return false;
            }
        }
        else if (arg == "--jammi" || arg == "--j")
        {
            if (i + 1 < args.size())
            {
                jammiPath = args[++i];
                config.hasJammi = true;
            }
            else
            {
                logger.logLine("Error: --jammi (--j) requires a folder path");
                return false;
            }
        }
        else if (arg == "--output" || arg == "--o")
        {
            if (i + 1 < args.size())
            {
                outputPath = args[++i];
            }
            else
            {
                logger.logLine("Error: --output (--o) requires a folder path");
                return false;
            }
        }
        else if (arg == "--convert")
        {
            hasConvertFlag = true;
        }
        else if (!arg.startsWith("-"))
        {
            // This is a legacy folder path
            legacyFolderPath = arg;
        }
        else
        {
            logger.logLine("Warning: Unknown option: " + arg);
        }
    }

    // Determine operation mode
    if (config.hasCubbi || config.hasJammi)
    {
        // CHOMPI mode
        config.mode = OperationMode::Chompi;
        logger.logLine("Mode: CHOMPI Sample Processing");
        logger.logLine("");

        // Set output folder (use specified or default)
        if (!outputPath.isEmpty())
        {
            config.outputFolder = juce::File(outputPath);
        }
        else
        {
            config.outputFolder = juce::File::getCurrentWorkingDirectory().getChildFile("converted");
        }

        logger.logLine("Output folder: " + config.outputFolder.getFullPathName());
        logger.logLine("");

        // Validate cubbi folder if specified
        if (config.hasCubbi)
        {
            config.cubbiFolder = juce::File(cubbiPath);
            if (!config.cubbiFolder.exists())
            {
                logger.logLine("Error: Cubbi folder does not exist: " + cubbiPath);
                return false;
            }
            if (!config.cubbiFolder.isDirectory())
            {
                logger.logLine("Error: Cubbi path is not a directory: " + cubbiPath);
                return false;
            }
            logger.logLine("Cubbi folder: " + config.cubbiFolder.getFullPathName());
        }

        // Validate jammi folder if specified
        if (config.hasJammi)
        {
            config.jammiFolder = juce::File(jammiPath);
            if (!config.jammiFolder.exists())
            {
                logger.logLine("Error: Jammi folder does not exist: " + jammiPath);
                return false;
            }
            if (!config.jammiFolder.isDirectory())
            {
                logger.logLine("Error: Jammi path is not a directory: " + jammiPath);
                return false;
            }
            logger.logLine("Jammi folder: " + config.jammiFolder.getFullPathName());
        }

        logger.logLine("");
        return true;
    }
    else if (!legacyFolderPath.isEmpty())
    {
        // Legacy mode (scan or convert)
        config.mode = hasConvertFlag ? OperationMode::Convert : OperationMode::Scan;

        if (hasConvertFlag)
        {
            logger.logLine("Mode: Legacy Conversion (16-bit 48kHz)");
            logger.logLine("");
        }
        else
        {
            logger.logLine("Mode: Scan Only");
            logger.logLine("");
        }

        logger.logLine("Target folder: " + legacyFolderPath);
        logger.logLine("");

        // Verify folder exists
        config.targetFolder = juce::File(legacyFolderPath);
        if (!config.targetFolder.exists())
        {
            logger.logLine("Error: Folder does not exist: " + legacyFolderPath);
            return false;
        }

        if (!config.targetFolder.isDirectory())
        {
            logger.logLine("Error: Path is not a directory: " + legacyFolderPath);
            return false;
        }

        logger.logLine("Folder validated successfully!");
        logger.logLine("");

        // Scan for WAV files recursively
        logger.logLine("Scanning for WAV files...");
        logger.logLine("");

        config.targetFolder.findChildFiles(config.wavFiles,
                                    juce::File::findFiles,
                                    true,  // search recursively
                                    "*.wav");

        // Check if any files were found
        if (config.wavFiles.isEmpty())
        {
            logger.logLine("No WAV files found in the specified directory.");
            return false;
        }

        logger.logLine("Found " + juce::String(config.wavFiles.size()) + " WAV file(s):");
        logger.logLine("");

        return true;
    }
    else
    {
        // No valid mode specified
        logger.logLine("Error: No valid operation mode specified");
        logger.logLine("");
        displayUsage();
        return false;
    }
}

void CliProcessor::processAudioFiles(const juce::Array<juce::File>& wavFiles,
                                     const juce::File& targetFolder)
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
