// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>

//==============================================================================
// BankSlotComponent - One cell in the Pack-mode 5×14 grid (one CHOMPI slot).
//==============================================================================
// Displays an assigned audio file or an empty slot number.
// Accepts file drops from Finder; left-click browses; right-click shows menu.
//==============================================================================

class BankSlotComponent : public juce::Component,
                          public juce::SettableTooltipClient,
                          public juce::FileDragAndDropTarget,
                          public juce::DragAndDropTarget
{
public:
    BankSlotComponent(char bankLetter, int slotNumber);

    void setSample(const juce::File& file);
    void clearSample();
    juce::File getSample() const    { return sample; }
    bool hasSample() const          { return sample != juce::File{}; }
    int  getSlotNumber() const      { return slotNumber; }

    // Selection / focus visual state (managed by BankEditorPanel)
    void setSelected(bool s);
    void setFocused(bool f);
    bool isSlotSelected() const     { return selected; }
    bool isSlotFocused()  const     { return focused; }

    // Open file browser (called by Enter key via BankEditorPanel)
    void browseForFile();

    // Callbacks
    // Fired just before a sample is about to change (for undo capture)
    std::function<void()> onBeforeChange;

    std::function<void(BankSlotComponent*)> onSampleChanged;
    std::function<void(BankSlotComponent*)> onSlotClicked;       // triggered externally for preview
    std::function<void(BankSlotComponent*)> onSlotDoubleClicked; // open file browser

    // Folder memory: return start dir for browser; receive parent of selected file
    std::function<juce::File()>                                    getStartDirectory;
    std::function<void(juce::File)>                                onFolderBrowsed;

    // Selection / drag-move visual state (managed by BankEditorPanel)
    void setDragTarget(bool t);
    bool isSlotDragTarget() const { return dragTarget; }

    // Drag preview: show a different filename label during drag without changing the actual sample
    void setPreviewSample(const juce::File& f);   // showingPreview=true, previewFile=f
    void clearPreviewSample();                     // showingPreview=false

    // Swap-mode source highlight: warm tint on selected cells when cmd is held during drag
    void setSwapHighlight(bool s);

    // Drag-preview visual roles (all reset between drags). Each is independent.
    void setDragRoleSource     (bool s);   // content has been picked up — render vacated, suppress selection
    void setDragRoleDestination(bool s);   // moving content lands here — render with selection style
    // Displaced content lands here. dir: -1 = moves to lower index (left arrow),
    //   0 = clear, +1 = moves to higher index (right arrow).
    void setDragRoleDisplace   (int dir);
    // Labels shown during displace preview: lower-index slot on left, higher on right.
    void setDisplaceLabels(const juce::String& left, const juce::String& right);

    // Selection callbacks (wired by BankEditorPanel)
    std::function<void(BankSlotComponent*, const juce::MouseEvent&)> onSlotMouseDown;
    std::function<void(BankSlotComponent*, const juce::MouseEvent&)> onSlotMouseDrag;
    std::function<void(BankSlotComponent*, const juce::MouseEvent&)> onSlotMouseUp;

    // juce::Component
    void paint(juce::Graphics& g) override;
    void mouseDown       (const juce::MouseEvent& e) override;
    void mouseDrag       (const juce::MouseEvent& e) override;
    void mouseUp         (const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseEnter      (const juce::MouseEvent& e) override;
    void mouseExit       (const juce::MouseEvent& e) override;

    // juce::FileDragAndDropTarget (external — delegated to panel; filesDropped required by interface)
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray&, int, int) override {}

    // juce::DragAndDropTarget (internal — slot to slot)
    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails& details) override;
    void itemDragExit(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;

private:
    char bankLetter;
    int  slotNumber;
    juce::File sample;
    bool isDraggingOver = false;
    bool isHovered      = false;
    bool selected       = false;
    bool focused        = false;
    bool dragTarget     = false;
    bool showingPreview = false;
    bool swapHighlight  = false;
    bool dragRoleSource      = false;
    bool dragRoleDestination = false;
    int  dragRoleDisplace    = 0;    // -1 left arrow, 0 off, +1 right arrow
    juce::String displaceLeftLabel;
    juce::String displaceRightLabel;
    juce::File previewFile;

    std::unique_ptr<juce::FileChooser>  fileChooser;
    std::unique_ptr<juce::Drawable>     arrowDrawable;

    void showContextMenu();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankSlotComponent)
};
