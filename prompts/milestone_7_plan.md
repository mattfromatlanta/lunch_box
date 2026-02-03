# Milestone 7: Drag and Drop Folder Selection

## Objective
Add drag-and-drop functionality to the GUI, allowing users to drag folders directly onto folder selection areas instead of using the file browser buttons.

## Requirements

### Drag and Drop Targets

**Interactive Areas:**
1. **Cubbi folder area** - Drop zone for cubbi samples folder
2. **Jammi folder area** - Drop zone for jammi samples folder
3. **Output folder area** - Drop zone for output destination folder

### User Experience

**Visual Feedback:**
- Hover highlight when dragging over drop zone
- Visual indicator showing drop is possible
- Clear feedback on successful drop
- Error message for invalid drops

**Drop Behavior:**
- Accept folders only (not individual files)
- Update folder selection immediately
- Same validation as file browser selection
- Display folder path after drop
- Update process button state

**Drag States:**
- **Not dragging**: Normal appearance
- **Dragging over**: Highlighted border/background
- **Invalid drag**: Show "not allowed" cursor
- **Valid drag**: Show "copy" or "link" cursor

## Technical Architecture

### JUCE Drag and Drop

JUCE provides drag-and-drop through:
- `Component::FileDragAndDropTarget` interface
- `isInterestedInFileDrag()` - Check if drop is valid
- `filesDropped()` - Handle successful drop
- `fileDragEnter()` - Visual feedback on hover
- `fileDragExit()` - Clear visual feedback
- `fileDragMove()` - Track drag position

### Implementation Approach

Create drop-zone components that wrap existing labels/buttons:

```cpp
class FolderDropZone : public juce::Component,
                       public juce::FileDragAndDropTarget
{
public:
    FolderDropZone(std::function<void(juce::File)> onFolderDropped);

    // FileDragAndDropTarget interface
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

    // Visual feedback
    void paint(juce::Graphics& g) override;

private:
    std::function<void(juce::File)> callback;
    bool isDraggingOver = false;
};
```

### MainComponent Updates

**Current Structure:**
```
MainComponent
├── selectCubbiButton
├── cubbiPathLabel
├── selectJammiButton
├── jammiPathLabel
├── selectOutputButton
└── outputPathLabel
```

**Updated Structure:**
```
MainComponent
├── cubbiDropZone (contains button + label)
├── jammiDropZone (contains button + label)
├── outputDropZone (contains button + label)
└── existing components
```

## Implementation Steps

### Phase 1: Create FolderDropZone Component

1. **Create FolderDropZone Class**
   - Create `Source/GUI/FolderDropZone.h`
   - Create `Source/GUI/FolderDropZone.cpp`
   - Implement FileDragAndDropTarget interface

2. **Implement Drag Detection**
   ```cpp
   bool FolderDropZone::isInterestedInFileDrag(const juce::StringArray& files)
   {
       // Only accept folders (not files)
       if (files.size() != 1)
           return false;

       juce::File file(files[0]);
       return file.isDirectory();
   }
   ```

3. **Implement Drop Handling**
   ```cpp
   void FolderDropZone::filesDropped(const juce::StringArray& files, int x, int y)
   {
       if (files.size() == 1)
       {
           juce::File folder(files[0]);
           if (folder.isDirectory() && callback)
           {
               callback(folder);
           }
       }

       isDraggingOver = false;
       repaint();
   }
   ```

4. **Implement Visual Feedback**
   ```cpp
   void FolderDropZone::paint(juce::Graphics& g)
   {
       // Draw children first
       Component::paint(g);

       // Draw hover overlay
       if (isDraggingOver)
       {
           g.setColour(juce::Colours::blue.withAlpha(0.1f));
           g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);

           g.setColour(juce::Colours::blue);
           g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 5.0f, 2.0f);
       }
   }

   void FolderDropZone::fileDragEnter(const juce::StringArray& files, int x, int y)
   {
       isDraggingOver = true;
       repaint();
   }

   void FolderDropZone::fileDragExit(const juce::StringArray& files)
   {
       isDraggingOver = false;
       repaint();
   }
   ```

### Phase 2: Integrate into MainComponent

1. **Replace Labels with Drop Zones**
   ```cpp
   // In MainComponent.h
   class MainComponent : public juce::Component
   {
   private:
       // Replace simple labels with drop zones
       std::unique_ptr<FolderDropZone> cubbiDropZone;
       std::unique_ptr<FolderDropZone> jammiDropZone;
       std::unique_ptr<FolderDropZone> outputDropZone;

       // Keep existing buttons (now inside drop zones)
       juce::TextButton selectCubbiButton;
       juce::Label cubbiPathLabel;
       // ... etc
   };
   ```

