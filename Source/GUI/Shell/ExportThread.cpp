// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ExportThread.h"

ExportThread::ExportThread(GuiProcessor& proc,
                           juce::Array<BankFolderParser::BankAssignment> cubbi,
                           juce::Array<BankFolderParser::BankAssignment> jammi,
                           juce::File staging,
                           juce::File output)
    : juce::Thread("LunchBox Export"),
      processor(proc),
      cubbiAssignments(std::move(cubbi)),
      jammiAssignments(std::move(jammi)),
      stagingFolder(std::move(staging)),
      outputFolder(std::move(output))
{
}

void ExportThread::run()
{
    Outcome outcome;

    auto result = processor.processFilesFromAssignments(
        cubbiAssignments, jammiAssignments, stagingFolder,
        {},                                          // no per-file progress UI
        [this] { return threadShouldExit(); });      // Esc → signalThreadShouldExit()

    if (threadShouldExit() || result.cancelled)
    {
        // Cancelled mid-conversion — discard the partial staging folder.
        stagingFolder.deleteRecursively();
        outcome.cancelled = true;
    }
    else if (!result.success)
    {
        stagingFolder.deleteRecursively();
        outcome.result       = result;
        outcome.errorMessage = result.message;
    }
    else
    {
        // Swap the finished staging folder into place. Any previous pack goes to
        // the Trash (recoverable) when possible, otherwise it is deleted.
        if (outputFolder.exists() && !outputFolder.moveToTrash())
            outputFolder.deleteRecursively();

        if (!stagingFolder.moveFileTo(outputFolder))
        {
            outcome.errorMessage = "Could not move the finished pack into place. "
                                   "The exported files are in: " + stagingFolder.getFullPathName();
        }
        else
        {
            outcome.success      = true;
            outcome.result       = result;
            outcome.outputFolder = outputFolder;
        }
    }

    // Deliver on the message thread. Copies are captured, so the callback stays
    // valid even if this thread object is destroyed right afterwards.
    auto callback = onFinished;
    juce::MessageManager::callAsync([callback, outcome]
    {
        if (callback) callback(outcome);
    });
}
