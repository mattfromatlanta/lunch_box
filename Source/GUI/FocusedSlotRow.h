#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

//==============================================================================
// FocusedSlotRow - A single bank slot rendered as a tall horizontal bar.
//
// Used by BankFocusPanel. Shows slot number + waveform thumbnail + filename.
// Accepts file drops from Finder; right-click shows context menu.
// Mouse events are forwarded to BankFocusPanel for vertical drag-to-reorder.
//==============================================================================

class FocusedSlotRow : public juce::Component,
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
    void setDragSource(bool s);   // this row is being dragged (ghosted)
    void setInsertionTarget(bool t);  // draw insertion line above this row

    // Open file browser (also callable from keyboard via BankFocusPanel)
    void browseForFile();

    // Callbacks
    std::function<void(FocusedSlotRow*)>                          onSampleChanged;
    std::function<void(FocusedSlotRow*)>                          onSlotClicked;
    std::function<juce::File()>                                   getStartDirectory;
    std::function<void(juce::File)>                               onFolderBrowsed;

    // Mouse forwarding for reorder drag (handled by BankFocusPanel)
    std::function<void(FocusedSlotRow*, const juce::MouseEvent&)> onRowMouseDown;
    std::function<void(FocusedSlotRow*, const juce::MouseEvent&)> onRowMouseDrag;
    std::function<void(FocusedSlotRow*, const juce::MouseEvent&)> onRowMouseUp;

    // juce::Component
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp  (const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

    // juce::FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped  (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit  (const juce::StringArray& files) override;

private:
    int            slotNumber;
    juce::File     sample;
    juce::AudioThumbnail thumbnail;

    bool isDraggingOver   = false;
    bool isHovered        = false;
    bool selected         = false;
    bool isDragSrc        = false;
    bool insertionTarget  = false;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void showContextMenu();
    static bool isSupportedAudioFile(const juce::String& path);

    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FocusedSlotRow)
};
