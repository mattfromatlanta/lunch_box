#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include "../AudioConfiguration.h"
#include "../Logger.h"
#include "../ChompiProcessor.h"

//==============================================================================
// GuiProcessor - Bridge between GUI and processing logic
//==============================================================================
// This is a thin wrapper around ChompiProcessor for GUI context.
// It builds an AudioConfiguration from GUI selections and delegates
// to the same ChompiProcessor used by CLI mode.
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
        juce::String message;
    };

    // Process files from GUI selections
    // Returns result with file counts and success status
    ProcessingResult processFiles(const juce::File& cubbiFolder,
                                  const juce::File& jammiFolder,
                                  const juce::File& outputFolder);

    // Set a callback to receive all Logger output (for GUI runtime log)
    // Pass nullptr to disable
    void setLogCallback(std::function<void(const juce::String&)> callback);

private:
    Logger logger;
    juce::AudioFormatManager formatManager;
};
