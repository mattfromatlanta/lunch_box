#include "BankSlotComponent.h"
#include "../FileSystemHelper.h"

namespace
{
    const juce::Colour slotEmptyBg       { 0xff1d2228 };
    const juce::Colour slotFilledBg      { 0xff3a5060 };
    const juce::Colour slotBorder        { 0xff3a4a5a };
    const juce::Colour slotHoverBdr      { 0xff5577aa };
    const juce::Colour slotDropBdr       { 0xff4caf50 };
    const juce::Colour slotNumCol        { 0xff4a5a6a };
    const juce::Colour slotTxtCol        { 0xffaabbcc };
    const juce::Colour slotSelectedBg    { 0xff1e2a4a };
    const juce::Colour slotFocusBdr      { 0xff99aaff };
    const juce::Colour slotSelectedBdr   { 0xff4455aa };
    const juce::Colour slotDragTargetBg  { 0xff2d2518 };
    const juce::Colour slotDragTargetBdr { 0xffddaa33 };
    const juce::Colour slotSwapSourceBg  { 0xff2a1e08 };   // warm-gold tint for swap-mode sources
    const juce::Colour slotSwapSourceBdr { 0xff997733 };
}

BankSlotComponent::BankSlotComponent(char bank, int slot)
    : bankLetter(bank >= 'a' ? (char)(bank - ('a' - 'A')) : bank)
    , slotNumber(slot)
{
}

void BankSlotComponent::setSample(const juce::File& file)
{
    sample = file;
    repaint();
    if (onSampleChanged) onSampleChanged(this);
}

void BankSlotComponent::clearSample()
{
    sample = juce::File{};
    repaint();
    if (onSampleChanged) onSampleChanged(this);
}

void BankSlotComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(1).toFloat();

    // For display purposes: use preview file during drag previews, else actual sample
    const juce::File& displayFile   = showingPreview ? previewFile : sample;
    const bool        displayFilled = (displayFile != juce::File{});

    juce::Colour bg;
    if      (dragTarget)                 bg = displayFilled ? slotFilledBg.brighter(0.25f) : slotDragTargetBg;
    else if (swapHighlight && selected)  bg = displayFilled ? slotSwapSourceBg.brighter(0.1f) : slotSwapSourceBg;
    else if (selected)                   bg = displayFilled ? slotFilledBg.brighter(0.15f) : slotSelectedBg;
    else                                 bg = displayFilled ? slotFilledBg                  : slotEmptyBg;
    g.setColour(bg);
    g.fillRoundedRectangle(bounds, 3.0f);

    juce::Colour border;
    float bw = 1.0f;
    if      (isDraggingOver)            { border = slotDropBdr;       bw = 2.0f; }
    else if (dragTarget)                { border = slotDragTargetBdr; bw = 2.0f; }
    else if (focused)                   { border = slotFocusBdr;      bw = 2.0f; }
    else if (swapHighlight && selected) { border = slotSwapSourceBdr; }
    else if (selected)                  { border = slotSelectedBdr; }
    else if (isHovered)                 { border = slotHoverBdr; }
    else                                { border = slotBorder; }
    g.setColour(border);
    g.drawRoundedRectangle(bounds, 3.0f, bw);

    // Always show cell label (e.g. "A1", "E14") at 16pt
    const juce::String cellLabel = juce::String::charToString((juce::juce_wchar)bankLetter)
                                   + juce::String(slotNumber);
    g.setColour(displayFilled ? slotTxtCol.withAlpha(0.55f) : slotNumCol);
    g.setFont(juce::Font(16.0f));
    g.drawText(cellLabel, getLocalBounds(), juce::Justification::centred);
}

void BankSlotComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) { showContextMenu(); return; }
    if (onSlotMouseDown) onSlotMouseDown(this, e);
}

void BankSlotComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;
    if (onSlotMouseDrag) onSlotMouseDrag(this, e);
}

void BankSlotComponent::mouseUp(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;
    if (onSlotMouseUp) onSlotMouseUp(this, e);
}

void BankSlotComponent::mouseDoubleClick(const juce::MouseEvent&)
{
    if (onSlotDoubleClicked) onSlotDoubleClicked(this);
}

void BankSlotComponent::setSelected(bool s)
{
    if (selected != s) { selected = s; repaint(); }
}

void BankSlotComponent::setFocused(bool f)
{
    if (focused != f) { focused = f; repaint(); }
}

void BankSlotComponent::setDragTarget(bool t)
{
    if (dragTarget != t) { dragTarget = t; repaint(); }
}

void BankSlotComponent::setPreviewSample(const juce::File& f)
{
    previewFile    = f;
    showingPreview = true;
    repaint();
}

void BankSlotComponent::clearPreviewSample()
{
    if (!showingPreview) return;
    showingPreview = false;
    previewFile    = juce::File{};
    repaint();
}

void BankSlotComponent::setSwapHighlight(bool s)
{
    if (swapHighlight != s) { swapHighlight = s; repaint(); }
}

void BankSlotComponent::mouseEnter(const juce::MouseEvent&)
{
    isHovered = true;
    repaint();
}

void BankSlotComponent::mouseExit(const juce::MouseEvent&)
{
    isHovered = false;
    repaint();
}

bool BankSlotComponent::isSupportedAudioFile(const juce::String& path)
{
    auto ext = "*" + juce::File(path).getFileExtension().toLowerCase();
    return FileSystemHelper::getSupportedAudioExtensions().contains(ext);
}

bool BankSlotComponent::isInterestedInFileDrag(const juce::StringArray&)
{
    return false;  // Panel handles all external file drops
}

void BankSlotComponent::filesDropped(const juce::StringArray& files, int, int)
{
    isDraggingOver = false;
    if (files.size() == 1)
    {
        juce::File f(files[0]);
        if (f.existsAsFile())
            setSample(f);
    }
    repaint();
}

void BankSlotComponent::fileDragEnter(const juce::StringArray&, int, int)
{
    isDraggingOver = true;
    repaint();
}

void BankSlotComponent::fileDragExit(const juce::StringArray&)
{
    isDraggingOver = false;
    repaint();
}

void BankSlotComponent::browseForFile()
{
    if (onBeforeChange) onBeforeChange();

    juce::File startDir = (getStartDirectory)
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

// ─── Internal drag-and-drop (slot reordering) ────────────────────────────────

bool BankSlotComponent::isInterestedInDragSource(const SourceDetails& details)
{
    auto* source = dynamic_cast<BankSlotComponent*>(details.sourceComponent.get());
    return source != nullptr && source != this && source->hasSample();
}

void BankSlotComponent::itemDragEnter(const SourceDetails&)
{
    isDraggingOver = true;
    repaint();
}

void BankSlotComponent::itemDragExit(const SourceDetails&)
{
    isDraggingOver = false;
    repaint();
}

void BankSlotComponent::itemDropped(const SourceDetails& details)
{
    auto* source = dynamic_cast<BankSlotComponent*>(details.sourceComponent.get());
    if (!source) return;

    auto myFile     = getSample();
    auto sourceFile = source->getSample();

    setSample(sourceFile);

    if (myFile != juce::File{})
        source->setSample(myFile);   // swap
    else
        source->clearSample();       // move

    isDraggingOver = false;
    repaint();
}

// ─── Context menu ─────────────────────────────────────────────────────────────

void BankSlotComponent::showContextMenu()
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
            if (result == 1) browseForFile();
            else if (result == 2) { if (onSlotClicked) onSlotClicked(this); }
            else if (result == 3) { if (onBeforeChange) onBeforeChange(); clearSample(); }
        });
}
