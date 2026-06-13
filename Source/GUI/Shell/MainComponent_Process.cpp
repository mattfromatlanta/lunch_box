// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Sample-processing pipeline: collects assignments, drives GuiProcessor,
// renders status output to the console.

#include "MainComponent.h"
#include "ConsoleWindow.h"
#include "LabelStrings.h"

void MainComponent::processFiles()
{
    // Nothing to export → show a brief notice instead of disabling the button.
    if (viewMode == ViewMode::Bank)
        bankFocusPanel->commitActiveBankToModel();

    if (packModel.getFilledCount(LunchBoxNamer::Category::Cubbi)
      + packModel.getFilledCount(LunchBoxNamer::Category::Jammi) == 0)
    {
        messageOverlay.show(LunchBoxLabels::kMsgNoSamples);
        return;
    }

    packNameOverlay.onResult = [this](bool confirmed, juce::String packName)
    {
        if (!confirmed || packName.isEmpty())
            return;

        lastPackName = packName;
        saveString("lastPackName", lastPackName);

        auto startDir = outputBaseFolder.exists() ? outputBaseFolder
                                                  : juce::File::getSpecialLocation(juce::File::userHomeDirectory);

        fileChooser = std::make_unique<juce::FileChooser>(
            LunchBoxLabels::kChooseOutputFolder, startDir, "", true);

        auto flags = juce::FileBrowserComponent::openMode
                   | juce::FileBrowserComponent::canSelectDirectories;

        fileChooser->launchAsync(flags, [this, packName](const juce::FileChooser& chooser)
        {
            auto f = chooser.getResult();
            if (!f.isDirectory())
                return;

            auto homeDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
            bool outsideHome = (f != homeDir && !f.isAChildOf(homeDir));

            auto continueWithFolder = [this, f, packName]()
            {
                outputBaseFolder = f;
                saveFolder("lastOutputParent", outputBaseFolder);

                auto packFolder = f.getChildFile(packName);

                auto runExport = [this]()
                {
                    appendStatus(LunchBoxLabels::kStatusProcessStart);
                    if (viewMode == ViewMode::Bank)
                        bankFocusPanel->commitActiveBankToModel();
                    // Launches the background export; processButton is re-enabled
                    // in onExportFinished() when the thread completes.
                    processFilesFromEditors();
                };

                if (!packFolder.exists())
                {
                    runExport();
                    return;
                }

                auto children = packFolder.findChildFiles(juce::File::findFilesAndDirectories, false, "[!.]*");
                if (children.isEmpty())
                {
                    runExport();
                    return;
                }

                int fileCount = 0, dirCount = 0;
                for (auto& c : children)
                    (c.isDirectory() ? dirCount : fileCount)++;

                juce::String countStr;
                if (fileCount > 0)
                    countStr += juce::String(fileCount) + (fileCount == 1 ? " file" : " files");
                if (fileCount > 0 && dirCount > 0)
                    countStr += " and ";
                if (dirCount > 0)
                    countStr += juce::String(dirCount) + (dirCount == 1 ? " folder" : " folders");

                juce::NativeMessageBox::showOkCancelBox(
                    juce::MessageBoxIconType::WarningIcon,
                    packName + LunchBoxLabels::kDlgFolderNotEmpty,
                    LunchBoxLabels::kDlgFolderNotEmptyBody + packFolder.getFullPathName() + "\n\n"
                    + countStr + LunchBoxLabels::kDlgFolderNotEmptyWillDel,
                    nullptr,
                    juce::ModalCallbackFunction::create([runExport](int res2) mutable
                    {
                        if (res2 == 0) return;
                        runExport();
                    }));
            };

            if (outsideHome)
            {
                juce::NativeMessageBox::showOkCancelBox(
                    juce::MessageBoxIconType::WarningIcon,
                    LunchBoxLabels::kDlgOutsideHomeTitle,
                    "\"" + f.getFullPathName() + LunchBoxLabels::kDlgOutsideHomeBody
                    + "A \"" + packName + LunchBoxLabels::kDlgOutsideHomeSuffix,
                    nullptr,
                    juce::ModalCallbackFunction::create([continueWithFolder](int res2) mutable
                    {
                        if (res2 == 0) return;
                        continueWithFolder();
                    }));
                return;
            }

            continueWithFolder();
        });
    };

    packNameOverlay.show(lastPackName);
}

