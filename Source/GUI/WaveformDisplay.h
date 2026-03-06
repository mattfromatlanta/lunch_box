#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "AudioPreviewPlayer.h"

//==============================================================================
// WaveformDisplay - Renders an audio waveform thumbnail with playback cursor
//==============================================================================

class WaveformDisplay : public juce::Component,
                        private juce::ChangeListener,
                        private juce::Timer
{
public:
    explicit WaveformDisplay(AudioPreviewPlayer& player);
    ~WaveformDisplay() override;

    void loadFile(const juce::File& file);
    void clear();
    void startPositionTracking();  // Call after resuming playback

    void paint(juce::Graphics& g) override;

private:
    AudioPreviewPlayer& player;
    juce::AudioThumbnail thumbnail;
    bool fileLoaded = false;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
