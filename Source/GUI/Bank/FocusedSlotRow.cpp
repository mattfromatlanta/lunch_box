// SPDX-License-Identifier: AGPL-3.0-or-later
#include "FocusedSlotRow.h"
#include "LunchBoxFonts.h"
#include "UIConstants.h"
#include "LabelStrings.h"
#include "../FileSystemHelper.h"
#include <BinaryData.h>

namespace
{
    const juce::Colour rowHoverBdr     = LunchBoxColours::WHITE_CREAM;
    const juce::Colour rowDropBdr      { 0xff4caf50 };
    const juce::Colour rowSelectedBdr  = LunchBoxColours::WHITE_CREAM;
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
    setTooltip(LunchBoxLabels::kTipSlotBank);
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
    {
        thumbnail.setSource(new juce::FileInputSource(file));
        setTooltip(file.getFileName());
    }
    else
    {
        thumbnail.clear();
        setTooltip(LunchBoxLabels::kTipSlotBank);
    }
    repaint();
    if (onSampleChanged) onSampleChanged(this);
}

void FocusedSlotRow::clearSample()
{
    sample = juce::File{};
    thumbnail.clear();
    setTooltip(LunchBoxLabels::kTipSlotBank);
    repaint();
    if (onSampleChanged) onSampleChanged(this);
}

void FocusedSlotRow::setSelected(bool s)
{
    if (selected != s) { selected = s; repaint(); }
}

void FocusedSlotRow::setFocused(bool f)
{
    if (focused != f) { focused = f; repaint(); }
}

void FocusedSlotRow::setDragSource(bool s)
{
    if (isDragSrc != s) { isDragSrc = s; repaint(); }
}

void FocusedSlotRow::setDragRoleSource(bool s)
{
    if (dragRoleSource != s) { dragRoleSource = s; repaint(); }
}

void FocusedSlotRow::setDragRoleDestination(bool s)
{
    if (dragRoleDestination != s) { dragRoleDestination = s; repaint(); }
}

