#include "MainComponent.h"
#include "ClipboardHelper.h"
#include <BinaryData.h>

// M9 color palette
namespace
{
    const juce::Colour bgColour        { 0xff1a1f2e };
    const juce::Colour sectionColour   { 0xff8899aa };
    const juce::Colour headerColour    { 0xffe0e8f0 };
    const juce::Colour statusBgColour  { 0xff151a26 };
    const juce::Colour statusFgColour  { 0xffaabbcc };
    const juce::Colour accentColour    { 0xff4caf50 };
    const juce::Colour tabActiveCol    { 0xff2a4060 };
    const juce::Colour tabInactiveCol  { 0xff1e2838 };
    const juce::Colour tabTextCol      { 0xffaabbcc };
}

//==============================================================================
// ConsoleWindow - floating status log window
//==============================================================================
class ConsoleWindow : public juce::DocumentWindow
{
public:
    explicit ConsoleWindow(const juce::String& initialContent)
        : juce::DocumentWindow("Console", juce::Colour(0xff151a26), DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        editor.setMultiLine(true);
        editor.setReadOnly(true);
        editor.setScrollbarsShown(true);
        editor.setCaretVisible(false);
        editor.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
        editor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff151a26));
        editor.setColour(juce::TextEditor::textColourId,       juce::Colour(0xffaabbcc));
        editor.setColour(juce::TextEditor::outlineColourId,    juce::Colour(0xff2a3a4a));
        editor.setText(initialContent);
        setContentNonOwned(&editor, true);
        setSize(520, 280);
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
        opts.applicationName     = "ChompiPack";
        opts.filenameSuffix      = "settings";
        opts.osxLibrarySubFolder = "Application Support";
        appProperties.setStorageParameters(opts);
    }

    logoImage = juce::ImageCache::getFromMemory(BinaryData::chompi_logo_png,
                                                BinaryData::chompi_logo_pngSize);

    // Mode toggle buttons
    packModeButton.setButtonText("Pack");
    bankModeButton.setButtonText("Bank");
    packModeButton.onClick = [this] { setViewMode(ViewMode::Pack); };
    bankModeButton.onClick = [this] { setViewMode(ViewMode::Bank); };
    addAndMakeVisible(packModeButton);
    addAndMakeVisible(bankModeButton);


    // ── Pack mode components ───────────────────────────────

    cubbiTabButton.setButtonText("Cubbi");
    jammiTabButton.setButtonText("Jammi");
    cubbiTabButton.onClick = [this] {
        setCategoryTab(true);
        bankFocusPanel->switchToCategory(ChompiNamer::Category::Cubbi);
    };
    jammiTabButton.onClick = [this] {
        setCategoryTab(false);
        bankFocusPanel->switchToCategory(ChompiNamer::Category::Jammi);
    };
    addAndMakeVisible(cubbiTabButton);
    addAndMakeVisible(jammiTabButton);

    auto packSlotClicked = [this](BankEditorPanel::Cell, const juce::File& f)
    {
        previewPanel.playFile(f);
    };

    auto stopPreviewFn = [this] { stopPreview(); };

    cubbiEditor = std::make_unique<BankEditorPanel>(ChompiNamer::Category::Cubbi);
    cubbiEditor->onBeforeChange        = [this] { captureUndoState(); };
    cubbiEditor->onAssignmentsChanged  = [this] { updateProcessButtonState(); };
    cubbiEditor->onSlotClicked         = packSlotClicked;
    cubbiEditor->onPreviewStop         = [this] { stopPreview(); };
    cubbiEditor->getStartDirectory     = [this]() -> juce::File { return getSavedFolder("lastCubbiFolder"); };
    cubbiEditor->onFolderBrowsed       = [this](juce::File dir)  { saveFolder("lastCubbiFolder", dir); };
    cubbiEditor->onLog                 = [this](const juce::String& msg) { appendStatus(msg.trimEnd()); };
    cubbiEditor->onBackgroundClicked   = stopPreviewFn;
    addAndMakeVisible(cubbiEditor.get());

    jammiEditor = std::make_unique<BankEditorPanel>(ChompiNamer::Category::Jammi);
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

    bankStatusLabel.setFont(juce::Font(10.0f));
    bankStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff667788));
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
    processButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff1b1722));
    processButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff2b2732));
    processButton.setColour(juce::TextButton::textColourOffId,  juce::Colours::white);
    processButton.onClick = [this] { processFiles(); };
    processButton.setEnabled(false);
    addAndMakeVisible(processButton);

    fillButton.setButtonText("Fill");
    fillButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff1b1722));
    fillButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    fillButton.onClick = [this]
    {
        if (viewMode == ViewMode::Pack)
            getActiveEditor()->autoFillFromFolder({});
        else
            bankFocusPanel->triggerAutoFill();
    };
    addAndMakeVisible(fillButton);

    clearButton.setButtonText("Clear");
    clearButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff1b1722));
    clearButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
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

    // Apply initial mode styling
    styleTabButton(packModeButton, true);
    styleTabButton(bankModeButton, false);
    styleTabButton(cubbiTabButton, true);
    styleTabButton(jammiTabButton, false);

    setWantsKeyboardFocus(true);

    // Register commands so AppMenuBar can use addCommandItem to wire keyboard shortcuts
    commandManager.registerAllCommandsForTarget(this);
    commandManager.setFirstCommandTarget(this);

    setSize(493, 890);

    inspector = std::make_unique<melatonin::Inspector>(*this);
    inspector->setVisible(true);
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

    if (logoImage.isValid())
    {
        auto headerArea = getLocalBounds().reduced(12).removeFromTop(44);
        const float logoAspect = (float)logoImage.getWidth() / (float)logoImage.getHeight();
        const int logoH = headerArea.getHeight();
        const int logoW = juce::roundToInt(logoH * logoAspect);
        auto logoRect = juce::Rectangle<int>(logoW, logoH)
                            .withCentre(headerArea.getCentre());
        g.drawImage(logoImage, logoRect.toFloat());
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(12);

    // ── Blank header space (matches SVG top area) ─────────
    area.removeFromTop(44);

    // ── Nav row: [Cubbi][Jammi]  gap  [Pack][Bank] ────────
    {
        auto navRow = area.removeFromTop(32);
        const int halfW = (navRow.getWidth() - 24) / 2;

        auto leftPair  = navRow.removeFromLeft(halfW);
        navRow.removeFromLeft(24);
        auto rightPair = navRow;

        const int lHalf = leftPair.getWidth() / 2;
        cubbiTabButton.setBounds(leftPair.removeFromLeft(lHalf).reduced(2, 0));
        jammiTabButton.setBounds(leftPair.reduced(2, 0));

        const int rHalf = rightPair.getWidth() / 2;
        packModeButton.setBounds(rightPair.removeFromLeft(rHalf).reduced(2, 0));
        bankModeButton.setBounds(rightPair.reduced(2, 0));
    }
    area.removeFromTop(8);

    // ── Footer: 3 equal buttons [Clear][Fill][Pack] ───────
    auto footer = area.removeFromBottom(54 + 12);
    footer.removeFromTop(12);

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
        clearButton.setBounds(footer.removeFromLeft(bW).reduced(2, 0));
        fillButton.setBounds(footer.removeFromLeft(bW).reduced(2, 0));
        processButton.setBounds(footer.reduced(2, 0));
    }
}

