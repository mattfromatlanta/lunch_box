// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include "../AudioConfiguration.h"
#include "../Logger.h"
#include "../Processing/LunchBoxProcessor.h"
#include "../Processing/BankFolderParser.h"

//==============================================================================
// GuiProcessor - Bridge between GUI and processing logic
//==============================================================================
// This is a thin wrapper around LunchBoxProcessor for GUI context.
// It builds an AudioConfiguration from GUI selections and delegates
// to the same LunchBoxProcessor used by CLI mode.
//==============================================================================

class GuiProcessor
{
public:
    GuiProcessor();

    // Result of processing operation
    struct ProcessingResult
    {
        bool success = false;
        int cubbiFilesProcessed = 0;
        int jammiFilesProcessed = 0;
        int cubbiOptimized = 0;
        int jammiOptimized = 0;
        juce::String message;
    };

    // Simple mode: process files from folder selections
    ProcessingResult processFiles(const juce::File& cubbiFolder,
                                  const juce::File& jammiFolder,
                                  const juce::File& outputFolder);

    // Advanced mode: process from pre-built bank assignments
    ProcessingResult processFilesFromAssignments(
        const juce::Array<BankFolderParser::BankAssignment>& cubbiAssignments,
        const juce::Array<BankFolderParser::BankAssignment>& jammiAssignments,
        const juce::File& outputFolder);

    // Set a callback to receive all Logger output (for GUI runtime log)
    // Pass nullptr to disable
    void setLogCallback(std::function<void(const juce::String&)> callback);

private:
    Logger logger;
    juce::AudioFormatManager formatManager;
};
