// SPDX-License-Identifier: AGPL-3.0-or-later
//
// View-mode switching (Pack ↔ Bank), category-tab switching (Cubbi ↔ Jammi),
// and the cross-tab wipe / category-switch animations.

#include "MainComponent.h"
#include "UIColours.h"
#include "LabelStrings.h"

void MainComponent::setViewMode(ViewMode mode)
{
    if (mode == viewMode) return;

    // Snap any in-progress wipe before starting a new one
    if (isTransitioning)
    {
        juce::Desktop::getInstance().getAnimator().cancelAllAnimations(false);
        isTransitioning = false;
    }

    // Sync data between modes
    if (viewMode == ViewMode::Bank && mode != ViewMode::Bank)
        syncBankFocusToPack();
    if (mode == ViewMode::Bank && viewMode != ViewMode::Bank)
        syncPackToBankFocus();

    viewMode = mode;

    const bool isPack = (mode == ViewMode::Pack);
    const bool isBank = (mode == ViewMode::Bank);

    // Category tabs shared between Pack and Bank modes
    cubbiTabButton.setVisible(true);
    jammiTabButton.setVisible(true);
    if (isPack)
    {
        setCategoryTab(showCubbiEditor);
        // Translate Bank focus → Pack cell so the incoming editor lands on the right slot
        auto* incoming = showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();
        incoming->setFocusCellAndSelect({ bankFocusPanel->getActiveBank(),
                                          bankFocusPanel->getFocusedRow() });
    }
    else
    {
        styleTabButton(cubbiTabButton, showCubbiEditor,  LunchBoxColours::PURPLE);
        styleTabButton(jammiTabButton, !showCubbiEditor, LunchBoxColours::PURPLE);
        auto cat = showCubbiEditor ? LunchBoxNamer::Category::Cubbi
                                   : LunchBoxNamer::Category::Jammi;
        bankFocusPanel->switchToCategory(cat);
        // Translate Pack focus → Bank bank+row so the incoming panel lands on the right slot
        auto fc = (showCubbiEditor ? cubbiEditor.get() : jammiEditor.get())->getFocusCell();
        bankFocusPanel->setActiveFocus(fc.row, fc.col);
    }

    updateProcessButtonState();

    // ── Wipe transition ───────────────────────────────────────────────────────
    auto content = computeContentArea();
    const int w = content.getWidth();
    auto& anim = juce::Desktop::getInstance().getAnimator();

    if (isBank)  // Pack wipes out to the left; Bank wipes in from the right
    {
        auto* outgoing = showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();

        bankFocusPanel->setBounds(content.withX(content.getX() + w));
        bankFocusPanel->setVisible(true);
        bankStatusLabel.setVisible(true);
        bankFocusPanel->grabKeyboardFocus();

        packModeButton.startWipe(LunchBoxColours::BUTTON_BG,  true);
        bankModeButton.startWipe(LunchBoxColours::YELLOW,    true);

        isTransitioning = true;
        anim.animateComponent(outgoing,         content.withX(content.getX() - w), 1.0f, LunchBoxConstants::ANIM_DURATION_MS, false, 0.0, 0.0);
        anim.animateComponent(bankFocusPanel.get(), content,                        1.0f, LunchBoxConstants::ANIM_DURATION_MS, false, 0.0, 0.0);

        juce::Timer::callAfterDelay(LunchBoxConstants::ANIM_CLEANUP_DELAY_MS, [this, outgoing]
        {
            outgoing->setVisible(false);
            isTransitioning = false;
            resized();
        });
    }
    else  // Bank wipes out to the right; Pack wipes in from the left
    {
        auto* incoming = showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();

        incoming->setBounds(content.withX(content.getX() - w));
        incoming->setVisible(true);

        packModeButton.startWipe(LunchBoxColours::YELLOW,   false);
        bankModeButton.startWipe(LunchBoxColours::BUTTON_BG, false);

        isTransitioning = true;
        anim.animateComponent(bankFocusPanel.get(), content.withX(content.getX() + w), 1.0f, LunchBoxConstants::ANIM_DURATION_MS, false, 0.0, 0.0);
        anim.animateComponent(incoming,             content,                            1.0f, LunchBoxConstants::ANIM_DURATION_MS, false, 0.0, 0.0);

        juce::Timer::callAfterDelay(LunchBoxConstants::ANIM_CLEANUP_DELAY_MS, [this]
        {
            bankFocusPanel->setVisible(false);
            bankStatusLabel.setVisible(false);
            isTransitioning = false;
            resized();
        });
    }
}

void MainComponent::syncPackToBankFocus()
{
    bankFocusPanel->clearAll();

    for (const auto& a : cubbiEditor->getAssignments())
    {
        int bankIdx = (int)(a.bankLetter - 'a');
        bankFocusPanel->setSlot(LunchBoxNamer::Category::Cubbi, bankIdx, a.slotNumber - 1, a.sourceFile);
    }

    for (const auto& a : jammiEditor->getAssignments())
    {
        int bankIdx = (int)(a.bankLetter - 'a');
        bankFocusPanel->setSlot(LunchBoxNamer::Category::Jammi, bankIdx, a.slotNumber - 1, a.sourceFile);
    }
}

