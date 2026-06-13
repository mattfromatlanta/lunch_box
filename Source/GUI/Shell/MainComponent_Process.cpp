// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Sample-processing pipeline: collects assignments, drives GuiProcessor,
// renders status output to the console.

#include "MainComponent.h"
#include "ConsoleWindow.h"
#include "LabelStrings.h"

void MainComponent::processFiles()
{
    processButton.setEnabled(false);

    packNameOverlay.onResult = [this](bool confirmed, juce::String packName)
    {
        if (!confirmed || packName.isEmpty())
        {
            processButton.setEnabled(true);
            return;
        }

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
            {
                processButton.setEnabled(true);
                return;
            }

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
                    processFilesFromEditors();
                    processButton.setEnabled(true);
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
                    juce::ModalCallbackFunction::create([this, runExport](int res2) mutable
                    {
                        if (res2 == 0) { processButton.setEnabled(true); return; }
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
                    juce::ModalCallbackFunction::create([this, continueWithFolder](int res2) mutable
                    {
                        if (res2 == 0) { processButton.setEnabled(true); return; }
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

    auto result = processor->processFilesFromAssignments(
        cubbiAssignments, jammiAssignments, stagingFolder);

    if (!result.success)
    {
        stagingFolder.deleteRecursively();
        appendStatus(LunchBoxLabels::kStatusProcessFailed);
        appendStatus(LunchBoxLabels::kStatusErrorPrefix + result.message);
        return;
    }

    if (outputFolder.exists() && !outputFolder.moveToTrash())
        outputFolder.deleteRecursively();   // e.g. volumes with no Trash

    if (!stagingFolder.moveFileTo(outputFolder))
    {
        appendStatus(LunchBoxLabels::kStatusProcessFailed);
        appendStatus(juce::String(LunchBoxLabels::kStatusErrorPrefix)
                     + "Could not move the finished pack into place. The exported files are in: "
                     + stagingFolder.getFullPathName());
        return;
    }

    appendProcessingResult(result, outputFolder);
    processButton.triggerSuccessAnimation();
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

void MainComponent::updateProcessButtonState()
{
    const bool ready = (packModel.getFilledCount(LunchBoxNamer::Category::Cubbi)
                      + packModel.getFilledCount(LunchBoxNamer::Category::Jammi)) > 0;
    processButton.setEnabled(ready);
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
