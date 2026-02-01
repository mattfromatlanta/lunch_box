#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Logger.h"

//==============================================================================
// AudioScanner - Functions for scanning and analyzing audio files
//==============================================================================

// Initialize application, validate arguments, and scan for files
// Returns false if program should exit early
bool initializeApplication(int argc, char *argv[],
                           Logger &logger,
                           juce::AudioFormatManager &formatManager,
                           juce::File &targetFolder,
                           juce::Array<juce::File> &wavFiles);

// Process and analyze all audio files
void processAudioFiles(const juce::Array<juce::File>& wavFiles,
                      const juce::File& targetFolder,
                      juce::AudioFormatManager& formatManager,
                      Logger& logger);
