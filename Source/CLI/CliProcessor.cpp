// SPDX-License-Identifier: AGPL-3.0-or-later
#include "CliProcessor.h"
#include "../Processing/LunchBoxProcessor.h"

CliProcessor::CliProcessor()
{
}

int CliProcessor::run(const juce::StringArray& args)
{
    if (args.contains("--install"))
        return installCliTool();

    if (!initializeApplication(args))
        return 1;

    LunchBoxProcessor processor(logger);

    if (!processor.processSamples(config, formatManager))
        return 1;

    logger.logLine("");
    logger.logLine("All operations complete!");

    return 0;
}

int CliProcessor::installCliTool()
{
    juce::File binary = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    juce::File installTarget("/usr/local/bin/lunch_box");

    juce::String script = "#!/bin/bash\nexec \"" + binary.getFullPathName() + "\" \"$@\"\n";

    // Force LF line endings: replaceWithText() defaults to CRLF, which would put
    // a stray \r in the shebang ("bad interpreter: /bin/bash^M").
    if (!installTarget.replaceWithText(script, false, false, "\n"))
    {
        logger.logLine("Error: Could not write to " + installTarget.getFullPathName());
        logger.logLine("Try: sudo \"" + binary.getFullPathName() + "\" --install");
        return 1;
    }

    juce::ChildProcess chmod;
    chmod.start("chmod +x " + installTarget.getFullPathName());
    chmod.waitForProcessToFinish(5000);

    logger.logLine("Installed: " + installTarget.getFullPathName());
    logger.logLine("You can now run 'lunch_box' from anywhere.");
    return 0;
}

void CliProcessor::displayUsage()
{
    logger.logLine("Usage: lunch_box [OPTIONS]");
    logger.logLine("");
    logger.logLine("Options:");
    logger.logLine("  --cubbi, --c <path>    Process cubbi samples (percussive/loop/SFX)");
    logger.logLine("  --jammi, --j <path>    Process jammi samples (tuned/chromatic)");
    logger.logLine("  --output, --o <path>   Output directory (default: ./converted/)");
    logger.logLine("  --no-normalize         Skip -6 dB peak normalization (on by default)");
    logger.logLine("  --install              Install 'lunch_box' command to /usr/local/bin");
    logger.logLine("  --help, -h             Show this help message");
    logger.logLine("");
    logger.logLine("Examples:");
    logger.logLine("  lunch_box --cubbi /samples/cubbi --jammi /samples/jammi");
    logger.logLine("  lunch_box --c /samples/cubbi --o /my/output");
    logger.logLine("  lunch_box --j /samples/jammi");
}

bool CliProcessor::initializeApplication(const juce::StringArray& args)
{
    logger.logLine("==================================");
    logger.logLine("Lunch Box - CHOMPI Sample Processor");
    logger.logLine("==================================");
    logger.logLine("");

    formatManager.registerBasicFormats();

    if (args.isEmpty() || args.contains("--help") || args.contains("-h"))
    {
        displayUsage();
        return false;
    }

    juce::String cubbiPath;
    juce::String jammiPath;
    juce::String outputPath;

    for (int i = 0; i < args.size(); ++i)
    {
        juce::String arg = args[i];

        if (arg == "--cubbi" || arg == "--c")
        {
            if (i + 1 < args.size())
            {
                cubbiPath = args[++i];
                config.hasCubbi = true;
            }
            else
            {
                logger.logLine("Error: --cubbi requires a folder path");
                return false;
            }
        }
        else if (arg == "--jammi" || arg == "--j")
        {
            if (i + 1 < args.size())
            {
                jammiPath = args[++i];
                config.hasJammi = true;
            }
            else
            {
                logger.logLine("Error: --jammi requires a folder path");
                return false;
            }
        }
        else if (arg == "--no-normalize")
        {
            config.normalize = false;
        }
        else if (arg == "--output" || arg == "--o")
        {
            if (i + 1 < args.size())
            {
                outputPath = args[++i];
            }
            else
            {
                logger.logLine("Error: --output requires a folder path");
                return false;
            }
        }
        else
        {
            logger.logLine("Warning: Unknown option: " + arg);
        }
    }

    if (!config.hasCubbi && !config.hasJammi)
    {
        logger.logLine("Error: At least one of --cubbi or --jammi must be specified");
        logger.logLine("");
        displayUsage();
        return false;
    }

    config.outputFolder = outputPath.isEmpty()
        ? juce::File::getCurrentWorkingDirectory().getChildFile("converted")
        : juce::File(outputPath);

    logger.logLine("Output folder: " + config.outputFolder.getFullPathName());
    logger.logLine("");

    if (config.hasCubbi)
    {
        config.cubbiFolder = juce::File(cubbiPath);
        if (!config.cubbiFolder.isDirectory())
        {
            logger.logLine("Error: Cubbi path is not a valid directory: " + cubbiPath);
            return false;
        }
        logger.logLine("Cubbi folder: " + config.cubbiFolder.getFullPathName());
    }

    if (config.hasJammi)
    {
        config.jammiFolder = juce::File(jammiPath);
        if (!config.jammiFolder.isDirectory())
        {
            logger.logLine("Error: Jammi path is not a valid directory: " + jammiPath);
            return false;
        }
        logger.logLine("Jammi folder: " + config.jammiFolder.getFullPathName());
    }

    logger.logLine("");
    return true;
}