void MainComponent::syncBankFocusToPack()
{
    cubbiEditor->clearAllBanks();
    jammiEditor->clearAllBanks();

    for (const auto& a : bankFocusPanel->getAssignments(LunchBoxNamer::Category::Cubbi))
    {
        int bankIdx = (int)(a.bankLetter - 'a');
        cubbiEditor->setSlotFile(bankIdx, a.slotNumber - 1, a.sourceFile);
    }

    for (const auto& a : bankFocusPanel->getAssignments(LunchBoxNamer::Category::Jammi))
    {
        int bankIdx = (int)(a.bankLetter - 'a');
        jammiEditor->setSlotFile(bankIdx, a.slotNumber - 1, a.sourceFile);
    }
}

void MainComponent::animateBankCategorySwitch(bool showCubbi)
{
    if (showCubbi == showCubbiEditor) return;

    if (isTransitioning)
        juce::Desktop::getInstance().getAnimator().cancelAllAnimations(false);

    auto content = computeContentArea();
    const int w   = content.getWidth();
    const int dir = showCubbi ? -1 : 1;  // Jammi = right, Cubbi = left
    auto& anim    = juce::Desktop::getInstance().getAnimator();

    // Wipe button colours before any state change so startWipe captures the current fill
    const bool fromRight = !showCubbi;
    cubbiTabButton.startWipe(showCubbi ? LunchBoxColours::PURPLE : LunchBoxColours::BUTTON_BG, fromRight);
    jammiTabButton.startWipe(showCubbi ? LunchBoxColours::BUTTON_BG : LunchBoxColours::PURPLE, fromRight);

    // Snapshot the current panel state before any content change
    auto snapshot = bankFocusPanel->createComponentSnapshot(bankFocusPanel->getLocalBounds());
    bankTransitionOverlay = std::make_unique<juce::ImageComponent>();
    bankTransitionOverlay->setImage(snapshot);
    bankTransitionOverlay->setBounds(content);
    addAndMakeVisible(bankTransitionOverlay.get());
    bankTransitionOverlay->toFront(false);

    // Switch content and update state (no styleTabButton — startWipe handles colours)
    stopPreview();
    showCubbiEditor = showCubbi;
    bankFocusPanel->switchToCategory(showCubbi ? LunchBoxNamer::Category::Cubbi
                                               : LunchBoxNamer::Category::Jammi);
    bankFocusPanel->setBounds(content.withX(content.getX() + dir * w));

    isTransitioning = true;
    anim.animateComponent(bankTransitionOverlay.get(), content.withX(content.getX() - dir * w), 1.0f, LunchBoxConstants::ANIM_DURATION_MS, false, 0.0, 0.0);
    anim.animateComponent(bankFocusPanel.get(),        content,                                  1.0f, LunchBoxConstants::ANIM_DURATION_MS, false, 0.0, 0.0);

    juce::Timer::callAfterDelay(LunchBoxConstants::ANIM_CLEANUP_DELAY_MS, [this]
    {
        if (bankTransitionOverlay != nullptr)
        {
            removeChildComponent(bankTransitionOverlay.get());
            bankTransitionOverlay.reset();
        }
        isTransitioning = false;
        resized();
    });
}

void MainComponent::setCategoryTab(bool showCubbi, bool animate)
{
    stopPreview();

    if (viewMode == ViewMode::Pack)
    {
        const bool tabChanged = (showCubbi != showCubbiEditor);

        if (animate && tabChanged)
        {
            if (isTransitioning)
                juce::Desktop::getInstance().getAnimator().cancelAllAnimations(false);

            auto content = computeContentArea();
            const int w   = content.getWidth();
            auto& anim    = juce::Desktop::getInstance().getAnimator();

            auto* outgoing = showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();
            auto* incoming = showCubbi       ? cubbiEditor.get() : jammiEditor.get();

            // Jammi is "right" of Cubbi: going to Jammi = dir +1, going to Cubbi = dir -1
            const int  dir       = showCubbi ? -1 : 1;
            const bool fromRight = !showCubbi;

            cubbiTabButton.startWipe(showCubbi ? LunchBoxColours::PURPLE : LunchBoxColours::BUTTON_BG, fromRight);
            jammiTabButton.startWipe(showCubbi ? LunchBoxColours::BUTTON_BG : LunchBoxColours::PURPLE, fromRight);

            incoming->setBounds(content.withX(content.getX() + dir * w));
            incoming->setVisible(true);

            isTransitioning = true;
            anim.animateComponent(outgoing, content.withX(content.getX() - dir * w), 1.0f, LunchBoxConstants::ANIM_DURATION_MS, false, 0.0, 0.0);
            anim.animateComponent(incoming, content,                                  1.0f, LunchBoxConstants::ANIM_DURATION_MS, false, 0.0, 0.0);

            juce::Timer::callAfterDelay(LunchBoxConstants::ANIM_CLEANUP_DELAY_MS, [this, outgoing]
            {
                outgoing->setVisible(false);
                isTransitioning = false;
                resized();
            });
        }
        else
        {
            styleTabButton(cubbiTabButton, showCubbi,  LunchBoxColours::PURPLE);
            styleTabButton(jammiTabButton, !showCubbi, LunchBoxColours::PURPLE);
            cubbiEditor->setVisible(showCubbi);
            jammiEditor->setVisible(!showCubbi);
        }
    }
    else
    {
        styleTabButton(cubbiTabButton, showCubbi,  LunchBoxColours::PURPLE);
        styleTabButton(jammiTabButton, !showCubbi, LunchBoxColours::PURPLE);
    }

    showCubbiEditor = showCubbi;
}
