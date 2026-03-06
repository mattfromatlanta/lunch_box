#include "MainComponent.h"

// M9 color palette
namespace
{
    const juce::Colour bgColour        { 0xff1a1f2e };  // dark navy
    const juce::Colour sectionColour   { 0xff8899aa };  // muted blue-grey
    const juce::Colour headerColour    { 0xffe0e8f0 };  // near-white
    const juce::Colour statusBgColour  { 0xff151a26 };  // darker than bg
    const juce::Colour statusFgColour  { 0xffaabbcc };
    const juce::Colour accentColour    { 0xff4caf50 };  // green for process button
}

static void styleSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(11.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, sectionColour);
}

MainComponent::MainComponent()
{
    // Header
    headerLabel.setText("chompi pack  |  by matt from atlanta", juce::dontSendNotification);
    headerLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    headerLabel.setJustificationType(juce::Justification::centred);
    headerLabel.setColour(juce::Label::textColourId, headerColour);
    addAndMakeVisible(headerLabel);

    // Section labels
    styleSectionLabel(cubbiSectionLabel,  "CUBBI SAMPLES");
    styleSectionLabel(jammiSectionLabel,  "JAMMI SAMPLES");
    styleSectionLabel(outputSectionLabel, "OUTPUT FOLDER");
    addAndMakeVisible(cubbiSectionLabel);
    addAndMakeVisible(jammiSectionLabel);
    addAndMakeVisible(outputSectionLabel);

    // Cubbi drop zone
    cubbiDropZone = std::make_unique<FolderDropZone>("Select Cubbi Folder...",
                                                      "no cubbi folder selected");
    cubbiDropZone->onButtonClicked  = [this] { selectCubbiFolder(); };
    cubbiDropZone->onFolderSelected = [this](juce::File f) { handleCubbiFolderSelected(f); };
    addAndMakeVisible(cubbiDropZone.get());

    // Jammi drop zone
    jammiDropZone = std::make_unique<FolderDropZone>("Select Jammi Folder...",
                                                      "no jammi folder selected");
    jammiDropZone->onButtonClicked  = [this] { selectJammiFolder(); };
    jammiDropZone->onFolderSelected = [this](juce::File f) { handleJammiFolderSelected(f); };
    addAndMakeVisible(jammiDropZone.get());

    // Output drop zone
    outputDropZone = std::make_unique<FolderDropZone>("Select Output Folder...",
                                                       "default: converted/");
    outputDropZone->onButtonClicked  = [this] { selectOutputFolder(); };
    outputDropZone->onFolderSelected = [this](juce::File f) { handleOutputFolderSelected(f); };
    addAndMakeVisible(outputDropZone.get());

    // Process button (styled with accent color)
    processButton.setButtonText("Process Samples");
    processButton.setColour(juce::TextButton::buttonColourId,  accentColour);
    processButton.setColour(juce::TextButton::buttonOnColourId, accentColour.darker(0.2f));
    processButton.setColour(juce::TextButton::textColourOffId,  juce::Colours::white);
    processButton.onClick = [this] { processFiles(); };
    processButton.setEnabled(false);
    addAndMakeVisible(processButton);

    // Status text editor
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

    // Processing bridge
    processor = std::make_unique<GuiProcessor>([this](const juce::String& message) {
        appendStatus(message);
    });

    // Set size last — triggers resized(), all components must exist by this point
    setSize(600, 620);
}

