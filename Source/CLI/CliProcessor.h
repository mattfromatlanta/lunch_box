#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Logger.h"
#include "../AudioConfiguration.h"

//==============================================================================
// CliProcessor - Command-line interface processor
//==============================================================================
// Handles all CLI-specific functionality:
// - Command-line argument parsing
// - Mode detection (Scan/Convert/Chompi)
// - Legacy mode processing
// - Delegates to appropriate processing modules
//==============================================================================

class CliProcessor
{
public:
    CliProcessor();

    // Main entry point for CLI mode
    // Returns exit code (0 = success, 1 = error)
    int run(int argc, char* argv[]);

private:
    Logger logger;
    juce::AudioFormatManager formatManager;
    AudioConfiguration config;

    // Initialize application and parse arguments
    // Returns false if program should exit early
    bool initializeApplication(int argc, char* argv[]);

    // Process and analyze audio files (legacy scan mode)
    void processAudioFiles(const juce::Array<juce::File>& wavFiles,
                          const juce::File& targetFolder);

    // Display usage information
    void displayUsage();
};
