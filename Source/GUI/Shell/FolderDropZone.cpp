// SPDX-License-Identifier: AGPL-3.0-or-later
#include "FolderDropZone.h"
#include "LunchBoxFonts.h"
#include "UIColours.h"
#include "UIConstants.h"

namespace
{
    const juce::Colour zoneBg            = LunchBoxColours::DROPZONE_BG;
    const juce::Colour zoneBorder        = LunchBoxColours::DROPZONE_BORDER;
    const juce::Colour hoverFill         = LunchBoxColours::DROPZONE_HOVER_BG;
    const juce::Colour hoverBorder       = LunchBoxColours::DROPZONE_HOVER_BORDER;
    const juce::Colour pathColour        = LunchBoxColours::DROPZONE_PATH;
    const juce::Colour placeholderColour = LunchBoxColours::DROPZONE_PLACEHOLDER;
    constexpr float cornerSize           = LunchBoxConstants::CORNER_RADIUS;
}

FolderDropZone::FolderDropZone(const juce::String& buttonText,
                               const juce::String& placeholder)
    : placeholderText(placeholder)
{
    selectButton.setButtonText(buttonText);
    selectButton.onClick = [this] { if (onButtonClicked) onButtonClicked(); };
    addAndMakeVisible(selectButton);

    pathLabel.setText(placeholder, juce::dontSendNotification);
    pathLabel.setFont(LunchBoxFonts::nav());
    pathLabel.setColour(juce::Label::textColourId, placeholderColour);
    addAndMakeVisible(pathLabel);
}

void FolderDropZone::setSelectedFolder(const juce::File& folder)
{
    pathLabel.setText(folder.getFullPathName(), juce::dontSendNotification);
    pathLabel.setColour(juce::Label::textColourId, pathColour);
}

void FolderDropZone::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    // Zone background
    g.setColour(zoneBg);
    g.fillRoundedRectangle(bounds, cornerSize);

    if (isDraggingOver)
    {
        // Hover fill + border
        g.setColour(hoverFill);
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(hoverBorder);
        g.drawRoundedRectangle(bounds.reduced(1.0f), cornerSize, 2.0f);

        // Overlay text
        g.setColour(hoverBorder);
        g.setFont(LunchBoxFonts::cta());
        g.drawText("Drop folder here", getLocalBounds(), juce::Justification::centred);
    }
    else
    {
        // Subtle dashed border
        juce::Path borderPath;
        borderPath.addRoundedRectangle(bounds.reduced(1.0f), cornerSize);

        juce::PathStrokeType strokeType(1.5f);
        float dashes[] = { 5.0f, 5.0f };
        juce::Path dashedPath;
        strokeType.createDashedStroke(dashedPath, borderPath, dashes, 2);

        g.setColour(zoneBorder);
        g.fillPath(dashedPath);
    }
}

void FolderDropZone::resized()
{
    auto area = getLocalBounds().reduced(10);
    selectButton.setBounds(area.removeFromTop(28).removeFromLeft(200));
    area.removeFromTop(6);
    pathLabel.setBounds(area);
}

bool FolderDropZone::isInterestedInFileDrag(const juce::StringArray& files)
{
    return files.size() == 1 && juce::File(files[0]).isDirectory();
}

void FolderDropZone::filesDropped(const juce::StringArray& files, int, int)
{
    isDraggingOver = false;
    repaint();

    if (files.size() != 1)
        return;

    juce::File folder(files[0]);
    if (!folder.isDirectory())
        return;

    setSelectedFolder(folder);

    if (onFolderSelected)
        onFolderSelected(folder);
}

void FolderDropZone::fileDragEnter(const juce::StringArray&, int, int)
{
    isDraggingOver = true;
    repaint();
}

void FolderDropZone::fileDragExit(const juce::StringArray&)
{
    isDraggingOver = false;
    repaint();
}
