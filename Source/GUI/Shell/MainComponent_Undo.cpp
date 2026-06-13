// SPDX-License-Identifier: AGPL-3.0-or-later
//
// 10-step undo/redo: full-state snapshot, capture, apply, and stack management.

#include "MainComponent.h"

PackModel::Snapshot MainComponent::readCurrentState()
{
    // Bank view holds edits in its visible rows until flushed; make sure the
    // model is current before snapshotting. (Pack edits write through eagerly.)
    if (viewMode == ViewMode::Bank)
        bankFocusPanel->commitActiveBankToModel();

    return packModel.snapshot();
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

void MainComponent::applyUndoState(const PackModel::Snapshot& state)
{
    isApplyingUndoState = true;

    packModel.restore(state);

    // Refresh whichever view is showing; the other repopulates from the model
    // when it next becomes visible.
    if (viewMode == ViewMode::Bank)
        bankFocusPanel->refreshActiveFromModel();
    else
        getActiveEditor()->refreshFromModel();

    isApplyingUndoState = false;
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
