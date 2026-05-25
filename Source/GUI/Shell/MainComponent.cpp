// SPDX-License-Identifier: AGPL-3.0-or-later
#include "MainComponent.h"
#include "ClipboardHelper.h"
#include "UIColours.h"
#include <BinaryData.h>

namespace
{
    const juce::Colour bgColour   = LunchBoxColours::DARK_GREY;
    const juce::Colour tabTextCol = LunchBoxColours::WHITE_CREAM;
}

//==============================================================================
// ConsoleWindow - floating status log window
//==============================================================================
class ConsoleWindow : public juce::DocumentWindow
{
public:
    explicit ConsoleWindow(const juce::String& initialContent)
        : juce::DocumentWindow("Console", LunchBoxColours::CONSOLE_BG, DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        editor.setMultiLine(true);
        editor.setReadOnly(true);
        editor.setScrollbarsShown(true);
        editor.setCaretVisible(false);
        editor.setFont(juce::Font(juce::FontOptions{}
                                      .withName(juce::Font::getDefaultMonospacedFontName())
                                      .withHeight(LunchBoxConstants::CONSOLE_FONT_SIZE)));
        editor.setColour(juce::TextEditor::backgroundColourId, LunchBoxColours::CONSOLE_BG);
        editor.setColour(juce::TextEditor::textColourId,       LunchBoxColours::CONSOLE_TEXT);
        editor.setColour(juce::TextEditor::outlineColourId,    LunchBoxColours::CONSOLE_OUTLINE);
        editor.setText(initialContent);
        setContentNonOwned(&editor, true);
        setSize(LunchBoxConstants::CONSOLE_WIDTH, LunchBoxConstants::CONSOLE_HEIGHT);
        setResizable(true, false);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        if (onClose) onClose();
    }

    juce::TextEditor editor;
    std::function<void()> onClose;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleWindow)
};


