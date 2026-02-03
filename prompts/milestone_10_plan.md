# Milestone 10: Sample Waveform and Play Preview

## Objective
Add visual waveform display and audio playback preview for samples in the GUI, enabling users to audition samples before processing.

## Requirements

### Waveform Display
- Visual representation of audio amplitude over time
- Thumbnail waveforms in bank slots (Milestone 8 integration)
- Full waveform in preview pane
- Zoom and pan controls
- Selection/loop region display

### Audio Playback
- Play button for each sample
- Playback controls (play, pause, stop)
- Volume control
- Loop playback option
- Keyboard shortcuts (spacebar = play/pause)

### Integration Points
- **Simple Mode:** Preview selected folder's first file
- **Advanced Mode:** Preview any bank slot sample
- **Status Area:** Playback progress indicator

## Technical Architecture

### JUCE Audio Components

```cpp
class WaveformDisplay : public juce::Component,
                       private juce::ChangeListener
{
public:
    void loadAudioFile(const juce::File& file);
    void paint(juce::Graphics& g) override;
    void setZoomFactor(double zoom);

private:
    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
};

class AudioPreviewPlayer : public juce::AudioAppComponent
{
public:
    void loadFile(const juce::File& file);
    void play();
    void pause();
    void stop();
    double getCurrentPosition() const;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

private:
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
};
```

## Implementation Steps

### Phase 1: Waveform Display
1. Create WaveformDisplay component using AudioThumbnail
2. Implement thumbnail generation and caching
3. Add zoom/pan controls
4. Integrate with bank slots (small thumbnails)

### Phase 2: Audio Playback
1. Create AudioPreviewPlayer using AudioTransportSource
2. Implement play/pause/stop controls
3. Add position/time display
4. Add volume control

### Phase 3: Preview Panel
1. Create dedicated preview panel in GUI
2. Show waveform + playback controls
3. Display file metadata (format, duration, sample rate)
4. Auto-preview on slot selection

### Phase 4: Integration
1. Connect to bank slot selection
2. Connect to folder validation (preview first file)
3. Add keyboard shortcuts
4. Add right-click preview option

## Success Criteria
- [ ] Waveform displays correctly for all audio formats
- [ ] Playback works for all audio formats
- [ ] Play/pause/stop controls functional
- [ ] Thumbnail generation doesn't block UI
- [ ] Caching improves performance
- [ ] Volume control works
- [ ] Keyboard shortcuts work
- [ ] Preview auto-updates on selection

## Estimated Impact
- Code: ~600 lines (WaveformDisplay ~200, AudioPreviewPlayer ~200, integration ~200)
- User Benefit: Very High (essential for sample management)
