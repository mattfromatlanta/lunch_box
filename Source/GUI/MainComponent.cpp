#include "MainComponent.h"

MainComponent::MainComponent()
{
    // Set window size (increased to accommodate output folder section)
    setSize(600, 580);

    // Setup header label
    headerLabel.setText("chompi pack by matt from atlanta", juce::dontSendNotification);
    headerLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    headerLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(headerLabel);

    // Setup cubbi folder button
    selectCubbiButton.setButtonText("Select Cubbi Folder...");
    selectCubbiButton.onClick = [this] { selectCubbiFolder(); };
    addAndMakeVisible(selectCubbiButton);

    // Setup cubbi path label
    cubbiPathLabel.setText("no cubbi folder selected", juce::dontSendNotification);
    cubbiPathLabel.setFont(juce::Font(14.0f));
    cubbiPathLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(cubbiPathLabel);

    // Setup jammi folder button
    selectJammiButton.setButtonText("Select Jammi Folder...");
    selectJammiButton.onClick = [this] { selectJammiFolder(); };
    addAndMakeVisible(selectJammiButton);

    // Setup jammi path label
    jammiPathLabel.setText("no jammi folder selected", juce::dontSendNotification);
    jammiPathLabel.setFont(juce::Font(14.0f));
    jammiPathLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(jammiPathLabel);

    // Setup output folder button
    selectOutputButton.setButtonText("Select Output Folder...");
    selectOutputButton.onClick = [this] { selectOutputFolder(); };
    addAndMakeVisible(selectOutputButton);

    // Setup output path label (default to 'converted' folder)
    outputPathLabel.setText("default: converted/", juce::dontSendNotification);
    outputPathLabel.setFont(juce::Font(14.0f));
    outputPathLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(outputPathLabel);

    // Setup process button
    processButton.setButtonText("Process Samples");
    processButton.onClick = [this] { processFiles(); };
    processButton.setEnabled(false);  // Disabled until folder selected
    addAndMakeVisible(processButton);

    // Setup status text editor
    statusTextEditor.setMultiLine(true);
    statusTextEditor.setReadOnly(true);
    statusTextEditor.setScrollbarsShown(true);
    statusTextEditor.setCaretVisible(false);
    statusTextEditor.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    statusTextEditor.setText("Ready to process samples...");
    addAndMakeVisible(statusTextEditor);

    // Create processor
    processor = std::make_unique<GuiProcessor>([this](const juce::String& message) {
        appendStatus(message);
    });
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(20);

    // Header
    headerLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(20);  // Spacing

    // Cubbi section
    auto cubbiArea = area.removeFromTop(60);
    selectCubbiButton.setBounds(cubbiArea.removeFromTop(30).removeFromLeft(200));
    cubbiArea.removeFromTop(5);  // Small spacing
    cubbiPathLabel.setBounds(cubbiArea);
    area.removeFromTop(20);  // Spacing

    // Jammi section
    auto jammiArea = area.removeFromTop(60);
    selectJammiButton.setBounds(jammiArea.removeFromTop(30).removeFromLeft(200));
    jammiArea.removeFromTop(5);  // Small spacing
    jammiPathLabel.setBounds(jammiArea);
    area.removeFromTop(20);  // Spacing

    // Output section
    auto outputArea = area.removeFromTop(60);
    selectOutputButton.setBounds(outputArea.removeFromTop(30).removeFromLeft(200));
    outputArea.removeFromTop(5);  // Small spacing
    outputPathLabel.setBounds(outputArea);
    area.removeFromTop(20);  // Spacing

    // Process button (centered)
    auto processArea = area.removeFromTop(40);
    auto buttonWidth = 200;
    auto buttonX = (processArea.getWidth() - buttonWidth) / 2;
    processButton.setBounds(processArea.getX() + buttonX, processArea.getY(), buttonWidth, 30);
    area.removeFromTop(20);  // Spacing

    // Status text editor (remaining space)
    statusTextEditor.setBounds(area);
}

