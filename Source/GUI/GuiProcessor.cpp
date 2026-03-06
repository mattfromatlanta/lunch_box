#include "GuiProcessor.h"
#include "../FileSystemHelper.h"

GuiProcessor::GuiProcessor()
{
    formatManager.registerBasicFormats();
}

GuiProcessor::ProcessingResult GuiProcessor::processFiles(
    const juce::File& cubbiFolder,
    const juce::File& jammiFolder,
    const juce::File& outputFolder)
{
    ProcessingResult result;

    // Build AudioConfiguration from GUI selections
    AudioConfiguration config;
    config.mode = OperationMode::Chompi;
    config.outputFolder = outputFolder;

    // Set cubbi folder if provided
    if (cubbiFolder != juce::File{})
    {
        config.cubbiFolder = cubbiFolder;
        config.hasCubbi = true;
    }

    // Set jammi folder if provided
    if (jammiFolder != juce::File{})
    {
        config.jammiFolder = jammiFolder;
        config.hasJammi = true;
    }

    // Validate at least one folder selected
    if (!config.hasCubbi && !config.hasJammi)
    {
        result.success = false;
        result.message = "Please select at least one folder (Cubbi or Jammi)";
        return result;
    }

    // Ensure output folder exists
    if (!FileSystemHelper::ensureDirectoryExists(outputFolder, logger))
    {
        result.success = false;
        result.message = "Could not create output directory: " + outputFolder.getFullPathName();
        return result;
    }

    // Use ChompiProcessor (same as CLI mode)
    ChompiProcessor processor(logger);
    bool success = processor.processSamples(config, formatManager);

    if (success)
    {
        // Count processed files
        if (config.hasCubbi)
        {
            juce::Array<juce::File> cubbiFiles;
            outputFolder.findChildFiles(cubbiFiles, juce::File::findFiles, false, "cubbi_*.wav");
            result.cubbiFilesProcessed = cubbiFiles.size();
        }

        if (config.hasJammi)
        {
            juce::Array<juce::File> jammiFiles;
            outputFolder.findChildFiles(jammiFiles, juce::File::findFiles, false, "jammi_*.wav");
            result.jammiFilesProcessed = jammiFiles.size();
        }

        result.success = true;
        result.message = "Processing completed successfully";
    }
    else
    {
        result.success = false;
        result.message = "Processing failed. Check logs for details.";
    }

    return result;
}
