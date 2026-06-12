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
    void setFocused(bool f);
    void setDragSource(bool s);       // legacy: orange highlight (kept for transition)

    // Drag-preview visual roles (all reset between drags). Each is independent.
    void setDragRoleSource     (bool s);   // content has been picked up — render vacated, suppress selection
    void setDragRoleDestination(bool s);   // moving content lands here — render with selection style
    // Displaced content lands here. dir: -1 = moves to lower index (up arrow),
    //   0 = clear, +1 = moves to higher index (down arrow).
    void setDragRoleDisplace   (int dir);
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

    // juce::FileDragAndDropTarget (external — delegated to panel; filesDropped required by interface)
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray&, int, int) override {}

private:
    int            slotNumber;
    juce::File     sample;
    juce::AudioThumbnail thumbnail;

    // Preview state (used during drag to show future positions)
    bool           showingPreview = false;
    juce::File     previewFile;
    juce::AudioThumbnail previewThumbnail;

    bool isHovered        = false;
    bool selected         = false;
    bool focused          = false;
    bool isDragSrc        = false;
    bool dragRoleSource      = false;
    bool dragRoleDestination = false;
    int  dragRoleDisplace    = 0;    // -1 up arrow, 0 off, +1 down arrow
    juce::Colour bankColour = LunchBoxColours::WHITE_CREAM.withAlpha(0.3f);

    std::unique_ptr<juce::FileChooser>  fileChooser;
    std::unique_ptr<juce::Drawable>     arrowDrawable;

    void showContextMenu();

    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FocusedSlotRow)
};
