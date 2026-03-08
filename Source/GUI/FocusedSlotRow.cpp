#include "FocusedSlotRow.h"
#include "../FileSystemHelper.h"

namespace
{
    const juce::Colour rowEmptyBg      { 0xff1d2228 };
    const juce::Colour rowFilledBg     { 0xff1e3a52 };
    const juce::Colour rowSelectedBg   { 0xff1e2a4a };
    const juce::Colour rowBorderCol    { 0xff2a3040 };
    const juce::Colour rowHoverBdr     { 0xff5577aa };
    const juce::Colour rowDropBdr      { 0xff4caf50 };
    const juce::Colour rowSelectedBdr  { 0xff4455aa };
    const juce::Colour slotNumCol      { 0xff4a5a6a };
    const juce::Colour waveColour      { 0xff3d8a5c };
    const juce::Colour filenameCol     { 0xff8899aa };
    const juce::Colour insertionCol    { 0xffddaa33 };
}

FocusedSlotRow::FocusedSlotRow(int slot,
                                juce::AudioFormatManager& fmt,
                                juce::AudioThumbnailCache& cache)
    : slotNumber(slot),
      thumbnail(512, fmt, cache),
      previewThumbnail(512, fmt, cache)
{
    thumbnail.addChangeListener(this);
    previewThumbnail.addChangeListener(this);
}

FocusedSlotRow::~FocusedSlotRow()
{
    thumbnail.removeChangeListener(this);
    previewThumbnail.removeChangeListener(this);
}

void FocusedSlotRow::setSample(const juce::File& file)
{
    sample = file;
    if (file != juce::File{})
        thumbnail.setSource(new juce::FileInputSource(file));
    else
        thumbnail.clear();
    repaint();
    if (onSampleChanged) onSampleChanged(this);
}

void FocusedSlotRow::clearSample()
{
    sample = juce::File{};
    thumbnail.clear();
    repaint();
    if (onSampleChanged) onSampleChanged(this);
}

void FocusedSlotRow::setSelected(bool s)
{
    if (selected != s) { selected = s; repaint(); }
}

void FocusedSlotRow::setDragSource(bool s)
{
    if (isDragSrc != s) { isDragSrc = s; repaint(); }
}


void FocusedSlotRow::setPreviewSample(const juce::File& f)
{
    previewFile = f;
    if (f != juce::File{})
        previewThumbnail.setSource(new juce::FileInputSource(f));
    else
        previewThumbnail.clear();
    showingPreview = true;
    repaint();
}

void FocusedSlotRow::clearPreviewSample()
{
    if (!showingPreview) return;
    showingPreview = false;
    previewFile    = juce::File{};
    previewThumbnail.clear();
    repaint();
}

