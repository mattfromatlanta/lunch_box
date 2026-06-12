// SPDX-License-Identifier: AGPL-3.0-or-later
//
// 10-step undo/redo: full-state snapshot, capture, apply, and stack management.

#include "MainComponent.h"

MainComponent::UndoState MainComponent::readCurrentState()
{
    UndoState state;
    if (viewMode == ViewMode::Bank)
    {
        for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
            for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
            {
                state.cubbiSlots[b][s] = bankFocusPanel->getSlotFile(LunchBoxNamer::Category::Cubbi, b, s);
                state.jammiSlots[b][s] = bankFocusPanel->getSlotFile(LunchBoxNamer::Category::Jammi, b, s);
            }
    }
    else
    {
        for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
            for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
            {
                state.cubbiSlots[b][s] = cubbiEditor->getSlotFile(b, s);
                state.jammiSlots[b][s] = jammiEditor->getSlotFile(b, s);
            }
    }
    return state;
}

void MainComponent::captureUndoState()
{
    if (isApplyingUndoState) return;

    undoStack.add(readCurrentState());
    if (undoStack.size() > MAX_UNDO_STEPS)
        undoStack.remove(0);

    redoStack.clear();

    // Save after the current event loop so the slot change has been applied.
    juce::Component::SafePointer<MainComponent> safeThis(this);
    juce::MessageManager::callAsync([safeThis]
    {
        if (safeThis != nullptr)
            safeThis->saveSessionState();
    });
}

void MainComponent::applyUndoState(const UndoState& state)
{
    isApplyingUndoState = true;

    cubbiEditor->clearAllBanks();
    jammiEditor->clearAllBanks();

    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
        {
            cubbiEditor->setSlotFile(b, s, state.cubbiSlots[b][s]);
            jammiEditor->setSlotFile(b, s, state.jammiSlots[b][s]);
        }

    if (viewMode == ViewMode::Bank)
        syncPackToBankFocus();

    isApplyingUndoState = false;
    updateProcessButtonState();
    saveSessionState();
}

void MainComponent::performUndo()
{
    if (undoStack.isEmpty()) return;

    redoStack.add(readCurrentState());

    auto prev = undoStack.getLast();
    undoStack.removeLast();

    applyUndoState(prev);
}

void MainComponent::performRedo()
{
    if (redoStack.isEmpty()) return;

    undoStack.add(readCurrentState());
    if (undoStack.size() > MAX_UNDO_STEPS)
        undoStack.remove(0);

    auto next = redoStack.getLast();
    redoStack.removeLast();

    applyUndoState(next);
}

// ─── ApplicationCommandTarget ─────────────────────────────────────────────────