MainComponent::MainComponent()
{
    // Persistent preferences
    {
        juce::PropertiesFile::Options opts;
        opts.applicationName     = "LunchBox";
        opts.filenameSuffix      = "settings";
        opts.osxLibrarySubFolder = "Application Support";
        appProperties.setStorageParameters(opts);
    }

    lastPackName = getSavedString("lastPackName", lastPackName);

    // Mode toggle buttons
    packModeButton.setButtonText("Pack");
    bankModeButton.setButtonText("Bank");
    packModeButton.setTooltip("Pack view: 5-bank x 14-slot grid");
    bankModeButton.setTooltip("Bank view: focused single-bank waveform list");
    packModeButton.onClick = [this] { if (!isTransitioning) setViewMode(ViewMode::Pack); };
    bankModeButton.onClick = [this] { if (!isTransitioning) setViewMode(ViewMode::Bank); };
    addAndMakeVisible(packModeButton);
    addAndMakeVisible(bankModeButton);


    // ── Pack mode components ───────────────────────────────

    cubbiTabButton.setButtonText("Cubbi");
    jammiTabButton.setButtonText("Jammi");
    cubbiTabButton.setTooltip("Cubbi: percussive samples (Tab to toggle)");
    jammiTabButton.setTooltip("Jammi: chromatic / melodic samples (Tab to toggle)");
    cubbiTabButton.onClick = [this] {
        if (isTransitioning) return;
        if (viewMode == ViewMode::Bank)
            animateBankCategorySwitch(true);
        else
        {
            setCategoryTab(true, true);
            bankFocusPanel->switchToCategory(LunchBoxNamer::Category::Cubbi);
        }
    };
    jammiTabButton.onClick = [this] {
        if (isTransitioning) return;
        if (viewMode == ViewMode::Bank)
            animateBankCategorySwitch(false);
        else
        {
            setCategoryTab(false, true);
            bankFocusPanel->switchToCategory(LunchBoxNamer::Category::Jammi);
        }
    };
    addAndMakeVisible(cubbiTabButton);
    addAndMakeVisible(jammiTabButton);

    auto packSlotClicked = [this](BankEditorPanel::Cell, const juce::File& f)
    {
        previewPanel.playFile(f);
    };

    auto stopPreviewFn = [this] { stopPreview(); };

    cubbiEditor = std::make_unique<BankEditorPanel>(LunchBoxNamer::Category::Cubbi);
    cubbiEditor->onBeforeChange        = [this] { captureUndoState(); };
    cubbiEditor->onAssignmentsChanged  = [this] { updateProcessButtonState(); };
    cubbiEditor->onSlotClicked         = packSlotClicked;
    cubbiEditor->onPreviewStop         = [this] { stopPreview(); };
    cubbiEditor->getStartDirectory     = [this]() -> juce::File { return getSavedFolder("lastCubbiFolder"); };
    cubbiEditor->onFolderBrowsed       = [this](juce::File dir)  { saveFolder("lastCubbiFolder", dir); };
    cubbiEditor->onLog                 = [this](const juce::String& msg) { appendStatus(msg.trimEnd()); };
    cubbiEditor->onBackgroundClicked   = stopPreviewFn;
    addAndMakeVisible(cubbiEditor.get());

    jammiEditor = std::make_unique<BankEditorPanel>(LunchBoxNamer::Category::Jammi);
    jammiEditor->onBeforeChange        = [this] { captureUndoState(); };
    jammiEditor->onAssignmentsChanged  = [this] { updateProcessButtonState(); };
    jammiEditor->onSlotClicked         = packSlotClicked;
    jammiEditor->onPreviewStop         = [this] { stopPreview(); };
    jammiEditor->getStartDirectory     = [this]() -> juce::File { return getSavedFolder("lastJammiFolder"); };
    jammiEditor->onFolderBrowsed       = [this](juce::File dir)  { saveFolder("lastJammiFolder", dir); };
    jammiEditor->onLog                 = [this](const juce::String& msg) { appendStatus(msg.trimEnd()); };
    jammiEditor->onBackgroundClicked   = stopPreviewFn;
    addChildComponent(jammiEditor.get());  // hidden initially

    // ── Bank focus mode components ─────────────────────────

    bankFocusPanel = std::make_unique<BankFocusPanel>(
        previewPanel.getFormatManager(), previewPanel.getThumbnailCache());

    bankFocusPanel->onBeforeChange       = [this] { captureUndoState(); };
    bankFocusPanel->onAssignmentsChanged = [this] { updateProcessButtonState(); };
    bankFocusPanel->onSlotClicked        = [this](const juce::File& f) { previewPanel.playFile(f); };
    bankFocusPanel->onPreviewStop        = [this] { stopPreview(); };
    bankFocusPanel->getStartDirectory    = [this]() -> juce::File { return getSavedFolder("lastCubbiFolder"); };
    bankFocusPanel->onFolderBrowsed      = [this](juce::File dir)  { saveFolder("lastCubbiFolder", dir); };
    bankFocusPanel->onLog                = [this](const juce::String& msg) { bankStatusLabel.setText(msg.trimEnd(), juce::dontSendNotification); };
    addChildComponent(bankFocusPanel.get());  // hidden initially

    bankStatusLabel.setFont(LunchBoxFonts::footer());
    bankStatusLabel.setColour(juce::Label::textColourId, LunchBoxColours::WHITE_CREAM);
    bankStatusLabel.setJustificationType(juce::Justification::centredLeft);
    addChildComponent(bankStatusLabel);

    // ── Output folder section (shared) ────────────────────

    // Default output folder: ~/Desktop/chompis
    outputBaseFolder = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                           .getChildFile("chompis");

    // Restore persisted output folder
    {
        auto saved = getSavedFolder("lastOutputParent");
        if (saved != juce::File{})
            outputBaseFolder = saved;
    }

    processButton.setButtonText("Pack");
    processButton.setLookAndFeel(&footerButtonLAF);
    fillButton.setLookAndFeel(&footerButtonLAF);
    clearButton.setLookAndFeel(&footerButtonLAF);

    processButton.setTooltip("Convert and export all samples to CHOMPI format (Cmd+Return)");
    processButton.setColour(juce::TextButton::buttonColourId,  LunchBoxColours::BUTTON_BG);
    processButton.setColour(juce::TextButton::buttonOnColourId, LunchBoxColours::BUTTON_BG);
    processButton.setColour(juce::TextButton::textColourOffId,  LunchBoxColours::WHITE_CREAM);
    processButton.onClick = [this] { processFiles(); };
    processButton.setEnabled(false);
    addAndMakeVisible(processButton);

    fillButton.setButtonText("Fill");
    fillButton.setTooltip("Auto-fill empty slots from a folder");
    fillButton.setColour(juce::TextButton::buttonColourId,  LunchBoxColours::BUTTON_BG);
    fillButton.setColour(juce::TextButton::textColourOffId, LunchBoxColours::WHITE_CREAM);
    fillButton.onClick = [this]
    {
        if (viewMode == ViewMode::Pack)
            getActiveEditor()->autoFillFromFolder({});
        else
            bankFocusPanel->triggerAutoFill();
    };
    addAndMakeVisible(fillButton);

    clearButton.setButtonText("Clear");
    clearButton.setTooltip("Clear all slots in the current view");
    clearButton.setColour(juce::TextButton::buttonColourId,  LunchBoxColours::BUTTON_BG);
    clearButton.setColour(juce::TextButton::textColourOffId, LunchBoxColours::WHITE_CREAM);
    clearButton.onClick = [this]
    {
        captureUndoState();
        if (viewMode == ViewMode::Pack)
            getActiveEditor()->clearAllBanks();
        else
            bankFocusPanel->triggerClear();
    };
    addAndMakeVisible(clearButton);

    processor = std::make_unique<GuiProcessor>();
    addChildComponent(previewPanel);  // kept but hidden — will be re-introduced later
    addChildComponent(packNameOverlay);

    // Apply initial mode styling
    styleTabButton(packModeButton, true,  LunchBoxColours::YELLOW);
    styleTabButton(bankModeButton, false, LunchBoxColours::YELLOW);
    styleTabButton(cubbiTabButton, true,  LunchBoxColours::PURPLE);
    styleTabButton(jammiTabButton, false, LunchBoxColours::PURPLE);

    setWantsKeyboardFocus(true);

    // Register commands so AppMenuBar can use addCommandItem to wire keyboard shortcuts
    commandManager.registerAllCommandsForTarget(this);
    commandManager.setFirstCommandTarget(this);

    setSize(493, 890);

    loadSessionState();

#if CHOMPI_MELATONIN_INSPECTOR
    inspector = std::make_unique<melatonin::Inspector>(*this);
    inspector->setVisible(true);
#endif
}

