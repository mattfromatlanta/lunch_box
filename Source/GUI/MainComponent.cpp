#include "MainComponent.h"
#include "ClipboardHelper.h"

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

    // Header
    headerLabel.setText("chompi pack", juce::dontSendNotification);
    headerLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    headerLabel.setJustificationType(juce::Justification::centredLeft);
    headerLabel.setColour(juce::Label::textColourId, headerColour);
    addAndMakeVisible(headerLabel);

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

    auto packSlotClicked = [this](BankEditorPanel::Cell cell, const juce::File& f)
    {
        playingCell = cell;
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

    outputParentButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff1e2838));
    outputParentButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8899aa));
    outputParentButton.onClick = [this] { selectOutputFolder(); };
    addAndMakeVisible(outputParentButton);

    cleanOutputToggle.setButtonText("Clean folder?");
    cleanOutputToggle.setToggleState(false, juce::dontSendNotification);
    cleanOutputToggle.setColour(juce::ToggleButton::textColourId,    juce::Colour(0xff8899aa));
    cleanOutputToggle.setColour(juce::ToggleButton::tickColourId,    juce::Colour(0xff4caf50));
    cleanOutputToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff3a4a5a));
    addAndMakeVisible(cleanOutputToggle);

    // Restore persisted output folder
    {
        auto saved = getSavedFolder("lastOutputParent");
        if (saved != juce::File{})
            outputBaseFolder = saved;
    }

    updateOutputPathDisplay();

    processButton.setButtonText("Process Samples");
    processButton.setColour(juce::TextButton::buttonColourId,  accentColour);
    processButton.setColour(juce::TextButton::buttonOnColourId, accentColour.darker(0.2f));
    processButton.setColour(juce::TextButton::textColourOffId,  juce::Colours::white);
    processButton.onClick = [this] { processFiles(); };
    processButton.setEnabled(false);
    addAndMakeVisible(processButton);

    openOutputButton.setButtonText("Open Output");
    openOutputButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff2a4060));
    openOutputButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a5070));
    openOutputButton.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xffaabbcc));
    openOutputButton.onClick = [this] { getResolvedOutputFolder().startAsProcess(); };
    openOutputButton.setEnabled(false);
    addAndMakeVisible(openOutputButton);

    fillButton.setButtonText("Fill from folder...");
    fillButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff2a3a4a));
    fillButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffaabbcc));
    fillButton.onClick = [this]
    {
        if (viewMode == ViewMode::Pack)
        {
            auto* ed = showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();
            ed->autoFillFromFolder({});
        }
        else
        {
            bankFocusPanel->triggerAutoFill();
        }
    };
    addAndMakeVisible(fillButton);

    clearButton.setButtonText("Clear All");
    clearButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff2a3a4a));
    clearButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffaabbcc));
    clearButton.onClick = [this]
    {
        captureUndoState();
        if (viewMode == ViewMode::Pack)
        {
            auto* ed = showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();
            ed->clearAllBanks();
        }
        else
        {
            bankFocusPanel->triggerClear();
        }
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

    setSize(525, 900);
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
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(20);
    const int sectionLabelH = 18;
    const int sectionGap    = 8;
    const int itemGap       = 6;

    // ── Single header + nav row ───────────────────────────
    {
        auto row = area.removeFromTop(28);
        const int modeW = 52, catW = 58;

        // Buttons right-aligned: [Cubbi][Jammi]  gap  [Pack][Bank]
        bankModeButton.setBounds(row.removeFromRight(modeW).reduced(1, 0));
        packModeButton.setBounds(row.removeFromRight(modeW).reduced(1, 0));
        row.removeFromRight(16);
        jammiTabButton.setBounds(row.removeFromRight(catW).reduced(1, 0));
        cubbiTabButton.setBounds(row.removeFromRight(catW).reduced(1, 0));
        row.removeFromRight(16);

        // Header label takes remaining left space
        headerLabel.setBounds(row);
    }
    area.removeFromTop(10);

    // ── Reserve footer from bottom ────────────────────────
    // Footer: fill/clear (26px) + gap + output section + buttons [+ bank status label]
    const int footerTopPad = 8;
    const int fillClearH  = 26;
    const int outputH     = 26 + sectionGap;  // button row + gap
    const int buttonsH    = 32;
    const int bankStatusH = 6 + 16;  // always reserved; label hidden in Pack mode
    const int footerH     = footerTopPad + fillClearH + itemGap + outputH + buttonsH + bankStatusH;

    auto footer = area.removeFromBottom(footerH);

    if (viewMode == ViewMode::Pack)
    {
        // Pack mode: editor fills remaining space
        cubbiEditor->setBounds(area);
        jammiEditor->setBounds(area);
    }
    else  // ViewMode::Bank
    {
        // Bank mode: focus panel fills remaining space
        cubbiEditor->setBounds({});
        jammiEditor->setBounds({});
        bankFocusPanel->setBounds(area);
    }

    // ── Shared footer ─────────────────────────────────────
    {
        footer.removeFromTop(footerTopPad);

        // Fill / Clear row
        auto fillRow = footer.removeFromTop(fillClearH);
        const int btnW = fillRow.getWidth() / 2;
        fillButton.setBounds(fillRow.removeFromLeft(btnW).reduced(2, 0));
        clearButton.setBounds(fillRow.reduced(2, 0));

        footer.removeFromTop(itemGap);

        // Output folder section
        layoutOutputSection(footer, sectionLabelH, itemGap, sectionGap);

        // Process / Open buttons
        layoutButtonRow(footer, buttonsH);

        // Bank status label
        if (viewMode == ViewMode::Bank)
        {
            footer.removeFromTop(6);
            bankStatusLabel.setBounds(footer.removeFromTop(16));
        }
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

    // Update footer button labels for current mode
    clearButton.setButtonText(isBank ? "Clear Bank" : "Clear All");

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

void MainComponent::styleTabButton(juce::TextButton& btn, bool active)
{
    btn.setColour(juce::TextButton::buttonColourId,
                  active ? tabActiveCol : tabInactiveCol);
    btn.setColour(juce::TextButton::textColourOffId, tabTextCol);
    btn.setColour(juce::TextButton::textColourOnId,  headerColour);
}

// ─── Output folder ────────────────────────────────────────────────────────────

void MainComponent::handleOutputFolderSelected(juce::File folder)
{
    if (!folder.isDirectory()) return;

    outputBaseFolder = folder;
    saveFolder("lastOutputParent", outputBaseFolder);
    updateOutputPathDisplay();
    appendStatus("Output folder selected: " + outputBaseFolder.getFullPathName());
}

juce::File MainComponent::getResolvedOutputFolder()
{
    return outputBaseFolder;
}

void MainComponent::updateOutputPathDisplay()
{
    // Collect path segments from deepest to root
    juce::StringArray parts;
    auto f = outputBaseFolder;
    for (;;)
    {
        auto name = f.getFileName();
        if (name.isEmpty()) break;
        parts.insert(0, name);
        auto parent = f.getParentDirectory();
        if (parent == f) break;
        f = parent;
    }

    // Keep last 3 segments, prefix with ".." if truncated
    const bool truncated = parts.size() > 3;
    juce::StringArray display;
    const int start = truncated ? parts.size() - 3 : 0;
    for (int i = start; i < parts.size(); ++i)
        display.add(parts[i]);

    outputParentButton.setButtonText((truncated ? "../" : "") + display.joinIntoString("/"));
}

void MainComponent::layoutButtonRow(juce::Rectangle<int>& area, int h)
{
    auto row = area.removeFromTop(h);
    const int procW = 200, openW = 110, cleanW = 160, gap = 8;
    cleanOutputToggle.setBounds(row.removeFromLeft(cleanW).reduced(0, 3));
    auto buttons = row.removeFromRight(procW + gap + openW);
    processButton.setBounds(buttons.removeFromLeft(procW));
    buttons.removeFromLeft(gap);
    openOutputButton.setBounds(buttons);
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

void MainComponent::prepareOutputFolder(const juce::File& folder)
{
    if (cleanOutputToggle.getToggleState() && folder.exists())
    {
        folder.deleteRecursively();
        appendStatus("Cleaned: " + folder.getFullPathName());
    }
}

void MainComponent::layoutOutputSection(juce::Rectangle<int>& area, int /*labelH*/, int /*itemGap*/, int sectionGap)
{
    // Row: output button fills full width
    outputParentButton.setBounds(area.removeFromTop(26));
    area.removeFromTop(sectionGap);
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
        if (f != juce::File{})
            handleOutputFolderSelected(f);
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
    prepareOutputFolder(outputFolder);

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
    openOutputButton.setEnabled(true);
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
    playingCell = { -1, -1 };
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
            (showCubbiEditor ? cubbiEditor.get() : jammiEditor.get())->clearSelection();
        return true;
    }

    if (key == juce::KeyPress::spaceKey)
    {
        if (viewMode == ViewMode::Pack)
        {
            auto* ed = showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();
            stopPreview();
            ed->playFocused();
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
    auto* ed = showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();

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
        sampleClipboard = (showCubbiEditor ? cubbiEditor.get() : jammiEditor.get())->getSelectedFiles();

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
        auto* ed = showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();
        sampleClipboard = ed->getSelectedFiles();
        ed->clearSelectedCells();
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
        (showCubbiEditor ? cubbiEditor.get() : jammiEditor.get())->pasteFiles(files);
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
                (showCubbiEditor ? cubbiEditor.get() : jammiEditor.get())->selectAll();
            return true;
        }
        case cmdOpenOutput:  getResolvedOutputFolder().startAsProcess(); return true;
        case cmdProcess:     processFiles();    return true;
        case cmdToggleConsole: toggleConsole(); return true;
        default:               return false;
    }
}
