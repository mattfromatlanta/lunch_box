// SPDX-License-Identifier: AGPL-3.0-or-later
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
    const juce::File& outputFolder,
    ProgressCallback onProgress,
    CancelCallback shouldCancel)
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

    LunchBoxProcessor processor(logger);
    AudioConverter converter(logger, normalize);
    bool overallSuccess = true;

    // Per-file progress across both categories combined.
    const int total = cubbiAssignments.size() + jammiAssignments.size();
    int done = 0;
    auto tick = [&done, total, &onProgress]()
    {
        ++done;
        if (onProgress) onProgress(done, total);
    };

    if (!cubbiAssignments.isEmpty())
    {
        auto r = processor.processCategoryFromAssignments(
            cubbiAssignments, outputFolder,
            LunchBoxNamer::Category::Cubbi, formatManager, converter, tick, shouldCancel);

        result.cubbiFilesProcessed = r.filesConverted;
        if (r.cancelled) { result.cancelled = true; result.success = false; return result; }
        if (!r.success) overallSuccess = false;
    }

    if (!jammiAssignments.isEmpty())
    {
        auto r = processor.processCategoryFromAssignments(
            jammiAssignments, outputFolder,
            LunchBoxNamer::Category::Jammi, formatManager, converter, tick, shouldCancel);

        result.jammiFilesProcessed = r.filesConverted;
        if (r.cancelled) { result.cancelled = true; result.success = false; return result; }
        if (!r.success) overallSuccess = false;
    }

    result.success = overallSuccess;
    result.message = overallSuccess ? "Processing completed successfully"
                                    : "Processing failed. Check logs for details.";
    return result;
}

