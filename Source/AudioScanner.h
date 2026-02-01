#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Logger.h"

//==============================================================================
// AudioScanner - Functions for scanning and analyzing audio files
//==============================================================================

// Operation mode
enum class OperationMode
{
    Scan,       // Legacy scan-only mode
    Convert,    // Legacy conversion mode (--convert)
    Chompi      // CHOMPI mode (--cubbi/--jammi)
};

// Configuration for audio processing
struct AudioConfiguration
{
    OperationMode mode = OperationMode::Scan;

    // Legacy mode parameters
    juce::File targetFolder;
    juce::Array<juce::File> wavFiles;

    // CHOMPI mode parameters
    juce::File cubbiFolder;
    juce::File jammiFolder;
    juce::File outputFolder;
    bool hasCubbi = false;
    bool hasJammi = false;
};

// Initialize application, validate arguments, and scan for files
// Returns false if program should exit early
bool initializeApplication(int argc, char *argv[],
                           Logger &logger,
                           juce::AudioFormatManager &formatManager,
                           AudioConfiguration& config);

// Process and analyze all audio files
void processAudioFiles(const juce::Array<juce::File>& wavFiles,
                      const juce::File& targetFolder,
                      juce::AudioFormatManager& formatManager,
                      Logger& logger);
