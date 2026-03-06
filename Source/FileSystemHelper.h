#pragma once

#include <juce_core/juce_core.h>
#include "Logger.h"

//==============================================================================
// FileSystemHelper - Utility functions for file system operations
//==============================================================================

namespace FileSystemHelper
{
    // Ensure output directory exists, create if necessary
    // Returns true if directory exists or was created successfully
    bool ensureDirectoryExists(const juce::File& directory, Logger& logger);

    // Generate output file path for a converted file
    // Preserves filename and relative structure from source folder
    juce::File generateOutputPath(const juce::File& sourceFile,
                                   const juce::File& sourceFolder,
                                   const juce::File& outputFolder);

    // Validate that directory is writable
    bool isDirectoryWritable(const juce::File& directory);

    // Get default output directory (converted/ in working directory)
    juce::File getDefaultOutputDirectory();

    // Get human-readable format name from file extension (e.g., "MP3", "FLAC", "WAV")
    juce::String getAudioFormatName(const juce::File& file);

    // Get all supported audio file extensions as glob patterns
    juce::StringArray getSupportedAudioExtensions();
}
