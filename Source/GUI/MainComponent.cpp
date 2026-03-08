#include "MainComponent.h"

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

static void styleSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(11.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, sectionColour);
}

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
    cubbiEditor->onAssignmentsChanged  = [this] { updateProcessButtonState(); };
    cubbiEditor->onSlotClicked         = packSlotClicked;
    cubbiEditor->onPreviewStop         = [this] { stopPreview(); };
    cubbiEditor->getStartDirectory     = [this]() -> juce::File { return getSavedFolder("lastCubbiFolder"); };
    cubbiEditor->onFolderBrowsed       = [this](juce::File dir)  { saveFolder("lastCubbiFolder", dir); };
    cubbiEditor->onLog                 = [this](const juce::String& msg) { appendStatus(msg.trimEnd()); };
    cubbiEditor->onBackgroundClicked   = stopPreviewFn;
    addAndMakeVisible(cubbiEditor.get());

    jammiEditor = std::make_unique<BankEditorPanel>(ChompiNamer::Category::Jammi);
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

    styleSectionLabel(outputSectionLabel, "OUTPUT FOLDER");
    addAndMakeVisible(outputSectionLabel);

    // Default base: ~/Desktop, default name: "chompis"
    outputBaseFolder = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);

    outputParentButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff1e2838));
    outputParentButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8899aa));
    outputParentButton.onClick = [this] { selectOutputFolder(); };
    addAndMakeVisible(outputParentButton);

    outputSlashLabel.setText("/", juce::dontSendNotification);
    outputSlashLabel.setFont(juce::Font(13.0f));
    outputSlashLabel.setColour(juce::Label::textColourId, juce::Colour(0xff556677));
    outputSlashLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputSlashLabel);

    outputNameEditor.setFont(juce::Font(13.0f));
    outputNameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1e2838));
    outputNameEditor.setColour(juce::TextEditor::textColourId,       juce::Colour(0xffccddee));
    outputNameEditor.setColour(juce::TextEditor::outlineColourId,    juce::Colour(0xff3a4a5a));
    outputNameEditor.setText("chompis", juce::dontSendNotification);
    outputNameEditor.onTextChange = [this]
    {
        // Strip disallowed characters (only alphanumeric, _, -, / permitted)
        auto text = outputNameEditor.getText();
        juce::String cleaned;
        for (int i = 0; i < text.length(); ++i)
        {
            juce::juce_wchar c = text[i];
            if (juce::CharacterFunctions::isLetterOrDigit(c) || c == '_' || c == '-' || c == '/')
                cleaned += c;
        }
        if (cleaned != text)
        {
            int pos = outputNameEditor.getCaretPosition();
            outputNameEditor.setText(cleaned, false);  // false = don't re-trigger
            outputNameEditor.setCaretPosition(juce::jmin(pos, cleaned.length()));
        }
        updateOutputPathDisplay();
        saveString("lastOutputName", outputNameEditor.getText());
    };
    addAndMakeVisible(outputNameEditor);

    outputPathLabel.setFont(juce::Font(11.0f));
    outputPathLabel.setColour(juce::Label::textColourId, juce::Colour(0xff667788));
    addAndMakeVisible(outputPathLabel);

    cleanOutputToggle.setButtonText("Clean folder before export?");
    cleanOutputToggle.setToggleState(true, juce::dontSendNotification);
    cleanOutputToggle.setColour(juce::ToggleButton::textColourId,    juce::Colour(0xff8899aa));
    cleanOutputToggle.setColour(juce::ToggleButton::tickColourId,    juce::Colour(0xff4caf50));
    cleanOutputToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff3a4a5a));
    addAndMakeVisible(cleanOutputToggle);

    // Restore persisted output state (overrides defaults if saved)
    {
        auto savedParent = getSavedFolder("lastOutputParent");
        if (savedParent != juce::File{})
            outputBaseFolder = savedParent;

        auto savedName = getSavedString("lastOutputName");
        if (savedName.isNotEmpty())
            outputNameEditor.setText(savedName, juce::dontSendNotification);
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

    setSize(1118, 700);
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
    const int navH          = 26;

    // ── Shared header + nav row ───────────────────────────
    headerLabel.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);

    // Single nav row: [Cubbi][Jammi]  20px gap  [Pack][Bank]
    {
        auto row = area.removeFromTop(navH);
        const int modeW = 52, catW = 58;

        cubbiTabButton.setBounds(row.removeFromLeft(catW).reduced(1, 0));
        jammiTabButton.setBounds(row.removeFromLeft(catW).reduced(1, 0));
        row.removeFromLeft(20);
        packModeButton.setBounds(row.removeFromLeft(modeW).reduced(1, 0));
        bankModeButton.setBounds(row.removeFromLeft(modeW).reduced(1, 0));
    }
    area.removeFromTop(10);

    // ── Reserve footer from bottom ────────────────────────
    // Footer: fill/clear (26px) + gap + output section + buttons [+ bank status label]
    const int fillClearH  = 26;
    const int outputH     = 26 + 4 + 20 + sectionGap;  // path label removed; ~58px
    const int buttonsH    = 32;
    const int bankStatusH = 6 + 16;  // always reserved; label hidden in Pack mode
    const int footerH     = fillClearH + itemGap + outputH + buttonsH + bankStatusH;

    auto footer = area.removeFromBottom(footerH);

    if (viewMode == ViewMode::Pack)
    {
        // ── Pack mode: editor fills remaining space ────────
        const int editorH = 385;  // 5 × 77px (ROW_HEIGHT, no gaps)
        auto editorBounds = area.removeFromTop(editorH);
        cubbiEditor->setBounds(editorBounds);
        jammiEditor->setBounds(editorBounds);
    }
    else  // ViewMode::Bank
    {
        // ── Bank mode: focus panel fills remaining space ───
        cubbiEditor->setBounds({});
        jammiEditor->setBounds({});
        bankFocusPanel->setBounds(area.removeFromTop(385));  // 7 × 55px (ROW_HEIGHT)
    }

    // ── Shared footer ─────────────────────────────────────
    {
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

    outputBaseFolder = folder.getParentDirectory();
    outputNameEditor.setText(folder.getFileName(), juce::dontSendNotification);
    saveFolder("lastOutputParent", outputBaseFolder);
    saveString("lastOutputName",   folder.getFileName());
    updateOutputPathDisplay();
    appendStatus("Output folder selected: " + getResolvedOutputFolder().getFullPathName());
}

