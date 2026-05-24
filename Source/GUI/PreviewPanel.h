// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "AudioPreviewPlayer.h"
#include "WaveformDisplay.h"

//==============================================================================
// PreviewPanel - Waveform display + transport controls for sample auditioning
//==============================================================================

class PreviewPanel : public juce::Component,
                     private juce::ChangeListener
{
public:
    PreviewPanel();
    ~PreviewPanel() override;

    // Load an audio file for display only (no autoplay)
    void loadFile(const juce::File& file);

    // Load and immediately start playback (always restarts from beginning)
    void playFile(const juce::File& file);

    // Stop playback without unloading
    void stopPlayback();

    bool isPlaying() const;
    juce::File getCurrentFile() const { return currentFile; }

    // Access shared audio resources (used by BankFocusPanel for per-slot thumbnails)
    juce::AudioFormatManager& getFormatManager();
    juce::AudioThumbnailCache& getThumbnailCache();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    AudioPreviewPlayer player;
    WaveformDisplay waveformDisplay;

    juce::TextButton playPauseButton;
    juce::TextButton stopButton;
    juce::Label fileInfoLabel;

    juce::File currentFile;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void updatePlayPauseButton();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreviewPanel)
};