MainComponent::~MainComponent()
{
    if (auto* top = getTopLevelComponent())
        top->removeKeyListener(this);
}

void MainComponent::parentHierarchyChanged()
{
    if (auto* top = getTopLevelComponent())
        top->addKeyListener(this);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(bgColour);

    {
        auto headerArea = getLocalBounds().removeFromTop(LunchBoxConstants::HEADER_HEIGHT).toFloat();
        g.setFont(LunchBoxFonts::logoTitle(60.0f));
        g.setColour(LunchBoxColours::WHITE_CREAM);
        g.drawText("LUNCH BOX", headerArea, juce::Justification::centred, false);
    }
}

void MainComponent::resized()
{
    // If a wipe is in flight and the window is resized, snap to final state
    if (isTransitioning)
    {
        juce::Desktop::getInstance().getAnimator().cancelAllAnimations(false);
        isTransitioning = false;
    }

    using namespace LunchBoxConstants;

    auto bounds = getLocalBounds();
    bounds.removeFromTop(HEADER_HEIGHT);
    auto area = bounds.reduced(CONTENT_H_MARGIN, 0).withTrimmedBottom(CONTENT_BOTTOM_PAD);

    // ── Nav row: [Cubbi][Jammi]  gap  [Pack][Bank] ────────
    {
        auto navRow = area.removeFromTop(NAV_ROW_HEIGHT);
        const int halfW = (navRow.getWidth() - NAV_PAIR_GAP) / 2;

        auto leftPair  = navRow.removeFromLeft(halfW);
        navRow.removeFromLeft(NAV_PAIR_GAP);
        auto rightPair = navRow;

        const int lHalf = leftPair.getWidth() / 2;
        cubbiTabButton.setBounds(leftPair.removeFromLeft(lHalf).reduced(BUTTON_H_INSET, 0));
        jammiTabButton.setBounds(leftPair.reduced(BUTTON_H_INSET, 0));

        const int rHalf = rightPair.getWidth() / 2;
        packModeButton.setBounds(rightPair.removeFromLeft(rHalf).reduced(BUTTON_H_INSET, 0));
        bankModeButton.setBounds(rightPair.reduced(BUTTON_H_INSET, 0));
    }
    area.removeFromTop(NAV_TO_CONTENT_GAP);

    // ── Footer: 3 equal buttons [Clear][Fill][Pack] ───────
    auto footer = area.removeFromBottom(FOOTER_BAND_HEIGHT);
    footer.removeFromTop(FOOTER_TOP_GAP);

    // ── Grid / content area ───────────────────────────────
    if (viewMode == ViewMode::Pack)
    {
        cubbiEditor->setBounds(area);
        jammiEditor->setBounds(area);
    }
    else  // ViewMode::Bank
    {
        cubbiEditor->setBounds({});
        jammiEditor->setBounds({});
        bankFocusPanel->setBounds(area);
    }

    // ── Footer button layout ──────────────────────────────
    {
        const int bW = footer.getWidth() / 3;
        clearButton.setBounds(footer.removeFromLeft(bW).reduced(BUTTON_H_INSET, 0));
        fillButton.setBounds(footer.removeFromLeft(bW).reduced(BUTTON_H_INSET, 0));
        processButton.setBounds(footer.reduced(BUTTON_H_INSET, 0));
    }

    packNameOverlay.setBounds(getLocalBounds());
}