2. **Update Constructor**
   ```cpp
   MainComponent::MainComponent()
   {
       // Create drop zones with callbacks
       cubbiDropZone = std::make_unique<FolderDropZone>(
           [this](juce::File folder) {
               handleCubbiFolderSelection(folder);
           }
       );

       jammiDropZone = std::make_unique<FolderDropZone>(
           [this](juce::File folder) {
               handleJammiFolderSelection(folder);
           }
       );

       outputDropZone = std::make_unique<FolderDropZone>(
           [this](juce::File folder) {
               handleOutputFolderSelection(folder);
           }
       );

       // Add buttons and labels to drop zones
       cubbiDropZone->addAndMakeVisible(selectCubbiButton);
       cubbiDropZone->addAndMakeVisible(cubbiPathLabel);

       // Add drop zones to main component
       addAndMakeVisible(cubbiDropZone.get());
       // ... etc
   }
   ```

3. **Consolidate Folder Handling**
   ```cpp
   // Unified handler for both button and drag-drop
   void MainComponent::handleCubbiFolderSelection(juce::File folder)
   {
       if (!folder.exists() || !folder.isDirectory())
           return;

       selectedCubbiFolder = folder;
       cubbiPathLabel.setText(folder.getFullPathName(), juce::dontSendNotification);
       cubbiPathLabel.setColour(juce::Label::textColourId, juce::Colours::white);

       // Validation
       juce::Array<juce::File> wavFiles;
       folder.findChildFiles(wavFiles, juce::File::findFiles, true, "*.wav");

       if (wavFiles.isEmpty())
       {
           appendStatus("Warning: Cubbi folder contains no WAV files");
           cubbiPathLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
       }
       else
       {
           appendStatus("Cubbi folder selected: " + juce::String(wavFiles.size()) + " WAV files found");
       }

       updateProcessButtonState();
   }

   // Update button callback to use handler
   void MainComponent::selectCubbiFolder()
   {
       // ... existing file chooser code ...
       fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
       {
           auto folder = chooser.getResult();
           if (folder != juce::File{})
           {
               handleCubbiFolderSelection(folder);
           }
       });
   }
   ```

### Phase 3: Enhanced Visual Feedback

1. **Add Drag Indicator Text**
   ```cpp
   void FolderDropZone::paint(juce::Graphics& g)
   {
       Component::paint(g);

       if (isDraggingOver)
       {
           // Highlight background
           g.setColour(juce::Colours::blue.withAlpha(0.1f));
           g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);

           // Border
           g.setColour(juce::Colours::blue);
           g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 5.0f, 2.0f);

           // "Drop folder here" text
           g.setColour(juce::Colours::blue);
           g.setFont(juce::Font(14.0f, juce::Font::bold));
           g.drawText("Drop folder here",
                     getLocalBounds(),
                     juce::Justification::centred);
       }
   }
   ```

2. **Add Subtle Drop Zone Hints**
   ```cpp
   void FolderDropZone::paint(juce::Graphics& g)
   {
       // Always show a subtle dashed border to indicate drop zone
       if (!isDraggingOver)
       {
           g.setColour(juce::Colours::grey.withAlpha(0.3f));

           // Draw dashed border
           juce::Path dashPath;
           auto bounds = getLocalBounds().toFloat().reduced(2);
           dashPath.addRoundedRectangle(bounds, 5.0f);

           float dashLengths[] = {5.0f, 5.0f};
           juce::PathStrokeType strokeType(1.0f);
           strokeType.createDashedStroke(dashPath, dashPath, dashLengths, 2);

           g.strokePath(dashPath, strokeType);
       }

       // ... existing hover code ...
   }
   ```

### Phase 4: Error Handling

1. **Invalid Drop Feedback**
   ```cpp
   void FolderDropZone::filesDropped(const juce::StringArray& files, int x, int y)
   {
       isDraggingOver = false;
       repaint();

       if (files.size() != 1)
       {
           juce::AlertWindow::showMessageBoxAsync(
               juce::AlertWindow::WarningIcon,
               "Invalid Drop",
               "Please drop a single folder (not multiple items).",
               "OK"
           );
           return;
       }

       juce::File item(files[0]);

       if (!item.isDirectory())
       {
           juce::AlertWindow::showMessageBoxAsync(
               juce::AlertWindow::WarningIcon,
               "Invalid Drop",
               "Please drop a folder (not a file).",
               "OK"
           );
           return;
       }

       if (callback)
           callback(item);
   }
   ```

