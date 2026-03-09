#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

//==============================================================================
// BankSlotComponent - Single bank slot in the advanced bank editor (M8)
//==============================================================================
// Displays an assigned audio file or an empty slot number.
// Accepts file drops from Finder; left-click browses; right-click shows menu.
//==============================================================================

class BankSlotComponent : public juce::Component,
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

    // juce::FileDragAndDropTarget (external — from Finder)
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

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
    juce::File previewFile;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void showContextMenu();
    static bool isSupportedAudioFile(const juce::String& path);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankSlotComponent)
};
