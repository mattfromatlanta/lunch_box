// SPDX-License-Identifier: AGPL-3.0-or-later
//
// MainComponent — core lifecycle, layout, focus/keyboard handling, and small
// shell actions (output-folder picker, edit copy/cut/paste, log + console
// toggles). Topic-specific concerns live in sibling translation units:
//   MainComponent_View.cpp     — view-mode + category-tab switching + animations
//   MainComponent_Session.cpp  — preferences and session save/restore
//   MainComponent_Process.cpp  — sample processing pipeline
//   MainComponent_Undo.cpp     — undo/redo
//   MainComponent_Commands.cpp — ApplicationCommandTarget

#include "MainComponent.h"
#include "ClipboardHelper.h"
#include "ConsoleWindow.h"
#include "UIColours.h"
#include "LabelStrings.h"
#include <BinaryData.h>

namespace
{
    const juce::Colour bgColour   = LunchBoxColours::DARK_GREY;
    const juce::Colour tabTextCol = LunchBoxColours::WHITE_CREAM;
}
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

    // Header icons
    {
        auto loadSvgDrawable = [](const char* data, int size) -> std::unique_ptr<juce::Drawable>
        {
            auto xml = juce::XmlDocument::parse(juce::String::fromUTF8(data, size));
            return xml ? juce::Drawable::createFromSVG(*xml) : nullptr;
        };
        boxIconDrawable   = loadSvgDrawable(BinaryData::box_icon_svg,       BinaryData::box_icon_svgSize);
        toastIconDrawable = loadSvgDrawable(BinaryData::toast_icon_svg,     BinaryData::toast_icon_svgSize);
        logoDrawable      = loadSvgDrawable(BinaryData::lunch_box_logo_svg, BinaryData::lunch_box_logo_svgSize);
    }

    using namespace LunchBoxLabels;

    // Mode toggle buttons
    packModeButton.setButtonText(kTabPack);
    bankModeButton.setButtonText(kTabBank);
    packModeButton.setTooltip(kTipTabPack);
    bankModeButton.setTooltip(kTipTabBank);
    packModeButton.onClick = [this] { if (!isTransitioning) setViewMode(ViewMode::Pack); };
    bankModeButton.onClick = [this] { if (!isTransitioning) setViewMode(ViewMode::Bank); };
    addAndMakeVisible(packModeButton);
    addAndMakeVisible(bankModeButton);


    // ── Pack mode components ───────────────────────────────

    cubbiTabButton.setButtonText(kTabCubbi);
    jammiTabButton.setButtonText(kTabJammi);
    cubbiTabButton.setTooltip(kTipTabCubbi);
    jammiTabButton.setTooltip(kTipTabJammi);
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

    processButton.setButtonText(kBtnProcess);
    processButton.setLookAndFeel(&footerButtonLAF);
    fillButton.setLookAndFeel(&footerButtonLAF);
    clearButton.setLookAndFeel(&footerButtonLAF);

    processButton.setTooltip(kTipBtnProcess);
    processButton.setColour(juce::TextButton::buttonColourId,  LunchBoxColours::BUTTON_BG);
    processButton.setColour(juce::TextButton::buttonOnColourId, LunchBoxColours::BUTTON_BG);
    processButton.setColour(juce::TextButton::textColourOffId,  LunchBoxColours::WHITE_CREAM);
    processButton.onClick = [this] { processFiles(); };
    processButton.setEnabled(false);
    addAndMakeVisible(processButton);

    fillButton.setButtonText(kBtnFill);
    fillButton.setTooltip(kTipBtnFill);
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

    clearButton.setButtonText(kBtnClear);
    clearButton.setTooltip(kTipBtnClear);
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
    addChildComponent(helpOverlay);

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

        if (logoDrawable != nullptr)
        {
            const juce::Colour logoColour = LunchBoxColours::WHITE_CREAM;
            auto copy = logoDrawable->createCopy();
            copy->replaceColour(juce::Colours::black, logoColour);
            copy->replaceColour(juce::Colours::white, logoColour);
            constexpr float scale = 0.85f;
            auto logoBounds = headerArea.withSizeKeepingCentre(headerArea.getWidth()  * scale,
                                                               headerArea.getHeight() * scale);
            copy->drawWithin(g, logoBounds,
                             juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
        }
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
    helpOverlay.setBounds(getLocalBounds());
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

void MainComponent::selectOutputFolder()
{
    auto startDir = outputBaseFolder.exists() ? outputBaseFolder
                                              : juce::File::getSpecialLocation(juce::File::userHomeDirectory);

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
void MainComponent::stopPreview()
{
    previewPanel.stopPlayback();
}

void MainComponent::mouseDown(const juce::MouseEvent& e)
{
    if (packNameOverlay.isVisible() || helpOverlay.isVisible())
        return;
    if (e.y < LunchBoxConstants::HEADER_HEIGHT)
        helpOverlay.show();
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
        {
            setCategoryTab(!showCubbiEditor, true);
            bankFocusPanel->switchToCategory(showCubbiEditor ? LunchBoxNamer::Category::Cubbi
                                                             : LunchBoxNamer::Category::Jammi);
        }
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
                setViewMode(ViewMode::Pack);
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
            setViewMode(viewMode == ViewMode::Pack ? ViewMode::Bank : ViewMode::Pack);
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
        sampleClipboard = bankFocusPanel->getSelectedClipboard();
    else
        sampleClipboard = getActiveEditor()->getSelectedClipboard();

    // Record the current system clipboard change count so editPaste() can detect
    // whether an external copy happened after this one.
    lastInternalCopyChangeCount = ClipboardHelper::getChangeCount();
}

void MainComponent::editCut()
{
    captureUndoState();
    if (viewMode == ViewMode::Bank)
    {
        sampleClipboard = bankFocusPanel->getSelectedClipboard();
        bankFocusPanel->clearFocusedRows();
    }
    else
    {
        sampleClipboard = getActiveEditor()->getSelectedClipboard();
        getActiveEditor()->clearSelectedCells();
    }

    lastInternalCopyChangeCount = ClipboardHelper::getChangeCount();
}

void MainComponent::editPaste()
{
    // If the system clipboard has been updated since our last internal copy,
    // the user copied something externally — prefer that over the stale internal clipboard.
    const bool externalCopyIsNewer = (ClipboardHelper::getChangeCount() != lastInternalCopyChangeCount);

    if (externalCopyIsNewer)
    {
        // External files have no geometry context — paste sequentially from focus
        juce::Array<juce::File> files = ClipboardHelper::getAudioFilesFromClipboard();
        if (!files.isEmpty())
        {
            juce::Array<ClipboardEntry> entries;
            for (int i = 0; i < files.size(); ++i)
                entries.add({ files[i], i, 0 });
            captureUndoState();
            if (viewMode == ViewMode::Bank)
                bankFocusPanel->pasteClipboard(entries);
            else
                getActiveEditor()->pasteClipboard(entries);
            return;
        }
    }

    if (sampleClipboard.isEmpty()) return;

    captureUndoState();
    if (viewMode == ViewMode::Bank)
        bankFocusPanel->pasteClipboard(sampleClipboard);
    else
        getActiveEditor()->pasteClipboard(sampleClipboard);
}
