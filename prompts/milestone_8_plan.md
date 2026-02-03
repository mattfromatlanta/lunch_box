# Milestone 8: Individual Sample Management

## Objective
Enable users to drag or browse individual audio files into specific bank slots, with the ability to reorder, sort, and organize samples at a granular level before processing.

## Requirements

### Visual Bank Editor

**GUI Addition:** Bank management panel showing all 5 banks with 14 slots each

**Features:**
- Visual representation of all 70 slots (per category)
- Drag-drop individual samples into specific slots
- Browse button for each slot
- Reorder samples within banks (drag to reorder)
- Clear individual slots
- Clear entire banks
- Preview sample names in slots
- Visual indication of filled vs empty slots

### Sample Management

**Individual Sample Operations:**
- Add sample to specific slot
- Remove sample from slot
- Move sample between slots
- Replace sample in slot
- Auto-fill empty slots (like current batch mode)

**Source Options:**
- Drag files from Finder
- Browse for individual files
- Drag from folder into bank
- Drag between banks

### Bank View Layout

```
┌─────────────────────────────────────────────────────┐
│ Cubbi Banks                                         │
├─────────────────────────────────────────────────────┤
│                                                     │
│ Bank A  [1][2][3][4][5][6][7][8][9][10][11][12][13][14]  │
│         │kick │snare│hat │    │    │    │ ...         │
│                                                     │
│ Bank B  [1][2][3][4][5][6][7][8][9][10][11][12][13][14]  │
│         │clap │rim │    │    │    │    │ ...         │
│                                                     │
│ Bank C  [1][2][3][4][5][6][7][8][9][10][11][12][13][14]  │
│         │    │    │    │    │    │    │ ...         │
│                                                     │
│ [Switch to Jammi Banks]                             │
│                                                     │
│ [Auto-Fill Empty Slots from Folder...]             │
│ [Clear All Banks]  [Sort All Alphabetically]       │
└─────────────────────────────────────────────────────┘
```

## Technical Architecture

### New Components

**1. BankSlotComponent**
```cpp
class BankSlotComponent : public juce::Component,
                          public juce::FileDragAndDropTarget,
                          public juce::DragAndDropTarget
{
public:
    BankSlotComponent(int slotNumber);

    // Sample management
    void setSample(const juce::File& file);
    void clearSample();
    juce::File getSample() const;
    bool hasExample() const;

    // Drag and drop (from external)
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // Drag and drop (internal reordering)
    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;

    // Visual
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Callbacks
    std::function<void(BankSlotComponent*)> onSampleChanged;
    std::function<void(BankSlotComponent*)> onSlotClicked;

private:
    int slotNumber;
    juce::File sample;
    bool isEmpty = true;
    bool isHovered = false;
    bool isDraggingOver = false;
};
```

**2. BankRowComponent**
```cpp
class BankRowComponent : public juce::Component
{
public:
    BankRowComponent(char bankLetter); // 'a'-'e'

    // Slot access
    BankSlotComponent* getSlot(int index); // 0-13
    void setSlot(int index, const juce::File& file);
    void clearSlot(int index);
    void clearAllSlots();

    // Bulk operations
    void sortSlotsAlphabetically();
    void autoFillFromFolder(const juce::File& folder);
    int getFilledSlotCount() const;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    char bankLetter;
    juce::OwnedArray<BankSlotComponent> slots; // 14 slots
    juce::Label bankLabel;
};
```

**3. BankEditorPanel**
```cpp
class BankEditorPanel : public juce::Component
{
public:
    BankEditorPanel(ChompiNamer::Category category);

    // Bank access
    BankRowComponent* getBank(char letter); // 'a'-'e'

    // Operations
    void clearAllBanks();
    void sortAllBanks();
    void autoFillFromFolder(const juce::File& folder);

    // Export for processing
    std::vector<BankAssignment> getAssignments() const;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    ChompiNamer::Category category;
    juce::OwnedArray<BankRowComponent> banks; // 5 banks (A-E)

    juce::TextButton clearAllButton;
    juce::TextButton sortAllButton;
    juce::TextButton autoFillButton;
};
```

### Integration with MainComponent

**UI Structure:**
```
MainComponent
├── Header
├── Mode Selector: [Simple Mode] [Advanced Mode]
│
├── Simple Mode (current implementation)
│   ├── Cubbi folder selection
│   ├── Jammi folder selection
│   ├── Output folder selection
│   └── Process button
│
└── Advanced Mode (new)
    ├── Category Tabs: [Cubbi] [Jammi]
    ├── BankEditorPanel (per category)
    ├── Output folder selection
    └── Process button
```

## Implementation Steps

### Phase 1: Slot Component

1. **Create BankSlotComponent**
   - Visual representation (box with sample name)
   - Drag-drop target (accept audio files)
   - Drag-drop source (enable reordering)
   - Click to browse
   - Right-click context menu (clear, replace)

2. **Visual States**
   - Empty: Grey box with slot number
   - Filled: White box with sample name (truncated)
   - Hovered: Highlight border
   - Dragging over: Blue highlight
   - Selected: Bold border

3. **Drag and Drop**
   - Accept external file drops (from Finder)
   - Accept internal drags (from other slots)
   - Provide drag feedback
   - Support drag to reorder

