#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BankSlotComponent.h"
#include "../Processing/BankFolderParser.h"

//==============================================================================
// BankRowComponent - One CHOMPI bank (A–E) with 14 slots (M8)
//==============================================================================

class BankRowComponent : public juce::Component
{
public:
    explicit BankRowComponent(char bankLetter);

    // Slot access
    void setSlot(int index, const juce::File& file);  // index 0-13
    void clearSlot(int index);
    void clearAllSlots();
    juce::File getSlot(int index) const;
    int getFilledCount() const;
    BankSlotComponent* getSlotComponent(int index);  // for selection management

    // Bulk operations
    void sortSlotsAlphabetically();
    void autoFillFromFiles(const juce::Array<juce::File>& files, int startIndex = 0);

    // Export for processing
    juce::Array<BankFolderParser::BankAssignment> getAssignments() const;

    // Called when any slot changes — propagated from parent
    std::function<void()> onAssignmentsChanged;

    // Called when a slot is clicked (for preview)
    std::function<void(const juce::File&)> onSlotClicked;

    // Folder memory: return start dir for file browser; receive parent of selected file
    std::function<juce::File()>         getStartDirectory;
    std::function<void(juce::File)>     onFolderBrowsed;

    // juce::Component
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    char bankLetter;
    juce::OwnedArray<BankSlotComponent> slots;  // 14 slots

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankRowComponent)
};