void FocusedSlotRow::setDragRoleDisplace(int dir)
{
    if (dragRoleDisplace != dir) { dragRoleDisplace = dir; repaint(); }
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
    const juce::File&     displayFile   = showingPreview ? previewFile  : sample;
    const bool            displayFilled = (displayFile != juce::File{});
    juce::AudioThumbnail& displayThumb  = showingPreview ? previewThumbnail : thumbnail;

    // Drag preview overrides regular selection and focus visuals. Destination cells
    // take the selection look; source and displace cells suppress it. Focus styling
    // is fully suppressed whenever any drag role is active — the destination stripe
    // pattern is the only "focus" indicator during a drag.
    const bool inDragRole        = dragRoleDestination || dragRoleSource || dragRoleDisplace;
    const bool effectiveFocused  = focused && !inDragRole;
    const bool effectiveSelected = dragRoleDestination                  ? true
                                 : (dragRoleSource || dragRoleDisplace) ? false
                                                                        : selected;

    // Background
    juce::Colour bg;
    if (displayFilled)
        bg = effectiveSelected ? LunchBoxColours::getTabBg(bankColour).brighter(0.15f)
                               : LunchBoxColours::getTabBg(bankColour);
    else
        bg = effectiveSelected ? LunchBoxColours::BUTTON_BG.brighter(0.15f)
                               : LunchBoxColours::BUTTON_BG;
    constexpr float rowRadius = LunchBoxConstants::CORNER_RADIUS;
    auto fbounds = bounds.toFloat();
    g.setColour(bg);
    g.fillRoundedRectangle(fbounds, rowRadius);

    if (effectiveSelected && !effectiveFocused && !isDragSrc && !isDraggingOver)
    {
        auto* top = getTopLevelComponent();
        auto origin = top ? top->getLocalPoint(this, juce::Point<int>(0, 0))
                          : juce::Point<int>(0, 0);
        LunchBoxColours::drawSelectionStripes(g, fbounds, origin, rowRadius);
    }

    // Border
    auto borderBounds = fbounds.reduced(1.0f);
    if (isDragSrc)
    {
        g.setColour(insertionCol);
        g.drawRoundedRectangle(borderBounds, rowRadius, 4.0f);
    }
    else if (isDraggingOver)
    {
        g.setColour(rowDropBdr);
        g.drawRoundedRectangle(borderBounds, rowRadius, 4.0f);
    }
    else if (dragRoleDisplace)
    {
        g.setColour(LunchBoxColours::WHITE_CREAM.withAlpha(0.7f));
        g.drawRoundedRectangle(borderBounds, rowRadius, 2.0f * 1.25f);
    }
    else if (dragRoleSource)
    {
        // Vacated source — render neutral, no focus/selection visuals.
    }
    else if (effectiveFocused)
    {
        g.setColour(LunchBoxColours::FOCUS_BORDER);
        g.drawRoundedRectangle(borderBounds, rowRadius, LunchBoxConstants::BORDER_WIDTH_ACTIVE);
    }
    else if (effectiveSelected)
    {
        g.setColour(displayFilled ? LunchBoxColours::getFocused(bankColour).brighter(0.3f)
                                  : rowSelectedBdr);
        g.drawRoundedRectangle(borderBounds, rowRadius, 2.0f);
    }
    else if (isHovered)
    {
        g.setColour(displayFilled ? LunchBoxColours::getFocused(bankColour)
                                  : rowHoverBdr.withAlpha(0.5f));
        g.drawRoundedRectangle(borderBounds, rowRadius, 2.0f);
    }
    else if (displayFilled)
    {
        g.setColour(LunchBoxColours::getFocused(bankColour));
        g.drawRoundedRectangle(borderBounds, rowRadius, 2.0f);
    }

    // Slot number (left column, 28px)
    const int numW = 28;
    auto numArea = bounds.removeFromLeft(numW);
    g.setFont(LunchBoxFonts::body());
    g.setColour(displayFilled ? LunchBoxColours::WHITE_CREAM
                              : LunchBoxColours::WHITE_CREAM.withAlpha(0.4f));
    g.drawText(juce::String(slotNumber).paddedLeft('0', 2),
               numArea, juce::Justification::centred);

    if (!displayFilled)
        return;

    // Waveform drawn with bank colour
    if (displayThumb.getTotalLength() > 0.0)
    {
        g.setColour(bankColour.withAlpha(showingPreview ? 0.5f : 0.85f));
        displayThumb.drawChannels(g, bounds.reduced(0, 4),
                                  0.0, displayThumb.getTotalLength(), 1.0f);
    }

    // Displacement direction arrow — centered in the row when drag-displaced.
    // Up triangle (↑) = content moving to a lower global index.
    // Down triangle (↓) = content moving to a higher global index.
    if (dragRoleDisplace != 0)
    {
        const auto fb   = getLocalBounds().toFloat();
        const float cx  = fb.getCentreX();
        const float cy  = fb.getCentreY();
        const float arrowSize = 10.0f;
        juce::Rectangle<float> arrowBounds (cx - arrowSize * 0.5f, cy - arrowSize * 0.5f,
                                            arrowSize, arrowSize);

        auto xml = juce::XmlDocument::parse (
            juce::String::fromUTF8 (BinaryData::arrow_icon_svg, BinaryData::arrow_icon_svgSize));

        if (xml != nullptr)
        {
            auto* g_el   = xml->getChildByName ("g");
            auto* pathEl = g_el ? g_el->getChildByName ("path") : xml->getChildByName ("path");
            if (pathEl != nullptr)
            {
                auto col = LunchBoxColours::WHITE_CREAM.withAlpha (0.8f);
                pathEl->setAttribute ("fill", "#" + col.toDisplayString (false));
            }

            if (auto drawable = juce::Drawable::createFromSVG (*xml))
            {
                auto transform = juce::RectanglePlacement (juce::RectanglePlacement::centred)
                                     .getTransformToFit (drawable->getDrawableBounds(), arrowBounds);

                // SVG points right; rotate to point up (−90°) or down (+90°)
                const float angle = (dragRoleDisplace < 0) ? -juce::MathConstants<float>::halfPi
                                                           :  juce::MathConstants<float>::halfPi;
                transform = transform.followedBy (
                    juce::AffineTransform::rotation (angle, arrowBounds.getCentreX(), arrowBounds.getCentreY()));

                drawable->draw (g, 1.0f, transform);
            }
        }
    }
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

bool FocusedSlotRow::isInterestedInFileDrag(const juce::StringArray&)
{
    return false;  // Panel handles all external file drops
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

void FocusedSlotRow::showContextMenu()
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
            if      (result == 1) browseForFile();
            else if (result == 2) { if (onSlotClicked) onSlotClicked(this); }
            else if (result == 3) clearSample();
        });
}

void FocusedSlotRow::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint();
}