// ─── Mode switching ───────────────────────────────────────────────────────────

void MainComponent::setViewMode(ViewMode mode)
{
    // Sync data when leaving Bank mode
    if (viewMode == ViewMode::Bank && mode != ViewMode::Bank)
        syncBankFocusToAdvanced();
    // Sync data when entering Bank mode
    if (mode == ViewMode::Bank && viewMode != ViewMode::Bank)
        syncPackToBankFocus();

    viewMode = mode;

    styleTabButton(packModeButton, mode == ViewMode::Pack);
    styleTabButton(bankModeButton, mode == ViewMode::Bank);

    const bool isPack = (mode == ViewMode::Pack);
    const bool isBank = (mode == ViewMode::Bank);

    // Category tabs shared between Pack and Bank modes
    cubbiTabButton.setVisible(true);
    jammiTabButton.setVisible(true);
    if (isPack)
    {
        setCategoryTab(showCubbiEditor);
    }
    else  // isBank
    {
        styleTabButton(cubbiTabButton, showCubbiEditor);
        styleTabButton(jammiTabButton, !showCubbiEditor);
        auto cat = showCubbiEditor ? ChompiNamer::Category::Cubbi
                                   : ChompiNamer::Category::Jammi;
        bankFocusPanel->switchToCategory(cat);
    }

    // Bank mode
    bankFocusPanel->setVisible(isBank);
    if (isBank) bankFocusPanel->grabKeyboardFocus();
    bankStatusLabel.setVisible(isBank);

    updateProcessButtonState();
    resized();
}

void MainComponent::syncPackToBankFocus()
{
    bankFocusPanel->clearAll();

    for (const auto& a : cubbiEditor->getAssignments())
    {
        int bankIdx = (int)(a.bankLetter - 'a');
        bankFocusPanel->setSlot(ChompiNamer::Category::Cubbi, bankIdx, a.slotNumber - 1, a.sourceFile);
    }

    for (const auto& a : jammiEditor->getAssignments())
    {
        int bankIdx = (int)(a.bankLetter - 'a');
        bankFocusPanel->setSlot(ChompiNamer::Category::Jammi, bankIdx, a.slotNumber - 1, a.sourceFile);
    }
}

