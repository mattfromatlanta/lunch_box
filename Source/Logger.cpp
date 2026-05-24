// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Logger.h"

Logger::Logger() : Logger(true) {}

Logger::Logger(bool createLogFile)
{
    if (!createLogFile) return;  // callback-only mode — no file

    // Generate timestamp for log filename
    juce::Time now = juce::Time::getCurrentTime();
    juce::String timestamp = now.formatted("%Y%m%d_%H%M%S");
    juce::String logFileName = "chompi_pack_log_" + timestamp + ".txt";

    // Create logs directory if it doesn't exist
    juce::File logsDir = juce::File::getCurrentWorkingDirectory().getChildFile("logs");
    if (!logsDir.exists())
        logsDir.createDirectory();

    // Create log file in logs directory
    logFile = logsDir.getChildFile(logFileName);

    // Create the file and open output stream
    logFile.create();
    logStream = std::make_unique<juce::FileOutputStream>(logFile);

    if (logStream->openedOk())
    {
        std::cout << "Log file created: " << logFile.getFullPathName() << std::endl;
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Warning: Could not create log file" << std::endl;
    }
}

Logger::~Logger()
{
    if (logStream)
        logStream->flush();
}

void Logger::log(const juce::String& message)
{
    // Write to console
    std::cout << message;

    // Write to log file
    if (logStream && logStream->openedOk())
    {
        logStream->writeText(message, false, false, nullptr);
        logStream->flush();
    }

    // Forward to GUI callback if set
    if (onLog)
        onLog(message);
}

void Logger::logLine(const juce::String& message)
{
    log(message + "\n");
}
