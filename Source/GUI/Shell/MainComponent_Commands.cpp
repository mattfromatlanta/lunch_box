// SPDX-License-Identifier: AGPL-3.0-or-later
//
// ApplicationCommandTarget implementation — wires menu items and keyboard
// shortcuts to MainComponent actions.

#include "MainComponent.h"
#include "LabelStrings.h"

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget()
{
    return nullptr;
}

void MainComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.addArray({ cmdUndo, cmdRedo, cmdCopy, cmdCut, cmdPaste, cmdSelectAll,
                        cmdOpenOutput, cmdProcess, cmdToggleConsole, cmdFill, cmdClear });
}

void MainComponent::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& result)
{
    using namespace LunchBoxLabels;
    switch (id)
    {
        case cmdUndo:
            result.setInfo(kCmdUndo, kCmdUndoDesc, kMenuEdit, 0);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier);
            break;
        case cmdRedo:
            result.setInfo(kCmdRedo, kCmdRedoDesc, kMenuEdit, 0);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case cmdCopy:
            result.setInfo(kCmdCopy, kCmdCopyDesc, kMenuEdit, 0);
            result.addDefaultKeypress('c', juce::ModifierKeys::commandModifier);
            break;
        case cmdCut:
            result.setInfo(kCmdCut, kCmdCutDesc, kMenuEdit, 0);
            result.addDefaultKeypress('x', juce::ModifierKeys::commandModifier);
            break;
        case cmdPaste:
            result.setInfo(kCmdPaste, kCmdPasteDesc, kMenuEdit, 0);
            result.addDefaultKeypress('v', juce::ModifierKeys::commandModifier);
            break;
        case cmdSelectAll:
            result.setInfo(kCmdSelectAll, kCmdSelectAllDesc, kMenuEdit, 0);
            result.addDefaultKeypress('a', juce::ModifierKeys::commandModifier);
            break;
        case cmdOpenOutput:
            result.setInfo(kCmdOpenOutput, kCmdOpenOutputDesc, kMenuFile, 0);
            result.addDefaultKeypress('o', juce::ModifierKeys::commandModifier);
            break;
        case cmdProcess:
            result.setInfo(kCmdProcess, kCmdProcessDesc, kMenuFile, 0);
            result.addDefaultKeypress(juce::KeyPress::returnKey, juce::ModifierKeys::commandModifier);
            result.addDefaultKeypress('p', juce::ModifierKeys::commandModifier);
            break;
        case cmdFill:
            result.setInfo(kCmdFill, kCmdFillDesc, kMenuEdit, 0);
            result.addDefaultKeypress('f', juce::ModifierKeys::commandModifier);
            break;
        case cmdClear:
            result.setInfo(kCmdClear, kCmdClearDesc, kMenuEdit, 0);
            result.addDefaultKeypress('c', juce::ModifierKeys::commandModifier | juce::ModifierKeys::altModifier);
            break;
        case cmdToggleConsole:
            result.setInfo(consoleVisible ? kCmdHideConsole : kCmdShowConsole,
                           kCmdToggleConsoleDesc, kMenuSettings, 0);
            result.addDefaultKeypress('/', juce::ModifierKeys::commandModifier);
            result.setTicked(consoleVisible);
            break;
        default:
            break;
    }
}

bool MainComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo& info)
{
    // While Help is open, swallow every command. This is the one chokepoint both
    // the macOS menu key-equivalents and the key-mapping dispatch funnel through,
    // so it blocks shortcuts the keyPressed() guard can't see. (Cmd+Q is a system
    // menu item, not one of ours, so quitting still works.)
    if (helpOverlay.isVisible())
        return true;

    switch (info.commandID)
    {
        case cmdUndo:        performUndo();   return true;
        case cmdRedo:        performRedo();   return true;
        case cmdCopy:        editCopy();      return true;
        case cmdCut:         editCut();       return true;
        case cmdPaste:       editPaste();     return true;
        case cmdSelectAll:
        {
            if (viewMode == ViewMode::Bank)
                bankFocusPanel->selectAll();
            else
                getActiveEditor()->selectAll();
            return true;
        }
        case cmdOpenOutput:  getResolvedOutputFolder().revealToUser(); return true;
        case cmdProcess:     if (processButton.isEnabled()) processFiles(); return true;
        case cmdToggleConsole: toggleConsole(); return true;
        case cmdFill:        if (fillButton.isEnabled())  fillButton.triggerClick();  return true;
        case cmdClear:       if (clearButton.isEnabled()) clearButton.triggerClick(); return true;
        default:               return false;
    }
}
