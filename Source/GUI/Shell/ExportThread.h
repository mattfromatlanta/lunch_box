// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "GuiProcessor.h"
#include "../Processing/BankFolderParser.h"

//==============================================================================
// ExportThread - Runs a pack export off the message thread.
//==============================================================================
// Converts the assigned samples into a hidden staging folder, then atomically
// swaps it into place so an interrupted or cancelled export can never damage an
// existing pack. There is no progress window: the Pack button's spin animation
// signals that an export is running. Cancel by calling signalThreadShouldExit()
// (wired to the Esc key). The result is delivered to onFinished() on the message
// thread when the run ends.
//==============================================================================

class ExportThread : public juce::Thread
{
public:
    struct Outcome
    {
        bool success   = false;
        bool cancelled = false;
        GuiProcessor::ProcessingResult result;
        juce::File   outputFolder;
        juce::String errorMessage;
    };

    ExportThread(GuiProcessor& processor,
                 juce::Array<BankFolderParser::BankAssignment> cubbiAssignments,
                 juce::Array<BankFolderParser::BankAssignment> jammiAssignments,
                 juce::File stagingFolder,
                 juce::File outputFolder);

    // Invoked on the message thread when the export finishes, fails, or is
    // cancelled. Receives a copy of the outcome, so it stays valid even if this
    // ExportThread is destroyed immediately afterwards.
    std::function<void(const Outcome&)> onFinished;

    void run() override;

private:
    GuiProcessor& processor;
    juce::Array<BankFolderParser::BankAssignment> cubbiAssignments;
    juce::Array<BankFolderParser::BankAssignment> jammiAssignments;
    juce::File stagingFolder;
    juce::File outputFolder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportThread)
};
