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
    headerLabel.setText("chompi pack  |  by matt from atlanta", juce::dontSendNotification);
    headerLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    headerLabel.setJustificationType(juce::Justification::centred);
    headerLabel.setColour(juce::Label::textColourId, headerColour);
    addAndMakeVisible(headerLabel);

    // Mode toggle buttons
    simpleModeButton.setButtonText("Simple");
    advancedModeButton.setButtonText("Advanced");
    simpleModeButton.onClick   = [this] { setMode(false); };
    advancedModeButton.onClick = [this] { setMode(true); };
    addAndMakeVisible(simpleModeButton);
    addAndMakeVisible(advancedModeButton);

    // ── Simple mode components ─────────────────────────────

    styleSectionLabel(cubbiSectionLabel,   "CUBBI SAMPLES");
    styleSectionLabel(jammiSectionLabel,   "JAMMI SAMPLES");
    styleSectionLabel(previewSectionLabel, "");
    addAndMakeVisible(cubbiSectionLabel);
    addAndMakeVisible(jammiSectionLabel);

    cubbiDropZone = std::make_unique<FolderDropZone>("Select Cubbi Folder...",
                                                      "no cubbi folder selected");
    cubbiDropZone->onButtonClicked  = [this] { selectCubbiFolder(); };
    cubbiDropZone->onFolderSelected = [this](juce::File f) { handleCubbiFolderSelected(f); };
    addAndMakeVisible(cubbiDropZone.get());

    jammiDropZone = std::make_unique<FolderDropZone>("Select Jammi Folder...",
                                                      "no jammi folder selected");
    jammiDropZone->onButtonClicked  = [this] { selectJammiFolder(); };
    jammiDropZone->onFolderSelected = [this](juce::File f) { handleJammiFolderSelected(f); };
    addAndMakeVisible(jammiDropZone.get());

    // ── Advanced mode components ───────────────────────────

    cubbiTabButton.setButtonText("Cubbi");
    jammiTabButton.setButtonText("Jammi");
    cubbiTabButton.onClick = [this] { setCategoryTab(true); };
    jammiTabButton.onClick = [this] { setCategoryTab(false); };
    addAndMakeVisible(cubbiTabButton);
    addAndMakeVisible(jammiTabButton);

    auto advancedSlotClicked = [this](BankEditorPanel::Cell cell, const juce::File& f)
    {
        playingCell = cell;
        previewPanel.playFile(f);
    };

    auto stopPreviewFn = [this] { stopPreview(); };

    cubbiEditor = std::make_unique<BankEditorPanel>(ChompiNamer::Category::Cubbi);
    cubbiEditor->onAssignmentsChanged  = [this] { updateProcessButtonState(); };
    cubbiEditor->onSlotClicked         = advancedSlotClicked;
    cubbiEditor->getStartDirectory     = [this]() -> juce::File { return getSavedFolder("lastCubbiFolder"); };
    cubbiEditor->onFolderBrowsed       = [this](juce::File dir)  { saveFolder("lastCubbiFolder", dir); };
    cubbiEditor->onLog                 = [this](const juce::String& msg) { appendStatus(msg.trimEnd()); };
    cubbiEditor->onBackgroundClicked   = stopPreviewFn;
    addAndMakeVisible(cubbiEditor.get());

    jammiEditor = std::make_unique<BankEditorPanel>(ChompiNamer::Category::Jammi);
    jammiEditor->onAssignmentsChanged  = [this] { updateProcessButtonState(); };
    jammiEditor->onSlotClicked         = advancedSlotClicked;
    jammiEditor->getStartDirectory     = [this]() -> juce::File { return getSavedFolder("lastJammiFolder"); };
    jammiEditor->onFolderBrowsed       = [this](juce::File dir)  { saveFolder("lastJammiFolder", dir); };
    jammiEditor->onLog                 = [this](const juce::String& msg) { appendStatus(msg.trimEnd()); };
    jammiEditor->onBackgroundClicked   = stopPreviewFn;
    addChildComponent(jammiEditor.get());  // hidden initially

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

    statusTextEditor.setMultiLine(true);
    statusTextEditor.setReadOnly(true);
    statusTextEditor.setScrollbarsShown(true);
    statusTextEditor.setCaretVisible(false);
    statusTextEditor.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    statusTextEditor.setColour(juce::TextEditor::backgroundColourId, statusBgColour);
    statusTextEditor.setColour(juce::TextEditor::textColourId,       statusFgColour);
    statusTextEditor.setColour(juce::TextEditor::outlineColourId,    juce::Colour(0xff2a3a4a));
    statusTextEditor.setText("Ready to process samples...");
    addAndMakeVisible(statusTextEditor);

    processor = std::make_unique<GuiProcessor>();
    addChildComponent(previewPanel);  // kept but hidden — will be re-introduced later

    // Apply initial mode styling
    styleTabButton(simpleModeButton,   true);
    styleTabButton(advancedModeButton, false);
    styleTabButton(cubbiTabButton,     true);
    styleTabButton(jammiTabButton,     false);

    // Advanced mode controls are hidden initially
    cubbiTabButton.setVisible(false);
    jammiTabButton.setVisible(false);
    cubbiEditor->setVisible(false);
    jammiEditor->setVisible(false);

    setWantsKeyboardFocus(true);

    setSize(600, 740);
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
    const int zoneH         = 62;
    const int sectionGap    = 14;
    const int itemGap       = 6;
    const int modeToggleH   = 28;
    const int tabH          = 28;

    // Header
    headerLabel.setBounds(area.removeFromTop(44));
    area.removeFromTop(sectionGap);

    // Mode toggle
    {
        auto row = area.removeFromTop(modeToggleH);
        int btnW = row.getWidth() / 2;
        simpleModeButton.setBounds(row.removeFromLeft(btnW).reduced(2, 0));
        advancedModeButton.setBounds(row.reduced(2, 0));
    }

    if (!isAdvancedMode)
    {
        // ── Simple mode layout ─────────────────────────────
        area.removeFromTop(sectionGap);

        cubbiSectionLabel.setBounds(area.removeFromTop(sectionLabelH));
        area.removeFromTop(itemGap);
        cubbiDropZone->setBounds(area.removeFromTop(zoneH));
        area.removeFromTop(sectionGap);

        jammiSectionLabel.setBounds(area.removeFromTop(sectionLabelH));
        area.removeFromTop(itemGap);
        jammiDropZone->setBounds(area.removeFromTop(zoneH));
        area.removeFromTop(sectionGap);

        layoutOutputSection(area, sectionLabelH, itemGap, sectionGap);

        layoutButtonRow(area, 32);
        area.removeFromTop(sectionGap);

        statusTextEditor.setBounds(area);
    }
    else
    {
        // ── Advanced mode layout ───────────────────────────
        area.removeFromTop(10);

        // Category tabs
        {
            auto row = area.removeFromTop(tabH);
            int btnW = row.getWidth() / 2;
            cubbiTabButton.setBounds(row.removeFromLeft(btnW).reduced(2, 0));
            jammiTabButton.setBounds(row.reduced(2, 0));
        }
        area.removeFromTop(8);

        // Bank editor (only the visible one is laid out; both share the same rect)
        const int editorH = 312;  // 5 × (52+4) + 4 + 28
        auto editorBounds = area.removeFromTop(editorH);
        cubbiEditor->setBounds(editorBounds);
        jammiEditor->setBounds(editorBounds);
        area.removeFromTop(sectionGap);

        layoutOutputSection(area, sectionLabelH, itemGap, sectionGap);

        layoutButtonRow(area, 32);
        area.removeFromTop(10);

        statusTextEditor.setBounds(area);
    }
}

