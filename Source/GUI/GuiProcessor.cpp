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

    if (cubbiFolder == juce::File{} && jammiFolder == juce::File{})
    {
        result.message = "Please select at least one folder (Cubbi or Jammi)";
        return result;
    }

    if (!FileSystemHelper::ensureDirectoryExists(outputFolder, logger))
    {
        result.message = "Could not create output directory: " + outputFolder.getFullPathName();
        return result;
    }

    ChompiProcessor processor(logger);
    AudioConverter converter(logger);
    bool overallSuccess = true;

    if (cubbiFolder != juce::File{})
    {
        auto r = processor.processCategory(cubbiFolder, outputFolder,
                                           ChompiNamer::Category::Cubbi,
                                           formatManager, converter);
        result.cubbiFilesProcessed = r.filesConverted;
        result.cubbiOptimized      = r.optimizedGenerated;
        if (!r.success) overallSuccess = false;
    }

    if (jammiFolder != juce::File{})
    {
        auto r = processor.processCategory(jammiFolder, outputFolder,
                                           ChompiNamer::Category::Jammi,
                                           formatManager, converter);
        result.jammiFilesProcessed = r.filesConverted;
        result.jammiOptimized      = r.optimizedGenerated;
        if (!r.success) overallSuccess = false;
    }

    logger.logLine("CHOMPI processing complete!");

    result.success = overallSuccess;
    result.message = overallSuccess ? "Processing completed successfully"
                                    : "Processing failed. Check logs for details.";
    return result;
}
