// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Logger.h"
#include "../FileSystemHelper.h"

//==============================================================================
// AudioConverter - Handles audio file format conversion
//==============================================================================

class AudioConverter
{
public:
    // Result of a conversion operation
    struct ConversionResult
    {
        bool success = false;  // Conversion completed successfully
        bool skipped = false;  // File was skipped (e.g., too many channels)
        juce::String message;  // Status or error message
    };

    explicit AudioConverter(Logger& loggerToUse);

    // Convert a single audio file with custom output filename
    ConversionResult convertFileWithName(const juce::File& sourceFile,
                                         const juce::File& outputFolder,
                                         const juce::String& outputFileName,
                                         juce::AudioFormatManager& formatManager);

private:
    Logger& logger;

    // Target format constants
    static constexpr int TARGET_BIT_DEPTH = 16;
    static constexpr double TARGET_SAMPLE_RATE = 48000.0;
    static constexpr int MAX_CHANNELS = 2;
    static constexpr double MAX_DURATION_SECONDS = 120.0;  // 2-minute limit per CHOMPI spec

    // Helper methods
    bool needsConversion(const juce::AudioFormatReader* reader) const;

    ConversionResult performConversion(const juce::File& outputFile,
                                       juce::AudioFormatReader* reader);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioConverter)
};