// ─── Mode switching ───────────────────────────────────────────────────────────

void MainComponent::setMode(bool advanced)
{
    isAdvancedMode = advanced;

    styleTabButton(simpleModeButton,   !advanced);
    styleTabButton(advancedModeButton,  advanced);

    // Simple mode visibility
    cubbiSectionLabel.setVisible(!advanced);
    jammiSectionLabel.setVisible(!advanced);
    cubbiDropZone->setVisible(!advanced);
    jammiDropZone->setVisible(!advanced);

    // Advanced mode visibility
    cubbiTabButton.setVisible(advanced);
    jammiTabButton.setVisible(advanced);
    if (advanced)
        setCategoryTab(showCubbiEditor);
    else
    {
        cubbiEditor->setVisible(false);
        jammiEditor->setVisible(false);
    }

    updateProcessButtonState();
    resized();
}

void MainComponent::setCategoryTab(bool showCubbi)
{
    showCubbiEditor = showCubbi;
    styleTabButton(cubbiTabButton, showCubbi);
    styleTabButton(jammiTabButton, !showCubbi);
    // Clear selection on the editor we're leaving
    cubbiEditor->clearSelection();
    jammiEditor->clearSelection();
    stopPreview();
    cubbiEditor->setVisible(showCubbi);
    jammiEditor->setVisible(!showCubbi);
}

