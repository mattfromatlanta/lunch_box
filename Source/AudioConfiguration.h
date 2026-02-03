#pragma once

#include <juce_core/juce_core.h>

//==============================================================================
// Shared audio processing configuration structures
//==============================================================================
// These data structures are used across CLI and processing modules
// to pass configuration and state between components.
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
