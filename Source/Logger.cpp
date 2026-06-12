// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Logger.h"

Logger::Logger() : Logger(true) {}

juce::File Logger::getLogDirectory()
{
    return juce::FileLogger::getSystemLogFileFolder().getChildFile("Lunch Box");
}

Logger::Logger(bool createLogFile)
{
    if (!createLogFile) return;  // callback-only mode — no file

    juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");

    juce::File logsDir = getLogDirectory();
    logsDir.createDirectory();

    logFile = logsDir.getChildFile("lunch_box_log_" + timestamp + ".txt");
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
    std::cout << message;

    if (logStream && logStream->openedOk())
    {
        logStream->writeText(message, false, false, nullptr);
        logStream->flush();
    }

    if (onLog)
        onLog(message);
}

void Logger::logLine(const juce::String& message)
{
    log(message + "\n");
}