void MainComponent::processFilesFromEditors()
{
    auto cubbiAssignments = cubbiEditor->getAssignments();
    auto jammiAssignments = jammiEditor->getAssignments();

    // Stage the export in a hidden sibling folder and only swap it into place
    // once every file is written, so an interrupted or failed export can never
    // destroy an existing pack. Any previous pack goes to the Trash (recoverable),
    // not straight to deletion.
    const juce::File outputFolder  = getResolvedOutputFolder();
    const juce::File stagingFolder = outputFolder
        .getSiblingFile("." + outputFolder.getFileName() + ".exporting")
        .getNonexistentSibling(false);

    // Run the conversion off the message thread so the UI stays responsive. The
    // Pack button spins while it runs (no progress dialog); Esc cancels. The
    // button stays disabled until onExportFinished() re-enables it.
    exportThread = std::make_unique<ExportThread>(
        *processor, cubbiAssignments, jammiAssignments, stagingFolder, outputFolder);

    juce::Component::SafePointer<MainComponent> safeThis(this);
    exportThread->onFinished = [safeThis](const ExportThread::Outcome& outcome)
    {
        if (safeThis != nullptr)
            safeThis->onExportFinished(outcome);
    };

    // Deactivate the button now and keep it disabled until the spin animation
    // has fully come to rest (PackButton::onAnimationStopped re-enables it).
    processButton.setEnabled(false);
    processButton.startSpin();
    exportThread->startThread();
}

void MainComponent::onExportFinished(const ExportThread::Outcome& outcome)
{
    processButton.stopSpin();

    if (outcome.cancelled)
    {
        appendStatus(LunchBoxLabels::kStatusProcessCancelled);
    }
    else if (!outcome.success)
    {
        appendStatus(LunchBoxLabels::kStatusProcessFailed);
        appendStatus(juce::String(LunchBoxLabels::kStatusErrorPrefix) + outcome.errorMessage);
    }
    else
    {
        appendProcessingResult(outcome.result, outcome.outputFolder);
    }

    // Tear down the finished thread, deferred so we're clear of its run() frame.
    juce::Component::SafePointer<MainComponent> safeThis(this);
    juce::MessageManager::callAsync([safeThis]
    {
        if (safeThis != nullptr)
            safeThis->exportThread.reset();
    });
}

void MainComponent::appendProcessingResult(const GuiProcessor::ProcessingResult& result,
                                            const juce::File& outputFolder)
{
    int totalProcessed = result.cubbiFilesProcessed + result.jammiFilesProcessed;
    appendStatus(LunchBoxLabels::kStatusProcessComplete);
    if (result.cubbiFilesProcessed > 0)
        appendStatus(LunchBoxLabels::kStatusCubbiLabel + juce::String(result.cubbiFilesProcessed) + LunchBoxLabels::kStatusSamplesUnit);
    if (result.jammiFilesProcessed > 0)
        appendStatus(LunchBoxLabels::kStatusJammiLabel + juce::String(result.jammiFilesProcessed) + LunchBoxLabels::kStatusSamplesUnit);
    appendStatus(LunchBoxLabels::kStatusTotalLabel + juce::String(totalProcessed) + LunchBoxLabels::kStatusSamplesUnit + " processed");
    appendStatus(LunchBoxLabels::kStatusOutputLabel + outputFolder.getFullPathName());
}

void MainComponent::appendStatus(const juce::String& message)
{
    consoleContent += message + "\n";
    if (consoleWindow != nullptr)
    {
        consoleWindow->editor.moveCaretToEnd();
        consoleWindow->editor.insertTextAtCaret(message + "\n");
        consoleWindow->editor.moveCaretToEnd();
    }
}

void MainComponent::setShowRuntimeLogs(bool shouldShow)
{
    showRuntimeLogs = shouldShow;

    if (showRuntimeLogs)
    {
        processor->setLogCallback([this](const juce::String& msg)
        {
            juce::MessageManager::callAsync([this, msg]
            {
                appendStatus("[log] " + msg.trimEnd());
            });
        });
    }
    else
    {
        processor->setLogCallback(nullptr);
    }
}