// ─── Mode switching ───────────────────────────────────────────────────────────

juce::Rectangle<int> MainComponent::computeContentArea() const
{
    using namespace LunchBoxConstants;
    auto bounds = getLocalBounds();
    bounds.removeFromTop(HEADER_HEIGHT);
    auto area = bounds.reduced(CONTENT_H_MARGIN, 0).withTrimmedBottom(CONTENT_BOTTOM_PAD);
    area.removeFromTop(NAV_ROW_HEIGHT);
    area.removeFromTop(NAV_TO_CONTENT_GAP);
    area.removeFromBottom(FOOTER_BAND_HEIGHT);
    return area;
}

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
        syncBankFocusToAdvanced();
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

void MainComponent::syncBankFocusToAdvanced()
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

BankEditorPanel* MainComponent::getActiveEditor()
{
    return showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();
}

void MainComponent::styleTabButton(WipeTabButton& btn, bool active, juce::Colour activeCol)
{
    btn.snapToColour(active ? activeCol : LunchBoxColours::BUTTON_BG);
    btn.setColour(juce::TextButton::textColourOffId, tabTextCol);
    btn.setColour(juce::TextButton::textColourOnId,  LunchBoxColours::WHITE_CREAM);
}

// ─── Output folder ────────────────────────────────────────────────────────────

juce::File MainComponent::getResolvedOutputFolder()
{
    return outputBaseFolder.getChildFile(lastPackName);
}

juce::File MainComponent::getSavedFolder(const juce::String& key)
{
    if (auto* prefs = appProperties.getUserSettings())
    {
        auto path = prefs->getValue(key);
        if (path.isNotEmpty())
        {
            juce::File f(path);
            if (f.isDirectory()) return f;
        }
    }
    return juce::File{};
}

void MainComponent::saveFolder(const juce::String& key, const juce::File& folder)
{
    if (auto* prefs = appProperties.getUserSettings())
    {
        prefs->setValue(key, folder.getFullPathName());
        prefs->saveIfNeeded();
    }
}

juce::String MainComponent::getSavedString(const juce::String& key,
                                            const juce::String& fallback)
{
    if (auto* prefs = appProperties.getUserSettings())
    {
        auto val = prefs->getValue(key);
        if (val.isNotEmpty()) return val;
    }
    return fallback;
}

void MainComponent::saveString(const juce::String& key, const juce::String& value)
{
    if (auto* prefs = appProperties.getUserSettings())
    {
        prefs->setValue(key, value);
        prefs->saveIfNeeded();
    }
}

