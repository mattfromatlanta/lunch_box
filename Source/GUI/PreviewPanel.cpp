// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PreviewPanel.h"

namespace
{
    const juce::Colour panelBg    { 0xff1a1f2e };
    const juce::Colour accentGreen { 0xff4caf50 };
    const juce::Colour btnBg      { 0xff2a3a4a };
    const juce::Colour mutedText  { 0xff667788 };
}

PreviewPanel::PreviewPanel()
    : waveformDisplay(player),
      playPauseButton("Play"),
      stopButton("Stop")
{
    addAndMakeVisible(waveformDisplay);

    playPauseButton.setColour(juce::TextButton::buttonColourId,  accentGreen);
    playPauseButton.setColour(juce::TextButton::buttonOnColourId, accentGreen.darker(0.2f));
    playPauseButton.setColour(juce::TextButton::textColourOffId,  juce::Colours::white);
    playPauseButton.onClick = [this]
    {
        if (player.isPlaying())
        {
            player.pause();
        }
        else
        {
            player.play();
            waveformDisplay.startPositionTracking();
        }
        updatePlayPauseButton();
    };
    playPauseButton.setEnabled(false);
    addAndMakeVisible(playPauseButton);

    stopButton.setColour(juce::TextButton::buttonColourId,  btnBg);
    stopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    stopButton.onClick = [this]
    {
        player.stop();
        updatePlayPauseButton();
    };
    stopButton.setEnabled(false);
    addAndMakeVisible(stopButton);

    fileInfoLabel.setFont(juce::Font(11.0f));
    fileInfoLabel.setColour(juce::Label::textColourId, mutedText);
    fileInfoLabel.setText("PREVIEW", juce::dontSendNotification);
    addAndMakeVisible(fileInfoLabel);

    player.addChangeListener(this);
}

PreviewPanel::~PreviewPanel()
{
    player.removeChangeListener(this);
}

void PreviewPanel::loadFile(const juce::File& file)
{
    if (!file.existsAsFile()) return;

    // Stop previous playback before loading
    if (player.isPlaying())
        player.stop();

    currentFile = file;
    player.loadFile(file);
    waveformDisplay.loadFile(file);

    // Update info label: filename + duration
    double duration = player.getDurationSeconds();
    int mins = (int)(duration / 60.0);
    int secs = (int)(duration) % 60;
    juce::String info = "PREVIEW  |  " + file.getFileNameWithoutExtension()
                      + "  (" + juce::String::formatted("%d:%02d", mins, secs) + ")";
    fileInfoLabel.setText(info, juce::dontSendNotification);

    playPauseButton.setEnabled(true);
    stopButton.setEnabled(true);
    updatePlayPauseButton();
}

void PreviewPanel::paint(juce::Graphics& g)
{
    g.fillAll(panelBg);
}

void PreviewPanel::resized()
{
    auto area = getLocalBounds().reduced(8, 6);

    // File info label at top
    fileInfoLabel.setBounds(area.removeFromTop(16));
    area.removeFromTop(4);

    // Transport buttons strip at bottom
    auto controls = area.removeFromBottom(26);
    playPauseButton.setBounds(controls.removeFromLeft(56));
    controls.removeFromLeft(4);
    stopButton.setBounds(controls.removeFromLeft(40));

    area.removeFromBottom(4);

    // Waveform fills remaining space
    waveformDisplay.setBounds(area);
}

void PreviewPanel::playFile(const juce::File& file)
{
    loadFile(file);
    player.play();
    waveformDisplay.startPositionTracking();
    updatePlayPauseButton();
}

void PreviewPanel::stopPlayback()
{
    player.stop();
    updatePlayPauseButton();
}

juce::AudioFormatManager& PreviewPanel::getFormatManager()  { return player.getFormatManager(); }
juce::AudioThumbnailCache& PreviewPanel::getThumbnailCache() { return player.getThumbnailCache(); }

bool PreviewPanel::isPlaying() const
{
    return player.isPlaying();
}

void PreviewPanel::changeListenerCallback(juce::ChangeBroadcaster*)
{
    updatePlayPauseButton();
    repaint();
}

void PreviewPanel::updatePlayPauseButton()
{
    playPauseButton.setButtonText(player.isPlaying() ? "Pause" : "Play");
}
