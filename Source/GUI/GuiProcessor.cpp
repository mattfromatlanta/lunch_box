#include "GuiProcessor.h"
#include "../FileSystemHelper.h"

GuiProcessor::GuiProcessor()
{
    formatManager.registerBasicFormats();
}

void GuiProcessor::setLogCallback(std::function<void(const juce::String&)> callback)
{
    logger.onLog = std::move(callback);
}

GuiProcessor::ProcessingResult GuiProcessor::processFilesFromAssignments(
    const juce::Array<BankFolderParser::BankAssignment>& cubbiAssignments,
    const juce::Array<BankFolderParser::BankAssignment>& jammiAssignments,
    const juce::File& outputFolder)
{
    ProcessingResult result;

    if (cubbiAssignments.isEmpty() && jammiAssignments.isEmpty())
    {
        result.success = false;
        result.message = "No samples assigned. Add samples to at least one slot.";
        return result;
    }

    if (!FileSystemHelper::ensureDirectoryExists(outputFolder, logger))
    {
        result.success = false;
        result.message = "Could not create output directory: " + outputFolder.getFullPathName();
        return result;
    }

    ChompiProcessor processor(logger);
    AudioConverter converter(logger);
    bool overallSuccess = true;

    if (!cubbiAssignments.isEmpty())
    {
        auto r = processor.processCategoryFromAssignments(
            cubbiAssignments, outputFolder,
            ChompiNamer::Category::Cubbi, formatManager, converter);

        result.cubbiFilesProcessed = r.filesConverted;
        result.cubbiOptimized      = r.optimizedGenerated;
        if (!r.success) overallSuccess = false;
    }

    if (!jammiAssignments.isEmpty())
    {
        auto r = processor.processCategoryFromAssignments(
            jammiAssignments, outputFolder,
            ChompiNamer::Category::Jammi, formatManager, converter);

        result.jammiFilesProcessed = r.filesConverted;
        result.jammiOptimized      = r.optimizedGenerated;
        if (!r.success) overallSuccess = false;
    }

    result.success = overallSuccess;
    result.message = overallSuccess ? "Processing completed successfully"
                                    : "Processing failed. Check logs for details.";
    return result;
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
        // Count base files only (exclude _double versions)
        if (config.hasCubbi)
        {
            juce::Array<juce::File> cubbiFiles;
            outputFolder.findChildFiles(cubbiFiles, juce::File::findFiles, false, "cubbi_*.wav");
            for (const auto& f : cubbiFiles)
            {
                if (f.getFileName().contains("_double"))
                    result.cubbiOptimized++;
                else
                    result.cubbiFilesProcessed++;
            }
        }

        if (config.hasJammi)
        {
            juce::Array<juce::File> jammiFiles;
            outputFolder.findChildFiles(jammiFiles, juce::File::findFiles, false, "jammi_*.wav");
            for (const auto& f : jammiFiles)
            {
                if (f.getFileName().contains("_double"))
                    result.jammiOptimized++;
                else
                    result.jammiFilesProcessed++;
            }
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