void MainComponent::styleTabButton(juce::TextButton& btn, bool active)
{
    btn.setColour(juce::TextButton::buttonColourId,
                  active ? tabActiveCol : tabInactiveCol);
    btn.setColour(juce::TextButton::textColourOffId, tabTextCol);
    btn.setColour(juce::TextButton::textColourOnId,  headerColour);
}

// ─── Unified folder handlers ──────────────────────────────────────────────────

void MainComponent::handleCubbiFolderSelected(juce::File folder)
{
    if (!folder.isDirectory()) return;

    selectedCubbiFolder = folder;
    cubbiDropZone->setSelectedFolder(folder);

    int count = countAudioFiles(folder);
    if (count == 0)
        appendStatus("Warning: Cubbi folder contains no supported audio files");
    else
        appendStatus("Cubbi folder selected: " + juce::String(count) + " audio files found");

    saveFolder("lastCubbiFolder", folder);
    updateProcessButtonState();
}

void MainComponent::handleJammiFolderSelected(juce::File folder)
{
    if (!folder.isDirectory()) return;

    selectedJammiFolder = folder;
    jammiDropZone->setSelectedFolder(folder);

    int count = countAudioFiles(folder);
    if (count == 0)
        appendStatus("Warning: Jammi folder contains no supported audio files");
    else
        appendStatus("Jammi folder selected: " + juce::String(count) + " audio files found");

    saveFolder("lastJammiFolder", folder);
    updateProcessButtonState();
}

void MainComponent::handleOutputFolderSelected(juce::File folder)
{
    if (!folder.isDirectory()) return;

    // Decompose: parent becomes the base, folder name populates the editor
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
    // Actual directory creation handled by processor via ensureDirectoryExists
}

void MainComponent::layoutOutputSection(juce::Rectangle<int>& area, int labelH, int itemGap, int sectionGap)
{
    outputSectionLabel.setBounds(area.removeFromTop(labelH));
    area.removeFromTop(itemGap);

    // Row: [parent button] [/] [name editor]
    auto row = area.removeFromTop(26);
    const int nameW  = 180;
    const int slashW = 16;
    outputNameEditor.setBounds(row.removeFromRight(nameW));
    outputSlashLabel.setBounds(row.removeFromRight(slashW));
    outputParentButton.setBounds(row);

    area.removeFromTop(4);
    outputPathLabel.setBounds(area.removeFromTop(14));
    area.removeFromTop(4);
    cleanOutputToggle.setBounds(area.removeFromTop(20));
    area.removeFromTop(sectionGap);
}

// ─── File browser launchers ───────────────────────────────────────────────────

void MainComponent::selectFolderFor(const juce::String& title,
                                    const juce::File& startDir,
                                    std::function<void(juce::File)> handler)
{
    auto start = (startDir != juce::File{} && startDir.isDirectory())
                     ? startDir
                     : juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    fileChooser = std::make_unique<juce::FileChooser>(title, start, "", true);

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(flags, [handler](const juce::FileChooser& chooser) {
        auto f = chooser.getResult();
        if (f != juce::File{}) handler(f);
    });
}

void MainComponent::selectCubbiFolder()
{
    selectFolderFor("Select Cubbi Folder",
                    getSavedFolder("lastCubbiFolder"),
                    [this](juce::File f) { handleCubbiFolderSelected(f); });
}

