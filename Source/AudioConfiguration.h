// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_core/juce_core.h>

//==============================================================================
// Shared audio processing configuration structures
//==============================================================================
// These data structures are used across CLI and processing modules
// to pass configuration and state between components.
//==============================================================================

// Configuration for audio processing
struct AudioConfiguration
{
    juce::File cubbiFolder;
    juce::File jammiFolder;
    juce::File outputFolder;
    bool hasCubbi = false;
    bool hasJammi = false;
};
