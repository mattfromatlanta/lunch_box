// SPDX-License-Identifier: AGPL-3.0-or-later
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
// - Mode detection (Scan/Convert/CHOMPI)
// - Legacy mode processing
// - Delegates to appropriate processing modules
//==============================================================================

class CliProcessor
{
public:
    CliProcessor();

    // Main entry point for CLI mode
    // Returns exit code (0 = success, 1 = error)
    int run(const juce::StringArray& args);

private:
    Logger logger;
    juce::AudioFormatManager formatManager;
    AudioConfiguration config;

    // Initialize application and parse arguments
    // Returns false if program should exit early
    bool initializeApplication(const juce::StringArray& args);

    // Install shell wrapper to /usr/local/bin/lunch_box
    // Returns exit code (0 = success, 1 = error)
    int installCliTool();

    // Display usage information
    void displayUsage();
};