void FocusedSlotRow::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Resolve display content — preview overrides actual state during drag
    const juce::File&     displayFile  = showingPreview ? previewFile  : sample;
    const bool            displayFilled = (displayFile != juce::File{});
    juce::AudioThumbnail& displayThumb  = showingPreview ? previewThumbnail : thumbnail;

    // Background
    juce::Colour bg;
    if      (selected) bg = displayFilled ? rowFilledBg.brighter(0.15f) : rowSelectedBg;
    else               bg = displayFilled ? rowFilledBg : rowEmptyBg;
    g.setColour(bg);
    g.fillRect(bounds);

    // Bottom border separator
    g.setColour(rowBorderCol);
    g.drawLine(0.0f, (float)bounds.getBottom() - 1.0f,
               (float)bounds.getWidth(), (float)bounds.getBottom() - 1.0f, 1.0f);

    // Border priority: drag-source → file-drop → selected → hovered
    if (isDragSrc)
    {
        g.setColour(juce::Colour(0xffdd7722));
        g.drawRect(bounds, 2);
    }
    else if (isDraggingOver)
    {
        g.setColour(rowDropBdr);
        g.drawRect(bounds, 2);
    }
    else if (selected)
    {
        g.setColour(rowSelectedBdr);
        g.drawRect(bounds, 1);
    }
    else if (isHovered)
    {
        g.setColour(rowHoverBdr.withAlpha(0.5f));
        g.drawRect(bounds, 1);
    }

    // Slot number (left column, 28px)
    const int numW = 28;
    auto numArea = bounds.removeFromLeft(numW);
    g.setColour(slotNumCol);
    g.setFont(juce::Font(16.0f));
    g.drawText(juce::String(slotNumber).paddedLeft('0', 2),
               numArea, juce::Justification::centred);

    if (!displayFilled)
        return;

    // Waveform (slightly dimmed when showing preview content)
    if (displayThumb.getTotalLength() > 0.0)
    {
        g.setColour(waveColour.withAlpha(showingPreview ? 0.5f : 0.75f));
        displayThumb.drawChannels(g, bounds.reduced(0, 4),
                                  0.0, displayThumb.getTotalLength(), 1.0f);
    }

    // Filename overlay — right-aligned, drawn on top of waveform
    g.setColour(filenameCol.withAlpha(showingPreview ? 0.6f : 0.9f));
    g.setFont(juce::Font(9.5f));
    g.drawText(displayFile.getFileNameWithoutExtension(),
               bounds.reduced(4, 0),
               juce::Justification::centredRight, true);
}

void FocusedSlotRow::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) { showContextMenu(); return; }
    if (onRowMouseDown) onRowMouseDown(this, e);
}

void FocusedSlotRow::mouseDrag(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;
    if (onRowMouseDrag) onRowMouseDrag(this, e);
}

void FocusedSlotRow::mouseUp(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;
    if (onRowMouseUp) onRowMouseUp(this, e);
}

void FocusedSlotRow::mouseDoubleClick(const juce::MouseEvent&)
{
    if (onRowDoubleClicked) onRowDoubleClicked(this);
}

void FocusedSlotRow::mouseEnter(const juce::MouseEvent&)
{
    isHovered = true;
    repaint();
}

void FocusedSlotRow::mouseExit(const juce::MouseEvent&)
{
    isHovered = false;
    repaint();
}

bool FocusedSlotRow::isSupportedAudioFile(const juce::String& path)
{
    auto ext = "*" + juce::File(path).getFileExtension().toLowerCase();
    return FileSystemHelper::getSupportedAudioExtensions().contains(ext);
}

bool FocusedSlotRow::isInterestedInFileDrag(const juce::StringArray& files)
{
    return files.size() == 1 && isSupportedAudioFile(files[0]);
}

void FocusedSlotRow::filesDropped(const juce::StringArray& files, int, int)
{
    isDraggingOver = false;
    if (files.size() == 1)
    {
        juce::File f(files[0]);
        if (f.existsAsFile()) setSample(f);
    }
    repaint();
}

void FocusedSlotRow::fileDragEnter(const juce::StringArray&, int, int)
{
    isDraggingOver = true;
    repaint();
}

void FocusedSlotRow::fileDragExit(const juce::StringArray&)
{
    isDraggingOver = false;
    repaint();
}

void FocusedSlotRow::browseForFile()
{
    juce::File startDir = getStartDirectory
                              ? getStartDirectory()
                              : juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Audio File", startDir, "*.wav;*.aiff;*.aif;*.mp3;*.flac", true);

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            auto f = chooser.getResult();
            if (f != juce::File{})
            {
                if (onFolderBrowsed) onFolderBrowsed(f.getParentDirectory());
                setSample(f);
            }
        });
}

void FocusedSlotRow::showContextMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Browse for File...");
    if (hasSample())
    {
        menu.addSeparator();
        menu.addItem(2, "Preview");
        menu.addSeparator();
        menu.addItem(3, "Clear Slot");
    }

    menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this),
        [this](int result)
        {
            if      (result == 1) browseForFile();
            else if (result == 2) { if (onSlotClicked) onSlotClicked(this); }
            else if (result == 3) clearSample();
        });
}

void FocusedSlotRow::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint();
}