juce::File MainComponent::getResolvedOutputFolder()
{
    auto name = outputNameEditor.getText().trim();
    if (name.isEmpty()) name = "chompis";
    return outputBaseFolder.getChildFile(name);
}

void MainComponent::updateOutputPathDisplay()
{
    auto resolved = getResolvedOutputFolder();
    outputPathLabel.setText(resolved.getFullPathName(), juce::dontSendNotification);
    outputParentButton.setButtonText(outputBaseFolder.getFullPathName());
}

void MainComponent::layoutButtonRow(juce::Rectangle<int>& area, int h)
{
    auto row = area.removeFromTop(h);
    const int procW = 200, openW = 110, gap = 8;
    auto pair = row.withSizeKeepingCentre(procW + gap + openW, h);
    processButton.setBounds(pair.removeFromLeft(procW));
    pair.removeFromLeft(gap);
    openOutputButton.setBounds(pair);
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
    // Row: [OUTPUT FOLDER label] [parent button] [/] [name editor]
    auto row = area.removeFromTop(26);
    const int labelW = 100;
    const int nameW  = 540;
    const int slashW = 16;
    outputSectionLabel.setBounds(row.removeFromLeft(labelW));
    outputNameEditor.setBounds(row.removeFromRight(nameW));
    outputSlashLabel.setBounds(row.removeFromRight(slashW));
    outputParentButton.setBounds(row);

    area.removeFromTop(4);
    cleanOutputToggle.setBounds(area.removeFromTop(20));
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
        stopPreview();
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
         || key == juce::KeyPress::backspaceKey) { bankFocusPanel->clearFocusedRows(); return true; }
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

    if (key == juce::KeyPress::tabKey)      { ed->tabFocus();        return true; }

    if (key == juce::KeyPress::deleteKey
     || key == juce::KeyPress::backspaceKey) { ed->clearSelectedCells(); return true; }

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
