#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Logger.h"
#include "ChompiNamer.h"
#include "AudioConverter.h"
#include "../AudioConfiguration.h"
#include "BankFolderParser.h"

//==============================================================================
// ChompiProcessor - High-level CHOMPI sample processing
//==============================================================================
// Orchestrates the complete CHOMPI processing workflow:
// 1. Scan folder for samples
// 2. Generate CHOMPI file mappings
// 3. Convert files with CHOMPI naming
// 4. Report statistics
//==============================================================================

class ChompiProcessor
{
public:
    // Result of processing a category
    struct ProcessingResult
    {
        int filesProcessed = 0;
        int filesConverted = 0;
        int filesSkipped = 0;
        int errors = 0;
        int optimizedGenerated = 0;
        bool success = true;
    };

    ChompiProcessor(Logger& logger);

    // Process complete CHOMPI workflow (cubbi and/or jammi)
    // Returns false if critical error occurred
    bool processSamples(const AudioConfiguration& config,
                       juce::AudioFormatManager& formatManager);

    // Process a single category (cubbi or jammi) from a source folder
    ProcessingResult processCategory(const juce::File& sourceFolder,
                                     const juce::File& outputFolder,
                                     ChompiNamer::Category category,
                                     juce::AudioFormatManager& formatManager,
                                     AudioConverter& converter);

    // Process a single category from pre-built bank assignments (advanced mode)
    ProcessingResult processCategoryFromAssignments(
        const juce::Array<BankFolderParser::BankAssignment>& assignments,
        const juce::File& outputFolder,
        ChompiNamer::Category category,
        juce::AudioFormatManager& formatManager,
        AudioConverter& converter);

private:
    Logger& logger;

    // Shared conversion loop used by both processCategory and processCategoryFromAssignments
    ProcessingResult runConversions(
        const juce::Array<BankFolderParser::BankAssignment>& assignments,
        const juce::File& outputFolder,
        ChompiNamer::Category category,
        juce::AudioFormatManager& formatManager,
        AudioConverter& converter);
};