void MainComponent::syncBankFocusToAdvanced()
{
    cubbiEditor->clearAllBanks();
    jammiEditor->clearAllBanks();

    for (const auto& a : bankFocusPanel->getAssignments(ChompiNamer::Category::Cubbi))
    {
        int bankIdx = (int)(a.bankLetter - 'a');
        cubbiEditor->setSlotFile(bankIdx, a.slotNumber - 1, a.sourceFile);
    }

    for (const auto& a : bankFocusPanel->getAssignments(ChompiNamer::Category::Jammi))
    {
        int bankIdx = (int)(a.bankLetter - 'a');
        jammiEditor->setSlotFile(bankIdx, a.slotNumber - 1, a.sourceFile);
    }
}

void MainComponent::setCategoryTab(bool showCubbi)
{
    showCubbiEditor = showCubbi;
    styleTabButton(cubbiTabButton, showCubbi);
    styleTabButton(jammiTabButton, !showCubbi);
    stopPreview();
    if (viewMode == ViewMode::Pack)
    {
        cubbiEditor->clearSelection();
        jammiEditor->clearSelection();
        cubbiEditor->setVisible(showCubbi);
        jammiEditor->setVisible(!showCubbi);
    }
}

BankEditorPanel* MainComponent::getActiveEditor()
{
    return showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();
}

void MainComponent::styleTabButton(juce::TextButton& btn, bool active)
{
    btn.setColour(juce::TextButton::buttonColourId,
                  active ? tabActiveCol : tabInactiveCol);
    btn.setColour(juce::TextButton::textColourOffId, tabTextCol);
    btn.setColour(juce::TextButton::textColourOnId,  headerColour);
}

// ─── Output folder ────────────────────────────────────────────────────────────

juce::File MainComponent::getResolvedOutputFolder()
{
    return outputBaseFolder;
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
    appendStatus("\n=== Starting CHOMPI Processing ===");

    if (viewMode == ViewMode::Bank)
        syncBankFocusToAdvanced();

    processFilesFromEditors();

    processButton.setEnabled(true);
}

void MainComponent::processFilesFromEditors()
{
    auto cubbiAssignments = cubbiEditor->getAssignments();
    auto jammiAssignments = jammiEditor->getAssignments();

    juce::File outputFolder = getResolvedOutputFolder();

    auto result = processor->processFilesFromAssignments(
        cubbiAssignments, jammiAssignments, outputFolder);

    if (result.success)
        appendProcessingResult(result, outputFolder);
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
        ready = (bankFocusPanel->getFilledCount(ChompiNamer::Category::Cubbi)
               + bankFocusPanel->getFilledCount(ChompiNamer::Category::Jammi)) > 0;

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
    // Don't intercept navigation keys when a text editor has keyboard focus
    if (dynamic_cast<juce::TextEditor*>(origin) != nullptr)
        return false;

    if (key == juce::KeyPress::escapeKey)
    {
        if (viewMode == ViewMode::Bank)
            bankFocusPanel->clearSelection();
        else
            getActiveEditor()->clearSelection();
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
            bool newCubbi = !showCubbiEditor;
            setCategoryTab(newCubbi);
            bankFocusPanel->switchToCategory(newCubbi ? ChompiNamer::Category::Cubbi
                                                      : ChompiNamer::Category::Jammi);
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
        setCategoryTab(!showCubbiEditor);
        bankFocusPanel->switchToCategory(showCubbiEditor ? ChompiNamer::Category::Cubbi
                                                         : ChompiNamer::Category::Jammi);
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
        for (int b = 0; b < ChompiNamer::NUM_BANKS; ++b)
            for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
            {
                state.cubbiSlots[b][s] = bankFocusPanel->getSlotFile(ChompiNamer::Category::Cubbi, b, s);
                state.jammiSlots[b][s] = bankFocusPanel->getSlotFile(ChompiNamer::Category::Jammi, b, s);
            }
    }
    else
    {
        for (int b = 0; b < ChompiNamer::NUM_BANKS; ++b)
            for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
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
}

void MainComponent::applyUndoState(const UndoState& state)
{
    isApplyingUndoState = true;

    cubbiEditor->clearAllBanks();
    jammiEditor->clearAllBanks();

    for (int b = 0; b < ChompiNamer::NUM_BANKS; ++b)
        for (int s = 0; s < ChompiNamer::SLOTS_PER_BANK; ++s)
        {
            cubbiEditor->setSlotFile(b, s, state.cubbiSlots[b][s]);
            jammiEditor->setSlotFile(b, s, state.jammiSlots[b][s]);
        }

    if (viewMode == ViewMode::Bank)
        syncPackToBankFocus();

    isApplyingUndoState = false;
    updateProcessButtonState();
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
                        cmdOpenOutput, cmdProcess, cmdToggleConsole });
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
        case cmdProcess:     processFiles();    return true;
        case cmdToggleConsole: toggleConsole(); return true;
        default:               return false;
    }
}
