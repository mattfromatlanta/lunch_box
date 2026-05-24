// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "UIColours.h"

//==============================================================================
// FocusedSlotRow - A single bank slot rendered as a tall horizontal bar.
//
// Used by BankFocusPanel. Shows slot number + waveform thumbnail + filename.
// Accepts file drops from Finder; right-click shows context menu.
// Mouse events are forwarded to BankFocusPanel for vertical drag-to-reorder.
//==============================================================================

class FocusedSlotRow : public juce::Component,
                       public juce::SettableTooltipClient,
                       public juce::FileDragAndDropTarget,
                       private juce::ChangeListener
{
public:
    FocusedSlotRow(int slotNumber,
                   juce::AudioFormatManager& fmt,
                   juce::AudioThumbnailCache& cache);
    ~FocusedSlotRow() override;

    void setSample(const juce::File& file);
    void clearSample();
    juce::File getSample() const { return sample; }
    bool hasSample()       const { return sample != juce::File{}; }
    int  getSlotNumber()   const { return slotNumber; }

    // Visual state (managed by BankFocusPanel)
    void setSelected(bool s);
    void setDragSource(bool s);       // this row is the current drag destination (orange border)
    void setBankColour(juce::Colour c) { bankColour = c; repaint(); }

    // Drag preview: show a different file's waveform without changing the actual sample
    void setPreviewSample(const juce::File& f);  // showingPreview=true, previewFile=f
    void clearPreviewSample();                    // showingPreview=false

    // Open file browser (also callable from keyboard via BankFocusPanel)
    void browseForFile();

    // Callbacks
    std::function<void(FocusedSlotRow*)>                          onSampleChanged;
    std::function<void(FocusedSlotRow*)>                          onSlotClicked;
    std::function<void(FocusedSlotRow*)>                          onRowDoubleClicked; // open file browser
    std::function<juce::File()>                                   getStartDirectory;
    std::function<void(juce::File)>                               onFolderBrowsed;

    // Mouse forwarding for reorder drag (handled by BankFocusPanel)
    std::function<void(FocusedSlotRow*, const juce::MouseEvent&)> onRowMouseDown;
    std::function<void(FocusedSlotRow*, const juce::MouseEvent&)> onRowMouseDrag;
    std::function<void(FocusedSlotRow*, const juce::MouseEvent&)> onRowMouseUp;

    // juce::Component
    void paint           (juce::Graphics& g)        override;
    void mouseDown       (const juce::MouseEvent& e) override;
    void mouseDrag       (const juce::MouseEvent& e) override;
    void mouseUp         (const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseEnter      (const juce::MouseEvent& e) override;
    void mouseExit       (const juce::MouseEvent& e) override;

    // juce::FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped  (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit  (const juce::StringArray& files) override;

private:
    int            slotNumber;
    juce::File     sample;
    juce::AudioThumbnail thumbnail;

    // Preview state (used during drag to show future positions)
    bool           showingPreview = false;
    juce::File     previewFile;
    juce::AudioThumbnail previewThumbnail;

    bool isDraggingOver   = false;
    bool isHovered        = false;
    bool selected         = false;
    bool isDragSrc        = false;
    juce::Colour bankColour = ChompiColours::WHITE_CREAM.withAlpha(0.3f);

    std::unique_ptr<juce::FileChooser> fileChooser;

    void showContextMenu();
    static bool isSupportedAudioFile(const juce::String& path);

    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FocusedSlotRow)
};