void MainComponent::saveSessionState()
{
    auto* props = appProperties.getUserSettings();
    if (!props) return;

    auto state = readCurrentState();

    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
        {
            props->setValue("session_cubbi_b" + juce::String(b) + "_s" + juce::String(s),
                            state.cubbiSlots[b][s].getFullPathName());
            props->setValue("session_jammi_b" + juce::String(b) + "_s" + juce::String(s),
                            state.jammiSlots[b][s].getFullPathName());
        }

    props->saveIfNeeded();
}

void MainComponent::loadSessionState()
{
    auto* props = appProperties.getUserSettings();
    if (!props) return;

    juce::StringArray missing;

    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
        {
            for (int cat = 0; cat < 2; ++cat)
            {
                auto prefix = (cat == 0) ? "cubbi" : "jammi";
                auto path = props->getValue("session_" + juce::String(prefix)
                                            + "_b" + juce::String(b) + "_s" + juce::String(s));
                if (path.isEmpty()) continue;

                juce::File f(path);
                if (f.existsAsFile())
                {
                    auto* editor = (cat == 0) ? cubbiEditor.get() : jammiEditor.get();
                    editor->setSlotFile(b, s, f);
                }
                else
                {
                    missing.add(path);
                }
            }
        }

    if (!missing.isEmpty())
    {
        appendStatus("Session restore: " + juce::String(missing.size()) + " file(s) not available:");
        for (auto& path : missing)
            appendStatus("  Missing: " + path);
    }

    updateProcessButtonState();
}

// ─── File browser launchers ───────────────────────────────────────────────────

void MainComponent::selectOutputFolder()
{
    auto startDir = getResolvedOutputFolder();
    if (!startDir.exists())
        startDir = outputBaseFolder;

    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Output Folder", startDir, "", true);

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
    {
        auto f = chooser.getResult();
        if (f.isDirectory())
        {
            outputBaseFolder = f;
            saveFolder("lastOutputParent", outputBaseFolder);
            appendStatus("Output folder: " + outputBaseFolder.getFullPathName());
        }
    });
}

// ─── Processing ───────────────────────────────────────────────────────────────

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
                        syncBankFocusToAdvanced();
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

