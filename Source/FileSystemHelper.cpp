// SPDX-License-Identifier: AGPL-3.0-or-later
#include "FileSystemHelper.h"

namespace FileSystemHelper
{
    bool ensureDirectoryExists(const juce::File& directory, Logger& logger)
    {
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

    bool isDirectoryWritable(const juce::File& directory)
    {
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

    juce::String getAudioFormatName(const juce::File& file)
    {
        auto ext = file.getFileExtension().toLowerCase();
        if (ext == ".wav")               return "WAV";
        if (ext == ".aiff" || ext == ".aif") return "AIFF";
        if (ext == ".mp3")               return "MP3";
        if (ext == ".flac")              return "FLAC";
        return "Unknown";
    }

    juce::StringArray getSupportedAudioExtensions()
    {
        return {"*.wav", "*.aiff", "*.aif", "*.mp3", "*.flac"};
    }
}