void MainComponent::selectCubbiFolder()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Cubbi Folder",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "",
        true);

    auto folderChooserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(folderChooserFlags, [this](const juce::FileChooser& chooser)
    {
        auto folder = chooser.getResult();
        if (folder != juce::File{})
        {
            selectedCubbiFolder = folder;
            cubbiPathLabel.setText(folder.getFullPathName(), juce::dontSendNotification);
            cubbiPathLabel.setColour(juce::Label::textColourId, juce::Colours::white);

            // Check if folder contains WAV files
            juce::Array<juce::File> wavFiles;
            folder.findChildFiles(wavFiles, juce::File::findFiles, true, "*.wav");

            if (wavFiles.isEmpty())
            {
                appendStatus("Warning: Cubbi folder contains no WAV files");
                cubbiPathLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
            }
            else
            {
                appendStatus("Cubbi folder selected: " + juce::String(wavFiles.size()) + " WAV files found");
            }

            updateProcessButtonState();
        }
    });
}

void MainComponent::selectJammiFolder()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Jammi Folder",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "",
        true);

    auto folderChooserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(folderChooserFlags, [this](const juce::FileChooser& chooser)
    {
        auto folder = chooser.getResult();
        if (folder != juce::File{})
        {
            selectedJammiFolder = folder;
            jammiPathLabel.setText(folder.getFullPathName(), juce::dontSendNotification);
            jammiPathLabel.setColour(juce::Label::textColourId, juce::Colours::white);

            // Check if folder contains WAV files
            juce::Array<juce::File> wavFiles;
            folder.findChildFiles(wavFiles, juce::File::findFiles, true, "*.wav");

            if (wavFiles.isEmpty())
            {
                appendStatus("Warning: Jammi folder contains no WAV files");
                jammiPathLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
            }
            else
            {
                appendStatus("Jammi folder selected: " + juce::String(wavFiles.size()) + " WAV files found");
            }

            updateProcessButtonState();
        }
    });
}

void MainComponent::selectOutputFolder()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Output Folder",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "",
        true);

    auto folderChooserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(folderChooserFlags, [this](const juce::FileChooser& chooser)
    {
        auto folder = chooser.getResult();
        if (folder != juce::File{})
        {
            selectedOutputFolder = folder;
            outputPathLabel.setText(folder.getFullPathName(), juce::dontSendNotification);
            outputPathLabel.setColour(juce::Label::textColourId, juce::Colours::white);
            appendStatus("Output folder selected: " + folder.getFullPathName());
        }
    });
}

void MainComponent::processFiles()
{
    // Disable process button during processing
    processButton.setEnabled(false);
    appendStatus("\n=== Starting CHOMPI Processing ===");

    // Determine output folder (use selected folder or default to 'converted')
    juce::File outputFolder = (selectedOutputFolder != juce::File{})
        ? selectedOutputFolder
        : juce::File::getCurrentWorkingDirectory().getChildFile("converted");

    // Process files
    auto result = processor->processFiles(
        selectedCubbiFolder,
        selectedJammiFolder,
        outputFolder
    );

    // Show results
    if (result.success)
    {
        appendStatus("\n=== Processing Complete ===");
        appendStatus("Total files processed: " + juce::String(result.cubbiFilesProcessed + result.jammiFilesProcessed));
        appendStatus("  Cubbi: " + juce::String(result.cubbiFilesProcessed));
        appendStatus("  Jammi: " + juce::String(result.jammiFilesProcessed));
        appendStatus("Output: " + outputFolder.getFullPathName());

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Processing Complete",
            "Successfully processed " + juce::String(result.cubbiFilesProcessed + result.jammiFilesProcessed) + " samples!",
            "OK"
        );
    }
    else
    {
        appendStatus("\n=== Processing Failed ===");
        appendStatus("Error: " + result.message);

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Processing Failed",
            result.message,
            "OK"
        );
    }

    // Re-enable process button
    processButton.setEnabled(true);
}

void MainComponent::updateProcessButtonState()
{
    // Enable process button if at least one folder is selected
    bool hasValidFolder = (selectedCubbiFolder != juce::File{}) || (selectedJammiFolder != juce::File{});
    processButton.setEnabled(hasValidFolder);
}

void MainComponent::appendStatus(const juce::String& message)
{
    statusTextEditor.moveCaretToEnd();
    statusTextEditor.insertTextAtCaret(message + "\n");
    statusTextEditor.moveCaretToEnd();
}