void MainComponent::stopPreview()
{
    previewPanel.stopPlayback();
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* origin)
{
    suppressTooltipsUntilMouseMove();

    if (packNameOverlay.isVisible())
        return false;

    // Don't intercept navigation keys when a text editor has keyboard focus
    if (dynamic_cast<juce::TextEditor*>(origin) != nullptr)
        return false;

    if (key == juce::KeyPress(juce::KeyPress::tabKey, juce::ModifierKeys::shiftModifier, 0))
    {
        if (!isTransitioning)
            setViewMode(viewMode == ViewMode::Pack ? ViewMode::Bank : ViewMode::Pack);
        return true;
    }

    if (key == juce::KeyPress::escapeKey)
    {
        stopPreview();
        if (viewMode == ViewMode::Bank)
            bankFocusPanel->clearSelection();
        else
            getActiveEditor()->setFocusCellAndSelect(getActiveEditor()->getFocusCell());
        return true;
    }

    if (key == juce::KeyPress::spaceKey)
    {
        if (viewMode == ViewMode::Pack)
        {
            stopPreview();
            getActiveEditor()->playFocused();
        }
        else  // Bank
        {
            stopPreview();
            bankFocusPanel->playFocused();
        }
        return true;
    }

    // ── Bank view keyboard navigation ────────────────────────────────────────
    if (viewMode == ViewMode::Bank)
    {
        if (key.getModifiers().isCommandDown())
        {
            if (key.getKeyCode() == juce::KeyPress::upKey)
            {
                int next = juce::jmax(0, bankFocusPanel->getActiveBank() - 1);
                bankFocusPanel->setActiveFocus(next, bankFocusPanel->getFocusedRow());
                return true;
            }
            if (key.getKeyCode() == juce::KeyPress::downKey)
            {
                int next = juce::jmin(LunchBoxNamer::NUM_BANKS - 1, bankFocusPanel->getActiveBank() + 1);
                bankFocusPanel->setActiveFocus(next, bankFocusPanel->getFocusedRow());
                return true;
            }
        }
        if (key.getModifiers().isShiftDown())
        {
            if (key.getKeyCode() == juce::KeyPress::upKey)   { bankFocusPanel->expandRowSelection(-1); return true; }
            if (key.getKeyCode() == juce::KeyPress::downKey) { bankFocusPanel->expandRowSelection( 1); return true; }
        }
        if (key == juce::KeyPress::upKey)   { bankFocusPanel->moveFocusedRow(-1); return true; }
        if (key == juce::KeyPress::downKey) { bankFocusPanel->moveFocusedRow( 1); return true; }
        if (key == juce::KeyPress::deleteKey
         || key == juce::KeyPress::backspaceKey) { captureUndoState(); bankFocusPanel->clearFocusedRows(); return true; }
        if (key == juce::KeyPress::tabKey)
        {
            if (!isTransitioning)
                animateBankCategorySwitch(!showCubbiEditor);
            return true;
        }
        if (key == juce::KeyPress::returnKey)
        {
            if (dynamic_cast<juce::Button*>(origin) != nullptr) return false;
            bankFocusPanel->browseForFocused();
            return true;
        }
        return false;
    }

    // ── Pack view keyboard navigation ─────────────────────────────────────────
    auto* ed = getActiveEditor();

    if (key.getModifiers().isShiftDown())
    {
        if (key.getKeyCode() == juce::KeyPress::leftKey)  { ed->expandSelection(0, -1); return true; }
        if (key.getKeyCode() == juce::KeyPress::rightKey) { ed->expandSelection(0,  1); return true; }
        if (key.getKeyCode() == juce::KeyPress::upKey)    { ed->expandSelection(-1, 0); return true; }
        if (key.getKeyCode() == juce::KeyPress::downKey)  { ed->expandSelection( 1, 0); return true; }
    }

    if (key == juce::KeyPress::leftKey)     { ed->moveFocus(0, -1);  return true; }
    if (key == juce::KeyPress::rightKey)    { ed->moveFocus(0,  1);  return true; }
    if (key == juce::KeyPress::upKey)       { ed->moveFocus(-1, 0);  return true; }
    if (key == juce::KeyPress::downKey)     { ed->moveFocus( 1, 0);  return true; }

    if (key == juce::KeyPress::tabKey)
    {
        if (!isTransitioning)
        {
            setCategoryTab(!showCubbiEditor, true);
            bankFocusPanel->switchToCategory(showCubbiEditor ? LunchBoxNamer::Category::Cubbi
                                                             : LunchBoxNamer::Category::Jammi);
        }
        return true;
    }

    if (key == juce::KeyPress::deleteKey
     || key == juce::KeyPress::backspaceKey) { captureUndoState(); ed->clearSelectedCells(); return true; }

    if (key == juce::KeyPress::returnKey)
    {
        if (dynamic_cast<juce::Button*>(origin) != nullptr) return false;
        ed->browseForFocused();
        return true;
    }

    return false;
}

void MainComponent::toggleConsole()
{
    consoleVisible = !consoleVisible;
    if (consoleVisible)
    {
        consoleWindow = std::make_unique<ConsoleWindow>(consoleContent);
        consoleWindow->onClose = [this] { toggleConsole(); };
    }
    else
    {
        consoleWindow.reset();
    }
}

void MainComponent::showLogFolder()
{
    juce::File logsDir = juce::File::getCurrentWorkingDirectory().getChildFile("logs");
    if (!logsDir.exists())
        logsDir.createDirectory();
    logsDir.revealToUser();
}

void MainComponent::clearStatusLog()
{
    consoleContent = "Status log cleared.\n";
    if (consoleWindow != nullptr)
        consoleWindow->editor.setText(consoleContent);
}

void MainComponent::editCopy()
{
    if (viewMode == ViewMode::Bank)
        sampleClipboard = bankFocusPanel->getSelectedFiles();
    else
        sampleClipboard = getActiveEditor()->getSelectedFiles();

    // Record the current system clipboard change count so editPaste() can detect
    // whether an external copy happened after this one.
    lastInternalCopyChangeCount = ClipboardHelper::getChangeCount();
}

