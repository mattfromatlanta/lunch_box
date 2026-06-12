// SPDX-License-Identifier: AGPL-3.0-or-later
#include "AudioPreviewPlayer.h"

AudioPreviewPlayer::AudioPreviewPlayer()
    : thumbnailCache(64)  // > the 28 thumbnails Bank view keeps alive (14 rows x 2)
{
    formatManager.registerBasicFormats();

    auto err = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (err.isEmpty())
    {
        deviceManager.addAudioCallback(&sourcePlayer);
        sourcePlayer.setSource(&transportSource);
    }
}

AudioPreviewPlayer::~AudioPreviewPlayer()
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    sourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&sourcePlayer);
}

void AudioPreviewPlayer::loadFile(const juce::File& file)
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    readerSource.reset();

    auto* reader = formatManager.createReaderFor(file);
    if (reader != nullptr)
    {
        readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
        sendChangeMessage();
    }
}

void AudioPreviewPlayer::play()
{
    if (readerSource != nullptr)
        transportSource.start();
}

void AudioPreviewPlayer::pause()
{
    transportSource.stop();
}

void AudioPreviewPlayer::stop()
{
    transportSource.stop();
    transportSource.setPosition(0.0);
    sendChangeMessage();
}

bool AudioPreviewPlayer::isPlaying() const
{
    return transportSource.isPlaying();
}

double AudioPreviewPlayer::getCurrentPositionRatio() const
{
    auto duration = transportSource.getLengthInSeconds();
    if (duration <= 0.0) return 0.0;
    return juce::jlimit(0.0, 1.0, transportSource.getCurrentPosition() / duration);
}

double AudioPreviewPlayer::getDurationSeconds() const
{
    return transportSource.getLengthInSeconds();
}

juce::AudioFormatManager& AudioPreviewPlayer::getFormatManager()
{
    return formatManager;
}

juce::AudioThumbnailCache& AudioPreviewPlayer::getThumbnailCache()
{
    return thumbnailCache;
}
