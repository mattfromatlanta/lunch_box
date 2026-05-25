// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

//==============================================================================
// FolderDropZone - Folder selection panel with drag-and-drop support.
//==============================================================================
// Wraps a select button and path label inside a styled drop zone.
// Accepts folder drags from Finder; rejects files and multi-item drops.
//==============================================================================

class FolderDropZone : public juce::Component,
                       public juce::FileDragAndDropTarget
{
public:
    FolderDropZone(const juce::String& buttonText,
                   const juce::String& placeholder);

    // Assign these callbacks (styled like JUCE's button.onClick)
    std::function<void()>          onButtonClicked;
    std::function<void(juce::File)> onFolderSelected;

    // Update the displayed folder path
    void setSelectedFolder(const juce::File& folder);

    // juce::Component
    void paint(juce::Graphics& g) override;
    void resized() override;

    // juce::FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

private:
    juce::TextButton selectButton;
    juce::Label pathLabel;
    juce::String placeholderText;
    bool isDraggingOver = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FolderDropZone)
};
