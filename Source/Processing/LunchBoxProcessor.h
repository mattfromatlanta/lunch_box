// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include "../Logger.h"
#include "LunchBoxNamer.h"
#include "AudioConverter.h"
#include "../AudioConfiguration.h"
#include "BankFolderParser.h"

//==============================================================================
// LunchBoxProcessor - High-level CHOMPI sample processing
//==============================================================================
// Orchestrates the complete CHOMPI processing workflow:
// 1. Scan folder for samples
// 2. Generate CHOMPI file mappings
// 3. Convert files with CHOMPI naming
// 4. Report statistics
//==============================================================================

class LunchBoxProcessor
{
public:
    // Result of processing a category
    struct ProcessingResult
    {
        int filesProcessed = 0;
        int filesConverted = 0;
        int filesSkipped = 0;
        int errors = 0;
        bool success = true;
        bool cancelled = false;  // conversion loop stopped early at the caller's request
    };

    // Called once after each file is processed (for progress reporting); and a
    // predicate polled before each file so a background export can be cancelled.
    using ProgressCallback = std::function<void()>;
    using CancelCallback   = std::function<bool()>;

    explicit LunchBoxProcessor(Logger& loggerToUse);

    // Process complete CHOMPI workflow (cubbi and/or jammi)
    // Returns false if critical error occurred
    bool processSamples(const AudioConfiguration& config,
                       juce::AudioFormatManager& formatManager);

    // Process a single category (cubbi or jammi) from a source folder
    ProcessingResult processCategory(const juce::File& sourceFolder,
                                     const juce::File& outputFolder,
                                     LunchBoxNamer::Category category,
                                     juce::AudioFormatManager& formatManager,
                                     AudioConverter& converter);

    // Process a single category from pre-built bank assignments (advanced mode).
    // onFileProcessed/shouldCancel are optional (GUI background export); CLI omits them.
    ProcessingResult processCategoryFromAssignments(
        const juce::Array<BankFolderParser::BankAssignment>& assignments,
        const juce::File& outputFolder,
        LunchBoxNamer::Category category,
        juce::AudioFormatManager& formatManager,
        AudioConverter& converter,
        ProgressCallback onFileProcessed = {},
        CancelCallback shouldCancel = {});

private:
    Logger& logger;

    // Shared conversion loop used by both processCategory and processCategoryFromAssignments
    ProcessingResult runConversions(
        const juce::Array<BankFolderParser::BankAssignment>& assignments,
        const juce::File& outputFolder,
        LunchBoxNamer::Category category,
        juce::AudioFormatManager& formatManager,
        AudioConverter& converter,
        ProgressCallback onFileProcessed = {},
        CancelCallback shouldCancel = {});

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LunchBoxProcessor)
};