### Phase 2: Bank Row Component

1. **Create BankRowComponent**
   - Contains 14 BankSlotComponents
   - Bank letter label (A, B, C, D, E)
   - Horizontal layout
   - Filled slot counter (X/14)

2. **Slot Management**
   - Add sample to slot
   - Remove sample from slot
   - Clear all slots
   - Sort slots alphabetically

3. **Visual Layout**
   ```
   Bank A [10/14]
   [1:kick] [2:snare] [3:hat] [4:clap] [5:rim] ...
   ```

### Phase 3: Bank Editor Panel

1. **Create BankEditorPanel**
   - Contains 5 BankRowComponents
   - Category label (Cubbi or Jammi)
   - Bulk operation buttons
   - Scroll support for smaller screens

2. **Bulk Operations**
   - Auto-fill from folder
   - Clear all banks
   - Sort all banks
   - Export to processing format

3. **Category Switching**
   - Toggle between Cubbi and Jammi panels
   - Maintain state when switching
   - Visual indication of active category

### Phase 4: Mode Switching

1. **Add Mode Selector**
   - Simple mode (current folder-based)
   - Advanced mode (individual sample management)
   - Remember mode preference

2. **Simple Mode (Default)**
   - Current UI (folders + process)
   - Fast workflow
   - No manual organization

3. **Advanced Mode**
   - Bank editor panel
   - Granular control
   - Power user features

### Phase 5: Sample Operations

1. **Drag Individual Files**
   - From Finder to slot
   - Between slots (reorder)
   - Between banks (move)
   - Visual feedback

2. **Browse Individual Files**
   - Click slot → file browser
   - Select audio file
   - Update slot

3. **Context Menu**
   - Right-click slot → menu
   - Clear, Replace, Preview
   - Cut, Copy, Paste

4. **Keyboard Shortcuts**
   - Delete: Clear selected slot
   - Cmd+X: Cut sample
   - Cmd+C: Copy sample
   - Cmd+V: Paste sample
   - Cmd+A: Select all in bank

### Phase 6: Integration

1. **Update Processing Pipeline**
   - Accept assignments from BankEditorPanel
   - Process individual sample assignments
   - Maintain existing folder-based mode

2. **Data Flow**
   ```
   Simple Mode:
   Folder → Auto-assign → Process

   Advanced Mode:
   Manual assignments → Validate → Process
   ```

3. **Validation**
   - Check all assigned files exist
   - Validate audio format
   - Warn about empty slots

## User Workflows

### Workflow 1: Quick Organization

1. Switch to Advanced Mode
2. Click "Auto-Fill from Folder" for Bank A
3. Select folder with 14 kick samples
4. Click "Auto-Fill from Folder" for Bank B
5. Select folder with 14 snare samples
6. Process

### Workflow 2: Manual Curation

1. Switch to Advanced Mode
2. Drag "my_kick.wav" to Bank A Slot 1
3. Drag "my_snare.wav" to Bank A Slot 2
4. Browse for hihat, add to Bank A Slot 3
5. Continue building kit manually
6. Process

### Workflow 3: Reordering

1. Load samples via Auto-Fill
2. Drag Slot 5 to Slot 1 (reorder)
3. Drag Slot 2 to Bank B Slot 1 (move banks)
4. Sort Bank C alphabetically
5. Process

## Success Criteria

- [ ] BankSlotComponent displays sample name
- [ ] Can drag file from Finder to slot
- [ ] Can drag between slots to reorder
- [ ] Can browse file into slot
- [ ] Can clear individual slots
- [ ] Can clear entire bank
- [ ] BankRowComponent shows all 14 slots
- [ ] BankEditorPanel shows all 5 banks
- [ ] Mode switching works (Simple ↔ Advanced)
- [ ] Auto-fill from folder works
- [ ] Sort alphabetically works
- [ ] Processing uses individual assignments
- [ ] Empty slots handled gracefully
- [ ] Visual feedback clear and intuitive
- [ ] Keyboard shortcuts work
- [ ] Context menus work
- [ ] Documentation updated

## Dependencies

- ✅ Milestone 4 (GUI exists)
- ✅ Milestone 7 (Drag-drop infrastructure)
- ➕ New bank editor components
- ➕ Updated processing pipeline

## Future Considerations

- **Sample preview** (play audio - Milestone 10)
- **Waveform display** in slots (Milestone 10)
- **Templates** (save/load bank configurations)
- **Batch edit** (apply to multiple slots)
- **Undo/Redo** for sample management
- **Search/filter** samples

## Notes

- Keep Simple Mode as default (most users)
- Advanced Mode for power users
- Both modes equally valid
- Don't force complexity on simple use cases
- Maintain fast folder-based workflow

## Estimated Impact

**Code Changes:** Large
- BankSlotComponent: ~300 lines
- BankRowComponent: ~200 lines
- BankEditorPanel: ~300 lines
- MainComponent updates: ~200 lines
- Integration: ~150 lines
- Tests: ~250 lines
- **Total: ~1400 lines**

**User Benefit:** Very High (for power users)
- Granular control
- Precise organization
- Flexible workflows
- Professional features
- Optional complexity
