#include "WaveformDisplay.h"

namespace
{
    const juce::Colour waveBg      { 0xff151a26 };
    const juce::Colour waveColour  { 0xff4caf50 };
    const juce::Colour cursorColour{ 0xccffffff };
    const juce::Colour emptyText   { 0xff3a4a5a };
}

WaveformDisplay::WaveformDisplay(AudioPreviewPlayer& p)
    : player(p),
      thumbnail(512, player.getFormatManager(), player.getThumbnailCache())
{
    thumbnail.addChangeListener(this);
}

WaveformDisplay::~WaveformDisplay()
{
    thumbnail.removeChangeListener(this);
    stopTimer();
}

void WaveformDisplay::loadFile(const juce::File& file)
{
    thumbnail.setSource(new juce::FileInputSource(file));
    fileLoaded = true;
    startTimerHz(30);
    repaint();
}

void WaveformDisplay::startPositionTracking()
{
    if (fileLoaded)
        startTimerHz(30);
}

void WaveformDisplay::clear()
{
    stopTimer();
    thumbnail.clear();
    fileLoaded = false;
    repaint();
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(waveBg);
    g.fillRoundedRectangle(bounds, 4.0f);

    if (!fileLoaded || thumbnail.getTotalLength() <= 0.0)
    {
        g.setColour(emptyText);
        g.setFont(11.0f);
        g.drawText("select a folder to preview", bounds, juce::Justification::centred);
        return;
    }

    // Waveform
    g.setColour(waveColour);
    thumbnail.drawChannels(g, getLocalBounds().reduced(2, 4),
                           0.0, thumbnail.getTotalLength(), 1.0f);

    // Playback cursor
    double posRatio = player.getCurrentPositionRatio();
    if (posRatio > 0.0)
    {
        float posX = (float)(bounds.getWidth() * posRatio);
        g.setColour(cursorColour);
        g.drawLine(posX, 0.0f, posX, bounds.getHeight(), 1.5f);
    }
}

void WaveformDisplay::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint();
}

void WaveformDisplay::timerCallback()
{
    if (player.isPlaying())
        repaint();
    else
        stopTimer();
}