2. **Access Permission Errors**
   ```cpp
   void MainComponent::handleCubbiFolderSelection(juce::File folder)
   {
       if (!folder.exists())
       {
           juce::AlertWindow::showMessageBoxAsync(
               juce::AlertWindow::WarningIcon,
               "Folder Not Found",
               "The selected folder does not exist.",
               "OK"
           );
           return;
       }

       // Test read access
       auto children = folder.findChildFiles(juce::File::findFiles, false, "*");
       if (children.isEmpty() && !folder.getNumberOfChildFiles(juce::File::findFiles) == 0)
       {
           juce::AlertWindow::showMessageBoxAsync(
               juce::AlertWindow::WarningIcon,
               "Access Denied",
               "Cannot read folder contents. Please check permissions.",
               "OK"
           );
           return;
       }

       // Continue with normal processing
       // ...
   }
   ```

### Phase 5: Testing

1. **Drag and Drop Tests**
   - Drag folder onto cubbi zone → should accept
   - Drag file onto cubbi zone → should reject
   - Drag multiple folders → should reject
   - Drag onto wrong zone → should accept (any folder accepted)
   - Drag from external app (Finder) → should work
   - Drag between zones within app → should work

2. **Visual Tests**
   - Hover highlight appears correctly
   - Hover highlight disappears on exit
   - "Drop here" text appears
   - Invalid cursor shows for files
   - Valid cursor shows for folders

3. **Integration Tests**
   - Drag-drop works same as button selection
   - Validation runs after drag-drop
   - Status messages appear
   - Process button enables/disables
   - File counting works

4. **Edge Cases**
   - Drag empty folder → should warn about no files
   - Drag folder with only non-WAV files → should warn
   - Drag folder without permissions → should error
   - Drag symbolic link to folder → should follow link

## User Experience Flow

### Typical Workflow

**Before Drag-Drop:**
1. Click "Select Cubbi Folder..."
2. Navigate in file browser
3. Select folder
4. Click "Select"
5. Repeat for jammi and output

**After Drag-Drop:**
1. Drag cubbi folder from Finder onto GUI
2. Drag jammi folder from Finder onto GUI
3. (Optional) Drag output folder onto GUI
4. Click "Process Samples"

**Time Saved:** ~50% reduction in clicks/navigation

### Visual States

```
Normal State:
┌─────────────────────────┐
│ [Select Cubbi...]       │
│ no cubbi folder         │
│ (subtle dashed border)  │
└─────────────────────────┘

Dragging Over:
┌═════════════════════════┐ ← Blue highlight
║ Drop folder here        ║ ← Bold blue text
║ [Select Cubbi...]       ║
║ no cubbi folder         ║
└═════════════════════════┘

After Drop:
┌─────────────────────────┐
│ [Select Cubbi...]       │
│ /Users/matt/samples     │ ← White text
└─────────────────────────┘
```

## Success Criteria

- [ ] FolderDropZone component created
- [ ] Drag-drop works for cubbi folder
- [ ] Drag-drop works for jammi folder
- [ ] Drag-drop works for output folder
- [ ] Visual feedback appears on hover
- [ ] Only folders accepted (files rejected)
- [ ] Multiple items rejected
- [ ] Validation runs after drop
- [ ] Same behavior as button selection
- [ ] Error messages for invalid drops
- [ ] Works on macOS
- [ ] Documentation updated
- [ ] User guide includes drag-drop instructions

## Dependencies

- ✅ Milestone 4 complete (GUI exists)
- ✅ JUCE FileDragAndDropTarget interface
- ➕ New FolderDropZone component

## Platform Considerations

### macOS
- Native drag-drop from Finder
- Standard OS cursors
- Tested and working

### Windows
- Native drag-drop from Explorer
- Standard OS cursors
- Should work (JUCE abstraction)

### Linux
- Native drag-drop from file manager
- May vary by desktop environment
- Should work (JUCE abstraction)

## Future Considerations (Not in Milestone 7)

- **Drag individual files** (Milestone 8)
- **Drag multiple folders** at once
- **Drag-drop reordering** within banks
- **Visual folder preview** on hover
- **Undo** drag-drop action

## Notes

- Focus on folder drag-drop only
- Maintain button selection as alternative
- Both methods should feel equally valid
- Clear visual feedback is critical
- Error messages must be helpful
- Platform-native behavior preferred

## Estimated Impact

**Code Changes:** Small-Medium
- FolderDropZone component: ~150 lines
- MainComponent refactoring: ~100 lines
- Integration: ~50 lines
- Tests: ~80 lines

**User Benefit:** High
- Faster workflow
- More intuitive interface
- Reduced clicks
- Improved UX for experienced users
- Maintains accessibility (buttons still work)
