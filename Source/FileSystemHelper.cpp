#include "FileSystemHelper.h"

namespace FileSystemHelper
{
    bool ensureDirectoryExists(const juce::File& directory, Logger& logger)
    {
        // Check if directory already exists
        if (directory.exists())
        {
            if (directory.isDirectory())
            {
                return true;
            }
            else
            {
                logger.logLine("Error: Path exists but is not a directory: " + directory.getFullPathName());
                return false;
            }
        }

        // Try to create the directory
        juce::Result result = directory.createDirectory();

        if (result.wasOk())
        {
            logger.logLine("Created output directory: " + directory.getFullPathName());
            return true;
        }
        else
        {
            logger.logLine("Error: Failed to create directory: " + result.getErrorMessage());
            return false;
        }
    }

    juce::File generateOutputPath(const juce::File& sourceFile,
                                   const juce::File& sourceFolder,
                                   const juce::File& outputFolder)
    {
        // Get relative path from source folder
        juce::String relativePath = sourceFile.getRelativePathFrom(sourceFolder);

        // If relative path is empty, just use the filename
        if (relativePath.isEmpty())
        {
            relativePath = sourceFile.getFileName();
        }

        // Combine output folder with relative path to preserve structure
        juce::File outputPath = outputFolder.getChildFile(relativePath);

        // Ensure the output file has .wav extension
        if (!outputPath.hasFileExtension("wav") && !outputPath.hasFileExtension("WAV"))
        {
            outputPath = outputPath.withFileExtension("wav");
        }

        return outputPath;
    }

    bool outputFileExists(const juce::File& outputPath)
    {
        return outputPath.exists();
    }

    bool isDirectoryWritable(const juce::File& directory)
    {
        // Directory must exist
        if (!directory.exists() || !directory.isDirectory())
        {
            return false;
        }

        // Try to create a temporary file to test write permissions
        juce::File testFile = directory.getChildFile(".write_test_" + juce::String(juce::Random::getSystemRandom().nextInt()));

        if (testFile.create())
        {
            testFile.deleteFile();
            return true;
        }

        return false;
    }

    juce::File getDefaultOutputDirectory()
    {
        return juce::File::getCurrentWorkingDirectory().getChildFile("converted");
    }
}