void MainComponent::selectJammiFolder()
{
    selectFolderFor("Select Jammi Folder",
                    getSavedFolder("lastJammiFolder"),
                    [this](juce::File f) { handleJammiFolderSelected(f); });
}

void MainComponent::selectOutputFolder()
{
    // Open chooser starting at current resolved folder (or saved parent)
    auto startDir = getResolvedOutputFolder();
    if (!startDir.exists())
        startDir = outputBaseFolder;

    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Output Folder",
        startDir,
        "", true);

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

    if (isAdvancedMode)
    {
        processFilesAdvanced();
    }
    else
    {
        juce::File outputFolder = getResolvedOutputFolder();
        prepareOutputFolder(outputFolder);

        auto result = processor->processFiles(selectedCubbiFolder, selectedJammiFolder, outputFolder);

        if (result.success)
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
        else
        {
            appendStatus("\n=== Processing Failed ===");
            appendStatus("Error: " + result.message);
        }
    }

    processButton.setEnabled(true);
}

void MainComponent::processFilesAdvanced()
{
    auto cubbiAssignments = cubbiEditor->getAssignments();
    auto jammiAssignments = jammiEditor->getAssignments();

    juce::File outputFolder = getResolvedOutputFolder();
    prepareOutputFolder(outputFolder);

    auto result = processor->processFilesFromAssignments(
        cubbiAssignments, jammiAssignments, outputFolder);

    if (result.success)
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
    else
    {
        appendStatus("\n=== Processing Failed ===");
        appendStatus("Error: " + result.message);
    }
}

void MainComponent::updateProcessButtonState()
{
    bool ready = false;
    if (isAdvancedMode)
        ready = (cubbiEditor->getFilledCount() + jammiEditor->getFilledCount()) > 0;
    else
        ready = (selectedCubbiFolder != juce::File{}) || (selectedJammiFolder != juce::File{});

    processButton.setEnabled(ready);
}

void MainComponent::appendStatus(const juce::String& message)
{
    statusTextEditor.moveCaretToEnd();
    statusTextEditor.insertTextAtCaret(message + "\n");
    statusTextEditor.moveCaretToEnd();
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

    if (key == juce::KeyPress::spaceKey)
    {
        if (isAdvancedMode)
        {
            auto* ed = showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();
            if (previewPanel.isPlaying())
            {
                // Snapshot before stopPreview clears playingCell
                auto wasPlaying = playingCell;
                stopPreview();
                // Different cell focused → interrupt + immediately play it
                if (ed->selectionSize() == 1 && ed->getFocusCell() != wasPlaying)
                    ed->playFocused();
                // Same cell (or invalid) → just stop; nothing more to do
            }
            else
            {
                ed->playFocused();
            }
        }
        else
        {
            stopPreview();
        }
        return true;
    }

    if (!isAdvancedMode) return false;

    auto* ed = showCubbiEditor ? cubbiEditor.get() : jammiEditor.get();

    // Shift+Arrow: expand rectangular selection (no-op at grid border)
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
        // Don't intercept if a button has focus — let it handle its own click
        if (dynamic_cast<juce::Button*>(origin) != nullptr) return false;
        ed->browseForFocused();
        return true;
    }

    return false;
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
    statusTextEditor.clear();
    statusTextEditor.setText("Status log cleared.");
}

int MainComponent::countAudioFiles(const juce::File& folder)
{
    juce::Array<juce::File> files;
    for (const auto& pattern : FileSystemHelper::getSupportedAudioExtensions())
        folder.findChildFiles(files, juce::File::findFiles, true, pattern);
    return files.size();
}

void MainComponent::previewFirstAudioFile(const juce::File& folder)
{
    juce::Array<juce::File> files;
    for (const auto& pattern : FileSystemHelper::getSupportedAudioExtensions())
    {
        folder.findChildFiles(files, juce::File::findFiles, true, pattern);
        if (!files.isEmpty()) break;
    }

    if (!files.isEmpty())
    {
        files.sort();
        previewPanel.loadFile(files[0]);
    }
}
