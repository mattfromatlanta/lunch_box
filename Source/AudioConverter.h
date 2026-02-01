#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Logger.h"
#include "FileSystemHelper.h"

//==============================================================================
// AudioConverter - Handles audio file format conversion
//==============================================================================

class AudioConverter
{
public:
    // Result of a conversion operation
    struct ConversionResult
    {
        bool success;       // Conversion completed successfully
        bool skipped;       // File was skipped (e.g., too many channels)
        juce::String message;  // Status or error message
    };

    AudioConverter(Logger& logger);

    // Convert a single audio file to target format
    ConversionResult convertFile(const juce::File& sourceFile,
                                 const juce::File& outputFolder,
                                 juce::AudioFormatManager& formatManager);

    // Convert a single audio file with custom output filename
    ConversionResult convertFileWithName(const juce::File& sourceFile,
                                         const juce::File& outputFolder,
                                         const juce::String& outputFileName,
                                         juce::AudioFormatManager& formatManager);

    // Batch convert multiple files
    void convertFiles(const juce::Array<juce::File>& files,
                     const juce::File& sourceFolder,
                     const juce::File& outputFolder,
                     juce::AudioFormatManager& formatManager);

private:
    Logger& logger;

    // Target format constants
    static constexpr int TARGET_BIT_DEPTH = 16;
    static constexpr double TARGET_SAMPLE_RATE = 48000.0;
    static constexpr int MAX_CHANNELS = 2;

    // Helper methods
    bool needsConversion(const juce::AudioFormatReader* reader) const;

    ConversionResult performConversion(const juce::File& sourceFile,
                                       const juce::File& outputFile,
                                       juce::AudioFormatReader* reader,
                                       juce::AudioFormatManager& formatManager);
};
