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
            "Please select a folder for your pack.", startDir, "", true);

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
                    appendStatus("\n=== Starting CHOMPI Processing ===");
                    if (viewMode == ViewMode::Bank)
                        syncBankFocusToPack();
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
                    packName + " is not empty.",
                    "The export folder already exists:\n" + packFolder.getFullPathName() + "\n\n"
                    + countStr + " will be deleted before exporting.\n\nContinue?",
                    nullptr,
                    juce::ModalCallbackFunction::create([this, children, runExport](int res2) mutable
                    {
                        if (res2 == 0) { processButton.setEnabled(true); return; }
                        for (auto& child : children) child.deleteRecursively();
                        runExport();
                    }));
            };

            if (outsideHome)
            {
                juce::NativeMessageBox::showOkCancelBox(
                    juce::MessageBoxIconType::WarningIcon,
                    "Folder Outside Home Directory",
                    "\"" + f.getFullPathName() + "\" is outside your home directory.\n\n"
                    "A \"" + packName + "\" folder will be created there. Are you sure?",
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

    juce::File outputFolder = getResolvedOutputFolder();

    auto result = processor->processFilesFromAssignments(
        cubbiAssignments, jammiAssignments, outputFolder);

    if (result.success)
    {
        appendProcessingResult(result, outputFolder);
        processButton.triggerSuccessAnimation();
    }
    else
    {
        appendStatus("\n=== Processing Failed ===");
        appendStatus("Error: " + result.message);
    }
}

void MainComponent::appendProcessingResult(const GuiProcessor::ProcessingResult& result,
                                            const juce::File& outputFolder)
{
    int totalProcessed = result.cubbiFilesProcessed + result.jammiFilesProcessed;
    int totalOptimized = result.cubbiOptimized + result.jammiOptimized;
    appendStatus("\n=== Processing Complete ===");
    if (result.cubbiFilesProcessed > 0)
        appendStatus("  Cubbi: " + juce::String(result.cubbiFilesProcessed) + " samples");
    if (result.jammiFilesProcessed > 0)
        appendStatus("  Jammi: " + juce::String(result.jammiFilesProcessed) + " samples");
    appendStatus("  Total: " + juce::String(totalProcessed) + " samples processed");
    appendStatus("  Doubles: " + juce::String(totalOptimized) + " optimized versions created");
    appendStatus("  Output: " + outputFolder.getFullPathName());
}

void MainComponent::updateProcessButtonState()
{
    bool ready = false;
    if (viewMode == ViewMode::Pack)
        ready = (cubbiEditor->getFilledCount() + jammiEditor->getFilledCount()) > 0;
    else  // Bank
        ready = (bankFocusPanel->getFilledCount(LunchBoxNamer::Category::Cubbi)
               + bankFocusPanel->getFilledCount(LunchBoxNamer::Category::Jammi)) > 0;

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