void MainComponent::editCut()
{
    captureUndoState();
    if (viewMode == ViewMode::Bank)
    {
        sampleClipboard = bankFocusPanel->getSelectedFiles();
        bankFocusPanel->clearFocusedRows();
    }
    else
    {
        sampleClipboard = getActiveEditor()->getSelectedFiles();
        getActiveEditor()->clearSelectedCells();
    }

    lastInternalCopyChangeCount = ClipboardHelper::getChangeCount();
}

void MainComponent::editPaste()
{
    // If the system clipboard has been updated since our last internal copy,
    // the user copied something externally — prefer that over the stale internal clipboard.
    const bool externalCopyIsNewer = (ClipboardHelper::getChangeCount() != lastInternalCopyChangeCount);

    juce::Array<juce::File> files;
    if (externalCopyIsNewer)
        files = ClipboardHelper::getAudioFilesFromClipboard();

    // Fall back to internal clipboard if external yielded nothing
    if (files.isEmpty())
        files = sampleClipboard;

    if (files.isEmpty()) return;

    captureUndoState();
    if (viewMode == ViewMode::Bank)
        bankFocusPanel->pasteFiles(files);
    else
        getActiveEditor()->pasteFiles(files);
}

// ─── Undo / redo ──────────────────────────────────────────────────────────────

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
    juce::MessageManager::callAsync([this] { saveSessionState(); });
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
    switch (id)
    {
        case cmdUndo:
            result.setInfo("Undo", "Undo last action", "Edit", 0);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier);
            break;
        case cmdRedo:
            result.setInfo("Redo", "Redo last undone action", "Edit", 0);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case cmdCopy:
            result.setInfo("Copy",  "Copy selected samples",  "Edit", 0);
            result.addDefaultKeypress('c', juce::ModifierKeys::commandModifier);
            break;
        case cmdCut:
            result.setInfo("Cut",   "Cut selected samples",   "Edit", 0);
            result.addDefaultKeypress('x', juce::ModifierKeys::commandModifier);
            break;
        case cmdPaste:
            result.setInfo("Paste", "Paste samples",          "Edit", 0);
            result.addDefaultKeypress('v', juce::ModifierKeys::commandModifier);
            break;
        case cmdSelectAll:
            result.setInfo("Select All", "Select all slots", "Edit", 0);
            result.addDefaultKeypress('a', juce::ModifierKeys::commandModifier);
            break;
        case cmdOpenOutput:
            result.setInfo("Open Output Folder", "Open output folder in Finder", "File", 0);
            result.addDefaultKeypress('o', juce::ModifierKeys::commandModifier);
            break;
        case cmdProcess:
            result.setInfo("Process Samples", "Convert and export all samples", "File", 0);
            result.addDefaultKeypress(juce::KeyPress::returnKey, juce::ModifierKeys::commandModifier);
            result.addDefaultKeypress('p', juce::ModifierKeys::commandModifier);
            break;
        case cmdFill:
            result.setInfo("Fill Slots", "Auto-fill empty slots from a folder", "Edit", 0);
            result.addDefaultKeypress('f', juce::ModifierKeys::commandModifier);
            break;
        case cmdClear:
            result.setInfo("Clear Slots", "Clear selected slots", "Edit", 0);
            result.addDefaultKeypress('c', juce::ModifierKeys::commandModifier | juce::ModifierKeys::altModifier);
            break;
        case cmdToggleConsole:
            result.setInfo(consoleVisible ? "Hide Console" : "Show Console",
                           "Toggle the console window", "Settings", 0);
            result.addDefaultKeypress('/', juce::ModifierKeys::commandModifier);
            result.setTicked(consoleVisible);
            break;
        default:
            break;
    }
}

bool MainComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo& info)
{
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
        case cmdOpenOutput:  getResolvedOutputFolder().startAsProcess(); return true;
        case cmdProcess:     if (processButton.isEnabled()) processFiles(); return true;
        case cmdToggleConsole: toggleConsole(); return true;
        case cmdFill:        if (fillButton.isEnabled())  fillButton.triggerClick();  return true;
        case cmdClear:       if (clearButton.isEnabled()) clearButton.triggerClick(); return true;
        default:               return false;
    }
}
