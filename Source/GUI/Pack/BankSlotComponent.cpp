// SPDX-License-Identifier: AGPL-3.0-or-later
#include "BankSlotComponent.h"
#include "UIColours.h"
#include "UIConstants.h"
#include "LunchBoxFonts.h"
#include "LabelStrings.h"
#include "../FileSystemHelper.h"

namespace
{
    const juce::Colour slotEmptyBg       = LunchBoxColours::SLOT_EMPTY_BG;
    const juce::Colour slotBorder        = LunchBoxColours::SLOT_BORDER;
    const juce::Colour slotHoverBdr      = LunchBoxColours::WHITE_CREAM;
    const juce::Colour slotDropBdr       = LunchBoxColours::SLOT_DROP_BDR;
    const juce::Colour slotNumCol        = LunchBoxColours::WHITE_CREAM.withAlpha(0.3f);
    const juce::Colour slotTxtCol        = LunchBoxColours::WHITE_CREAM;

    static juce::Colour bankBorderColour(char bankLetter)
    {
        switch (bankLetter)
        {
            case 'A': return LunchBoxColours::RED;
            case 'B': return LunchBoxColours::PINK_SALMON;
            case 'C': return LunchBoxColours::YELLOW;
            case 'D': return LunchBoxColours::TEAL;
            case 'E': return LunchBoxColours::PURPLE;
            default:  return slotBorder;
        }
    }
    const juce::Colour slotSelectedBg    = LunchBoxColours::SLOT_SELECTED_BG;
    const juce::Colour slotFocusBdr      = LunchBoxColours::FOCUS_BORDER;
    const juce::Colour slotSelectedBdr   = LunchBoxColours::WHITE_CREAM;
    const juce::Colour slotDragTargetBg  = LunchBoxColours::SLOT_DRAG_TARGET_BG;
    const juce::Colour slotDragTargetBdr = LunchBoxColours::SLOT_DRAG_TARGET_BDR;
    const juce::Colour slotSwapSourceBg  = LunchBoxColours::SLOT_SWAP_SRC_BG;
    const juce::Colour slotSwapSourceBdr = LunchBoxColours::SLOT_SWAP_SRC_BDR;
}

BankSlotComponent::BankSlotComponent(char bank, int slot)
    : bankLetter(bank >= 'a' ? (char)(bank - ('a' - 'A')) : bank)
    , slotNumber(slot)
{
    setTooltip(LunchBoxLabels::kTipSlotPack);
}

void BankSlotComponent::setSample(const juce::File& file)
{
    sample = file;
    setTooltip(file.getFileName());
    repaint();
    if (onSampleChanged) onSampleChanged(this);
}

void BankSlotComponent::clearSample()
{
    sample = juce::File{};
    setTooltip(LunchBoxLabels::kTipSlotPack);
    repaint();
    if (onSampleChanged) onSampleChanged(this);
}

void BankSlotComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(2).toFloat();

    // For display purposes: use preview file during drag previews, else actual sample
    const juce::File& displayFile   = showingPreview ? previewFile : sample;
    const bool        displayFilled = (displayFile != juce::File{});

    const juce::Colour filledBg = LunchBoxColours::getFocused(bankBorderColour(bankLetter));

    // Drag preview overrides regular selection visuals:
    //   Source cells vacate (suppress selection look)
    //   Destination cells take over the selection look
    //   Displace cells render normally but get a thicker accent border
    const bool effectiveSelected = (dragRoleDestination)
                                    ? true
                                    : (dragRoleSource ? false : selected);

    juce::Colour bg;
    if      (dragTarget)                          bg = displayFilled ? filledBg.brighter(0.25f)      : slotDragTargetBg;
    else if (swapHighlight && effectiveSelected)  bg = displayFilled ? slotSwapSourceBg.brighter(0.1f) : slotSwapSourceBg;
    else if (effectiveSelected)                   bg = displayFilled ? filledBg.brighter(0.15f)      : slotSelectedBg;
    else                                          bg = displayFilled ? filledBg                       : slotEmptyBg;
    g.setColour(bg);
    g.fillRoundedRectangle(bounds, LunchBoxConstants::CORNER_RADIUS);

    if (effectiveSelected && !focused && !dragTarget && !swapHighlight)
    {
        auto* top = getTopLevelComponent();
        auto origin = top ? top->getLocalPoint(this, juce::Point<int>(0, 0))
                          : juce::Point<int>(0, 0);
        LunchBoxColours::drawSelectionStripes(g, bounds, origin, LunchBoxConstants::CORNER_RADIUS);
    }

    juce::Colour border;
    float bw = LunchBoxConstants::BORDER_WIDTH;
    if      (isDraggingOver)                     { border = slotDropBdr;              bw = LunchBoxConstants::BORDER_WIDTH_ACTIVE; }
    else if (dragTarget)                         { border = slotDragTargetBdr;        bw = LunchBoxConstants::BORDER_WIDTH_ACTIVE; }
    else if (dragRoleDisplace)                   { border = LunchBoxColours::WHITE_CREAM.withAlpha(0.7f);
                                                   bw     = LunchBoxConstants::BORDER_WIDTH * 1.25f; }
    else if (dragRoleSource)                     { border = LunchBoxColours::getBorder(bankBorderColour(bankLetter)); }
    else if (focused)                            { border = slotFocusBdr;             bw = LunchBoxConstants::BORDER_WIDTH_ACTIVE; }
    else if (swapHighlight && effectiveSelected) { border = slotSwapSourceBdr; }
    else if (effectiveSelected)                  { border = slotSelectedBdr; }
    else if (isHovered)                          { border = slotHoverBdr; }
    else                                         { border = LunchBoxColours::getBorder(bankBorderColour(bankLetter)); }
    g.setColour(border);
    g.drawRoundedRectangle(bounds, LunchBoxConstants::CORNER_RADIUS, bw);

    // Always show cell label (e.g. "A1", "E14") at 16pt
    const juce::String cellLabel = juce::String::charToString((juce::juce_wchar)bankLetter)
                                   + juce::String(slotNumber);
    g.setFont(LunchBoxFonts::body());

    if (displayFilled)
    {
        const int pillW = (int)((getWidth() - 4) * 0.6f);
        const int pillH = static_cast<int>(g.getCurrentFont().getHeight()) + 8;
        const float pillR = pillH / 2.0f;
        auto pill = juce::Rectangle<int>(pillW, pillH).withCentre(getLocalBounds().getCentre()).toFloat();
        g.setColour(LunchBoxColours::getLabelBg(bankBorderColour(bankLetter)));
        g.fillRoundedRectangle(pill, pillR);
        g.setColour(LunchBoxColours::WHITE_CREAM);
        g.drawRoundedRectangle(pill, pillR, 2.0f);
    }

    g.setColour(displayFilled ? slotTxtCol : slotNumCol);
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

void BankSlotComponent::setDragRoleSource(bool s)
{
    if (dragRoleSource != s) { dragRoleSource = s; repaint(); }
}

void BankSlotComponent::setDragRoleDestination(bool s)
{
    if (dragRoleDestination != s) { dragRoleDestination = s; repaint(); }
}

void BankSlotComponent::setDragRoleDisplace(bool s)
{
    if (dragRoleDisplace != s) { dragRoleDisplace = s; repaint(); }
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
        LunchBoxLabels::kChooseAudioFile, startDir, "*.wav;*.aiff;*.aif;*.mp3;*.flac", true);

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
    menu.addItem(1, LunchBoxLabels::kCtxBrowse);
    if (hasSample())
    {
        menu.addSeparator();
        menu.addItem(2, LunchBoxLabels::kCtxPreview);
        menu.addSeparator();
        menu.addItem(3, LunchBoxLabels::kCtxClearSlot);
    }

    menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this),
        [this](int result)
        {
            if (result == 1) browseForFile();
            else if (result == 2) { if (onSlotClicked) onSlotClicked(this); }
            else if (result == 3) { if (onBeforeChange) onBeforeChange(); clearSample(); }
        });
}
