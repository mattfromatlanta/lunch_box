#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

//==============================================================================
// AudioPreviewPlayer - Manages audio device and playback transport
//==============================================================================

class AudioPreviewPlayer : public juce::ChangeBroadcaster
{
public:
    AudioPreviewPlayer();
    ~AudioPreviewPlayer() override;

    void loadFile(const juce::File& file);

    void play();
    void pause();
    void stop();

    bool isPlaying() const;
    double getCurrentPositionRatio() const;  // 0.0 – 1.0
    double getDurationSeconds() const;

    juce::AudioFormatManager& getFormatManager();
    juce::AudioThumbnailCache& getThumbnailCache();

private:
    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer sourcePlayer;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPreviewPlayer)
};
