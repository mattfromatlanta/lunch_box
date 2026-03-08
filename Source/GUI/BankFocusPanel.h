#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "FocusedSlotRow.h"
#include "../Processing/ChompiNamer.h"
#include "../Processing/BankFolderParser.h"

//==============================================================================
// BankFocusPanel - Single-bank focus view (third tab)
//
// Shows one bank at a time as a vertical list of 14 FocusedSlotRows.
// Bank selector (A-E) is a left column; Cubbi/Jammi toggle is at the top.
// Stores all data internally (slots[2][5][14]) so assignments are preserved
// when switching banks/categories. Synced with BankEditorPanel on tab switch
// by MainComponent.
//==============================================================================

class BankFocusPanel : public juce::Component
{
public:
    BankFocusPanel(juce::AudioFormatManager& fmt, juce::AudioThumbnailCache& cache);
    ~BankFocusPanel();

    // --- Data access ---

    // Get all assignments for one category (all 5 banks)
    // Note: flushes active row state to storage before reading
    juce::Array<BankFolderParser::BankAssignment> getAssignments(ChompiNamer::Category cat);

    // Set a specific slot (used by MainComponent for cross-tab sync)
    void setSlot(ChompiNamer::Category cat, int bankIdx, int slotIdx, const juce::File& file);
    void clearAll();

    int getFilledCount(ChompiNamer::Category cat);

    // --- Bulk operations on the active bank ---
    void autoFillActiveFromFiles(const juce::Array<juce::File>& files);
    void clearActiveBank();

    // --- Keyboard navigation (driven by MainComponent::keyPressed) ---
    void moveFocusedRow(int delta);          // Up/Down arrow
    void expandRowSelection(int delta);      // Shift+Up/Down
    void playFocused();                      // Space
    void browseForFocused();                 // Enter
    void clearFocusedRows();                 // Delete/Backspace

    void triggerAutoFill();   // shows folder picker and fills active bank
    void triggerClear();      // clears active bank and refreshes rows

    // --- Category / bank switching (driven by MainComponent) ---
    void switchToCategory(ChompiNamer::Category cat);

    // --- Callbacks ---
    std::function<void()>                    onAssignmentsChanged;
    std::function<void(const juce::File&)>   onSlotClicked;
    std::function<void()>                    onPreviewStop;
    std::function<juce::File()>              getStartDirectory;
    std::function<void(juce::File)>          onFolderBrowsed;
    std::function<void(const juce::String&)> onLog;

    // --- juce::Component ---
    void paint(juce::Graphics& g) override;
    void resized() override;
    void modifierKeysChanged(const juce::ModifierKeys& mods) override;

private:
    // Shared audio resources (from AudioPreviewPlayer, owned by MainComponent)
    juce::AudioFormatManager&  formatManager;
    juce::AudioThumbnailCache& thumbnailCache;

    // All slot data: [category][bank][slot]
    juce::File slots[2][ChompiNamer::NUM_BANKS][ChompiNamer::SLOTS_PER_BANK];

    // Active view state
    ChompiNamer::Category activeCategory = ChompiNamer::Category::Cubbi;
    int activeBank = 0;  // 0=A … 4=E

    // Child components
    juce::TextButton bankButtons[ChompiNamer::NUM_BANKS];  // A-E

    juce::OwnedArray<FocusedSlotRow> rows;  // 14 rows for active bank

    std::unique_ptr<juce::FileChooser> fileChooser;

    // Selection / keyboard focus state
    int  focusedRowIdx   = 0;
    int  selectionAnchor = 0;
    juce::Array<int> selection;

    // Drag-to-reorder state
    int  dragSourceIdx    = -1;    // row index being dragged
    int  dragInsertIdx    = -1;    // current drag destination row
    bool isDragging       = false;
    bool deferredDeselect = false; // plain click on multi-selected row: wait for mouseUp

    static constexpr int BANK_COL_WIDTH  = 36;
    static constexpr int ROW_HEIGHT      = 55;
    static constexpr int ROW_GAP         = 1;

    // Internal helpers
    void switchToBank(int bankIdx);
    void flushRowsToStorage();      // write active row UI state back to slots[]
    void populateRowsFromStorage();
    void updateBankButtonStyles();
    void wireRowCallbacks(FocusedSlotRow* row, int rowIdx);

    // Selection helpers
    void setFocusedRow(int idx, bool clearOthers);
    void notifyPreviewForFocused();
    void updateRowVisuals();
    void toggleRowInSelection(int idx);
    void selectRowRange(int from, int to);

    bool isPopulating = false;      // prevents re-entrant flushRowsToStorage()

    std::unique_ptr<juce::LookAndFeel_V4> bankButtonLAF;

    void handleRowMouseDown(FocusedSlotRow* row, const juce::MouseEvent& e);
    void handleRowMouseDrag(FocusedSlotRow* row, const juce::MouseEvent& e);
    void handleRowMouseUp  (FocusedSlotRow* row, const juce::MouseEvent& e);

    void updateDragPreviews();  // live waveform preview + shift highlights during drag
    void clearAllPreviews();    // clears preview and shift-highlight state on all rows

    int rowIndexFor(FocusedSlotRow* row) const;
    int rowAtPoint(int x, int y) const;
    void commitReorder(int fromIdx, int toIdx);
    void clearReorderState();

    void styleTabButton(juce::TextButton& btn, bool active);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankFocusPanel)
};