MainComponent::~MainComponent() {}

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

    // Header
    headerLabel.setBounds(area.removeFromTop(44));
    area.removeFromTop(sectionGap);

    // Cubbi
    cubbiSectionLabel.setBounds(area.removeFromTop(sectionLabelH));
    area.removeFromTop(itemGap);
    cubbiDropZone->setBounds(area.removeFromTop(zoneH));
    area.removeFromTop(sectionGap);

    // Jammi
    jammiSectionLabel.setBounds(area.removeFromTop(sectionLabelH));
    area.removeFromTop(itemGap);
    jammiDropZone->setBounds(area.removeFromTop(zoneH));
    area.removeFromTop(sectionGap);

    // Output
    outputSectionLabel.setBounds(area.removeFromTop(sectionLabelH));
    area.removeFromTop(itemGap);
    outputDropZone->setBounds(area.removeFromTop(zoneH));
    area.removeFromTop(sectionGap);

    // Process button (centered)
    auto btnArea = area.removeFromTop(36);
    processButton.setBounds(btnArea.withSizeKeepingCentre(200, 32));
    area.removeFromTop(sectionGap);

    // Status (remaining space)
    statusTextEditor.setBounds(area);
}

// ─── Unified folder handlers ─────────────────────────────────────────────────

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

    updateProcessButtonState();
}

void MainComponent::handleOutputFolderSelected(juce::File folder)
{
    if (!folder.isDirectory()) return;

    selectedOutputFolder = folder;
    outputDropZone->setSelectedFolder(folder);
    appendStatus("Output folder selected: " + folder.getFullPathName());
}

// ─── File browser launchers ───────────────────────────────────────────────────

void MainComponent::selectCubbiFolder()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Cubbi Folder",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "", true);

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
        auto f = chooser.getResult();
        if (f != juce::File{}) handleCubbiFolderSelected(f);
    });
}

void MainComponent::selectJammiFolder()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Jammi Folder",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "", true);

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
        auto f = chooser.getResult();
        if (f != juce::File{}) handleJammiFolderSelected(f);
    });
}

void MainComponent::selectOutputFolder()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Output Folder",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "", true);

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
        auto f = chooser.getResult();
        if (f != juce::File{}) handleOutputFolderSelected(f);
    });
}

// ─── Processing ───────────────────────────────────────────────────────────────

void MainComponent::processFiles()
{
    processButton.setEnabled(false);
    appendStatus("\n=== Starting CHOMPI Processing ===");

    juce::File outputFolder = (selectedOutputFolder != juce::File{})
        ? selectedOutputFolder
        : juce::File::getCurrentWorkingDirectory().getChildFile("converted");

    auto result = processor->processFiles(selectedCubbiFolder, selectedJammiFolder, outputFolder);

    if (result.success)
    {
        appendStatus("\n=== Processing Complete ===");
        appendStatus("Total files: " + juce::String(result.cubbiFilesProcessed + result.jammiFilesProcessed));
        appendStatus("  Cubbi: " + juce::String(result.cubbiFilesProcessed));
        appendStatus("  Jammi: " + juce::String(result.jammiFilesProcessed));
        appendStatus("Output:  " + outputFolder.getFullPathName());

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Processing Complete",
            "Successfully processed " +
                juce::String(result.cubbiFilesProcessed + result.jammiFilesProcessed) +
                " samples!\n\nOutput: " + outputFolder.getFullPathName(),
            "OK");
    }
    else
    {
        appendStatus("\n=== Processing Failed ===");
        appendStatus("Error: " + result.message);

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Processing Failed",
            result.message,
            "OK");
    }

    processButton.setEnabled(true);
}

void MainComponent::updateProcessButtonState()
{
    bool ready = (selectedCubbiFolder != juce::File{}) || (selectedJammiFolder != juce::File{});
    processButton.setEnabled(ready);
}

void MainComponent::appendStatus(const juce::String& message)
{
    statusTextEditor.moveCaretToEnd();
    statusTextEditor.insertTextAtCaret(message + "\n");
    statusTextEditor.moveCaretToEnd();
}

int MainComponent::countAudioFiles(const juce::File& folder)
{
    juce::Array<juce::File> files;
    for (const auto& pattern : juce::StringArray{"*.wav", "*.aiff", "*.aif", "*.mp3", "*.flac"})
        folder.findChildFiles(files, juce::File::findFiles, true, pattern);
    return files.size();
}
