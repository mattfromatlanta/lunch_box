#pragma once

#include <juce_core/juce_core.h>
#include <iostream>
#include <functional>
#include <memory>

//==============================================================================
// Logger class - writes to both console and log file
//==============================================================================
class Logger
{
public:
    Logger();
    ~Logger();

    // Write message to both console and log file
    void log(const juce::String& message);

    // Convenience method for logging with newline
    void logLine(const juce::String& message);

    // Optional callback invoked for every log message (used by GUI runtime log)
    std::function<void(const juce::String&)> onLog;

private:
    juce::File logFile;
    std::unique_ptr<juce::FileOutputStream> logStream;
};
