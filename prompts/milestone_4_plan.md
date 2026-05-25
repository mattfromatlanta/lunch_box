# Milestone 4: GUI Application

## Objective
Transform the command-line application into a GUI application with a simple floating window interface for selecting cubbi/jammi folders and processing samples.

## Requirements

### GUI Window Specifications
- **Window Type:** Floating/standalone application window
- **Window Title:** "Lunch Box"
- **Window Size:** Resizable with reasonable default (e.g., 600x400 pixels)
- **Window Style:** Modern, clean interface appropriate for audio tools

### GUI Components

1. **Header/Title**
   - Text: "chompi pack by matt from atlanta"
   - Styling: Prominent, centered or top-aligned
   - Font: Readable, slightly larger than body text

2. **Cubbi Folder Selection**
   - Button: "Select Cubbi Folder..."
   - Label/Display: Shows selected path or "no cubbi folder selected"
   - Behavior: Opens native file browser (Finder on macOS)
   - Validation: Verify folder exists and contains .wav files

3. **Jammi Folder Selection**
   - Button: "Select Jammi Folder..."
   - Label/Display: Shows selected path or "no jammi folder selected"
   - Behavior: Opens native file browser (Finder on macOS)
   - Validation: Verify folder exists and contains .wav files

4. **Process Button**
   - Button: "Process Samples" or "Convert to CHOMPI Format"
   - State: Disabled until at least one folder selected
   - Behavior: Triggers CHOMPI mode processing
   - Feedback: Shows progress or completion status

5. **Status/Output Area (Optional but Recommended)**
   - Text area showing processing status
   - Display file counts, bank assignments, errors
   - Scrollable for long outputs

### User Experience Flow

```
1. User launches application
   └─ GUI window opens

2. User clicks "Select Cubbi Folder"
   ├─ Native file browser opens (folder selection mode)
   ├─ User navigates to cubbi samples folder
   ├─ User confirms selection
   └─ GUI displays selected folder path

3. User clicks "Select Jammi Folder"
   ├─ Native file browser opens
   ├─ User selects jammi folder
   └─ GUI displays selected folder path

4. User clicks "Process Samples"
   ├─ GUI validates at least one folder selected
   ├─ Processing begins (existing CHOMPI mode logic)
   ├─ Status updates shown (optional)
   └─ Completion message displayed

5. User can process additional batches or close application
```

### Backward Compatibility
- Command-line interface should remain functional
- Detect if launched with CLI arguments → use CLI mode
- Detect if launched without arguments → launch GUI mode
- Both modes share core processing logic

## Technical Architecture

### JUCE Modules Required (New for GUI)
- **juce_gui_basics**: Core GUI components (buttons, labels, windows)
- **juce_gui_extra**: File browser dialogs
- **Existing modules**: juce_core, juce_audio_formats, juce_audio_basics

### Key JUCE GUI Classes

1. **juce::DocumentWindow** or **juce::DialogWindow**
   - Purpose: Main application window
   - Methods:
     - `setContentOwned()` to add main component
     - `centreWithSize()` for initial positioning
     - `setVisible()` to show window
     - `setResizable()` to allow window resizing

2. **juce::Component**
   - Purpose: Base class for custom GUI components
   - MainComponent will inherit from this
   - Methods:
     - `paint()` for custom drawing
     - `resized()` for layout management
     - `addAndMakeVisible()` to add child components

3. **juce::TextButton**
   - Purpose: Clickable buttons
   - Usage: Folder selection buttons, process button
   - Methods:
     - `onClick` lambda/callback for button actions
     - `setEnabled()` to enable/disable
     - `setButtonText()` to set label

4. **juce::Label**
   - Purpose: Display text (header, folder paths, status)
   - Methods:
     - `setText()` to update displayed text
     - `setFont()` for custom styling
     - `setJustificationType()` for alignment

5. **juce::FileChooser**
   - Purpose: Native file/folder selection dialogs
   - Usage: Open Finder for folder selection
   - Methods:
     - `browseForDirectory()` for folder selection
     - `getResult()` to get selected File object
     - Modal and async options available

6. **juce::TextEditor** (Optional)
   - Purpose: Multi-line text display for status/logs
   - Methods:
     - `setReadOnly()` for output-only display
     - `setText()` to update content
     - `setMultiLine()` for scrollable text area

### Program Structure (Updated)

```
lunch_box/
├── Source/
│   ├── Main.cpp                    # Entry point - detect CLI vs GUI mode
│   ├── AudioScanner.h/cpp          # (Existing)
│   ├── AudioConverter.h/cpp        # (Existing)
│   ├── LunchBoxNamer.h/cpp           # (Existing)
│   ├── Logger.h/cpp                # (Existing)
│   ├── FileSystemHelper.h/cpp      # (Existing)
│   │
│   ├── GUI/
│   │   ├── MainWindow.h/cpp        # NEW - Application window
│   │   ├── MainComponent.h/cpp     # NEW - Main GUI component
│   │   └── GuiProcessor.h/cpp      # NEW - Bridge between GUI and processing logic
│   │
│   └── CLI/
│       └── CliProcessor.h/cpp      # NEW - CLI mode processing (refactored from Main.cpp)
│
├── CMakeLists.txt                  # Updated for GUI application
└── Resources/                       # NEW - Optional: icons, images
```

### Core GUI Components

#### 1. MainWindow Class
```cpp
class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(juce::String name);
    ~MainWindow() override;

    void closeButtonPressed() override;

private:
    std::unique_ptr<MainComponent> mainComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
```

#### 2. MainComponent Class
```cpp
class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Header label
    juce::Label headerLabel;

    // Cubbi folder selection
    juce::TextButton selectCubbiButton;
    juce::Label cubbiPathLabel;
    juce::File selectedCubbiFolder;

    // Jammi folder selection
    juce::TextButton selectJammiButton;
    juce::Label jammiPathLabel;
    juce::File selectedJammiFolder;

    // Processing
    juce::TextButton processButton;

    // Status/output (optional)
    juce::TextEditor statusTextEditor;

    // File choosers
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Callbacks
    void selectCubbiFolder();
    void selectJammiFolder();
    void processFiles();
    void updateProcessButtonState();

    // Processing bridge
    std::unique_ptr<GuiProcessor> processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
```

#### 3. GuiProcessor Class
```cpp
// Bridge between GUI and existing processing logic
// This is a thin wrapper around LunchBoxProcessor for GUI context
class GuiProcessor
{
public:
    GuiProcessor(std::function<void(juce::String)> statusCallback);

    struct ProcessingResult
    {
        bool success;
        int cubbiFilesProcessed;
        int jammiFilesProcessed;
        juce::String message;
    };

    ProcessingResult processFiles(const juce::File& cubbiFolder,
                                  const juce::File& jammiFolder,
                                  const juce::File& outputFolder);

private:
    std::function<void(juce::String)> statusCallback;

    // Use existing processing infrastructure
    std::unique_ptr<Logger> logger;
    std::unique_ptr<LunchBoxProcessor> processor;  // Reuse shared processing logic
    juce::AudioFormatManager formatManager;
};
```

**Note:** GuiProcessor is a minimal bridge that:
- Builds an `AudioConfiguration` from GUI selections
- Instantiates `LunchBoxProcessor` (same as CLI mode uses)
- Provides GUI-specific status callbacks
- Wraps the shared processing logic for GUI context

### Application Flow

```
Main.cpp entry point
│
├─ Check for command-line arguments
│  │
│  ├─ Arguments present → CLI Mode
│  │  └─ Use CliProcessor (existing logic)
│  │
│  └─ No arguments → GUI Mode
│     │
│     └─ Initialize JUCE GUI Application
│        │
│        ├─ Create MainWindow
│        ├─ Create MainComponent
│        ├─ Setup GUI components
│        └─ Enter message loop
│
└─ GUI Event Handling
   │
   ├─ "Select Cubbi Folder" clicked
   │  ├─ Open FileChooser (folder mode)
   │  ├─ User selects folder
   │  ├─ Validate folder (exists, contains .wav files)
   │  ├─ Update cubbiPathLabel
   │  └─ Update process button state
   │
   ├─ "Select Jammi Folder" clicked
   │  ├─ Open FileChooser (folder mode)
   │  ├─ User selects folder
   │  ├─ Validate folder
   │  ├─ Update jammiPathLabel
   │  └─ Update process button state
   │
   └─ "Process Samples" clicked
      ├─ Disable process button (prevent double-click)
      ├─ Call GuiProcessor::processFiles()
      ├─ Update status display with progress
      ├─ Show completion message
      └─ Re-enable process button
```

## Implementation Steps

> **✅ Pre-Milestone 4 Refactoring Complete**
>
> The code has been refactored to separate CLI-specific logic from processing logic. This ensures that changes to audio processing tools (AudioConverter, LunchBoxNamer, LunchBoxProcessor) are immediately available to both CLI and GUI implementations.
>
> **Completed refactoring:**
> - ✅ Created `Source/CLI/CliProcessor` module for all CLI-specific code
> - ✅ Created `Source/AudioConfiguration.h` for shared data structures
> - ✅ Moved argument parsing, mode detection, and CLI orchestration to CliProcessor
> - ✅ Updated `Main.cpp` to minimal entry point (ready for dual-mode detection)
> - ✅ Processing logic (AudioConverter, LunchBoxNamer, LunchBoxProcessor) remains independent
> - ✅ Verified CLI functionality still works correctly after refactoring
>
> **Current architecture:**
> ```
> Main.cpp → CliProcessor → LunchBoxProcessor → AudioConverter, LunchBoxNamer
>                              (shared processing logic)
> ```
>
> **Ready for GUI implementation:**
> ```
> Main.cpp → GuiProcessor → LunchBoxProcessor → AudioConverter, LunchBoxNamer
>                              (same shared processing logic)
> ```

### Phase 1: JUCE GUI Application Setup

1. **Update CMakeLists.txt**
   - Change from console application to GUI application
   - Add juce_gui_basics and juce_gui_extra modules
   - Configure application properties (name, version, company)
   - Set bundle identifier for macOS

   ```cmake
   juce_add_gui_app(lunch_box
       PRODUCT_NAME "Lunch Box"
       COMPANY_NAME "Matt from Atlanta"
       BUNDLE_ID "com.mattfromatlanta.chompipack"
   )

   target_link_libraries(lunch_box
       PRIVATE
           juce::juce_core
           juce::juce_audio_formats
           juce::juce_audio_basics
           juce::juce_gui_basics
           juce::juce_gui_extra
   )
   ```

2. **Create GUI directory structure**
   - Create Source/GUI/ directory
   - Create Source/CLI/ directory (for refactored CLI code)

3. **Test basic GUI window**
   - Create minimal MainWindow and MainComponent
   - Verify window opens and displays
   - Test window resizing and closing

### Phase 2: Refactor Existing Code ✅ COMPLETE

**Status:** This phase has been completed as pre-milestone 4 refactoring.

**Completed work:**

1. **✅ Extracted CLI logic from Main.cpp**
   - Created `Source/CLI/CliProcessor.h` and `.cpp`
   - Moved argument parsing to CliProcessor
   - Moved mode detection and orchestration to CliProcessor
   - Main.cpp is now minimal (12 lines of actual code)

2. **✅ Created shared configuration types**
   - Created `Source/AudioConfiguration.h`
   - Moved `AudioConfiguration` struct (used by both CLI and processing)
   - Moved `OperationMode` enum (used by both CLI and processing)
   - Both CliProcessor and LunchBoxProcessor include this shared header

3. **✅ Processing logic already reusable**
   - `LunchBoxProcessor` can be instantiated and called from any context
   - `AudioConverter` is a standalone class with no CLI dependencies
   - `LunchBoxNamer` is a standalone class with no CLI dependencies
   - Logger can be instantiated anywhere
   - All processing modules ready for GUI integration

4. **✅ Updated Main.cpp structure**
   Current implementation (CLI only):
   ```cpp
   int main(int argc, char* argv[])
   {
       CliProcessor cliProcessor;
       return cliProcessor.run(argc, argv);
   }
   ```

   Ready for Phase 3 update to:
   ```cpp
   int main(int argc, char* argv[])
   {
       if (argc > 1)
       {
           // CLI mode
           CliProcessor cliProcessor;
           return cliProcessor.run(argc, argv);
       }
       else
       {
           // GUI mode - to be implemented in Phase 3
           return launchGUI();
       }
   }
   ```

**Benefits achieved:**
- ✅ Clear separation of concerns (CLI vs processing vs GUI)
- ✅ Processing code can be called from both CLI and GUI
- ✅ Changes to processing logic automatically available to both interfaces
- ✅ No code duplication between CLI and future GUI
- ✅ CLI still fully functional and tested

### Phase 3: Build GUI Components

1. **Create MainWindow.h/cpp**
   - Inherit from juce::DocumentWindow
   - Set window properties (title, size, position)
   - Create and add MainComponent
   - Handle window close event

2. **Create MainComponent.h/cpp**
   - Inherit from juce::Component
   - Add all GUI elements (labels, buttons, text editor)
   - Implement layout in resized()
   - Implement basic painting in paint()

3. **Implement GUI layout**
   - Header at top
   - Cubbi section (button + path label)
   - Jammi section (button + path label)
   - Process button
   - Status text area at bottom
   - Use proper spacing and margins
   - Make responsive to window resizing

### Phase 4: Implement File Selection

1. **Implement folder selection dialogs**
   - Use juce::FileChooser for native dialogs
   - Configure for directory selection only
   - Handle async callback when user selects folder
   - Update GUI labels with selected paths

2. **Add folder validation**
   - Check folder exists
   - Check folder contains .wav files
   - Display warning if empty or no WAV files
   - Update status text with warnings

3. **Implement folder state management**
   - Store selected File objects
   - Clear button to reset selections
   - Remember last selected directory (optional)
   - Validate before enabling process button

### Phase 5: Implement Processing Logic

1. **Create GuiProcessor class** (thin wrapper around existing LunchBoxProcessor)
   - Build `AudioConfiguration` from GUI folder selections
   - Initialize Logger (with GUI-appropriate output)
   - Initialize `juce::AudioFormatManager`
   - Instantiate `LunchBoxProcessor` (reuse shared logic from CLI)
   - Call `LunchBoxProcessor::processSamples()` with configuration
   - Provide status callbacks for GUI updates (optional enhancement)

   ```cpp
   ProcessingResult GuiProcessor::processFiles(...)
   {
       // Build configuration from GUI selections
       AudioConfiguration config;
       config.mode = OperationMode::Chompi;
       config.cubbiFolder = cubbiFolder;
       config.jammiFolder = jammiFolder;
       config.outputFolder = outputFolder;
       config.hasCubbi = cubbiFolder.exists();
       config.hasJammi = jammiFolder.exists();

       // Use existing LunchBoxProcessor (same as CLI)
       LunchBoxProcessor processor(logger);
       bool success = processor.processSamples(config, formatManager);

       // Return result for GUI display
       return {success, ...};
   }
   ```

2. **Connect GUI to processor**
   - Wire process button to GuiProcessor::processFiles()
   - Pass selected folders from GUI state
   - Receive and display results
   - Handle errors and display to user
   - **Note:** Processing uses identical logic to CLI mode

3. **Implement status updates**
   - Update status text area during processing
   - Show file counts, bank assignments
   - Display progress messages
   - Show completion message with summary

4. **Add processing feedback**
   - Disable controls during processing
   - Optional: Add progress bar
   - Show "Processing..." indicator
   - Re-enable controls when done

### Phase 6: Polish & Refinement

1. **Improve visual design**
   - Add consistent spacing and padding
   - Use color scheme appropriate for audio tool
   - Style buttons and labels
   - Add visual hierarchy (header, sections)

2. **Add user feedback**
   - Tooltips for buttons
   - Status messages for user actions
   - Clear error messages
   - Success confirmation dialog

3. **Keyboard shortcuts (optional)**
   - Cmd+O for folder selection
   - Cmd+P for process
   - Cmd+Q for quit

4. **Window management**
   - Save/restore window position and size
   - Set minimum window size
   - Make window properly closable

### Phase 7: Testing & Validation

1. **Test GUI functionality**
   - All buttons clickable and responsive
   - File dialogs open and work correctly
   - Folder paths display correctly
   - Process button enables/disables appropriately

2. **Test file processing**
   - Process cubbi folder only
   - Process jammi folder only
   - Process both folders
   - Verify output matches CLI mode

3. **Test edge cases**
   - Empty folders
   - Folders with no WAV files
   - Invalid folder paths
   - Canceling file dialog
   - Processing during processing (should be blocked)

4. **Test on macOS**
   - Native Finder integration works
   - Window behavior matches macOS conventions
   - Menu bar integration (if applicable)
   - Application icon displays

5. **Test CLI mode still works**
   - Verify CLI arguments still work
   - Ensure no GUI opens in CLI mode
   - Check output matches previous behavior

## Build Instructions (Updated)

### CMake Build Process
```bash
cd lunch_box
mkdir -p build
cd build
cmake ..
cmake --build .
```

### Running the Application

#### GUI Mode
```bash
# Just double-click the app on macOS, or:
open build/lunch_box_artefacts/lunch_box.app

# Or run directly:
./build/lunch_box_artefacts/lunch_box.app/Contents/MacOS/lunch_box
```

#### CLI Mode (Backward Compatible)
```bash
# CLI mode automatically activated when arguments present
./build/lunch_box_artefacts/lunch_box.app/Contents/MacOS/lunch_box \
  --cubbi ~/samples/cubbi \
  --jammi ~/samples/jammi

# Or use shorthand
./build/lunch_box_artefacts/lunch_box.app/Contents/MacOS/lunch_box \
  --c ~/samples/cubbi --j ~/samples/jammi
```

## Success Criteria

- [ ] GUI window opens on application launch (no CLI arguments)
- [ ] Header displays "chompi pack by matt from atlanta"
- [ ] "Select Cubbi Folder" button opens native file browser
- [ ] Selected cubbi path displays in GUI
- [ ] "Select Jammi Folder" button opens native file browser
- [ ] Selected jammi path displays in GUI
- [ ] Process button disabled until at least one folder selected
- [ ] Process button triggers CHOMPI mode processing
- [ ] Status updates display during processing
- [ ] Output files identical to CLI mode output
- [ ] CLI mode still works with command-line arguments
- [ ] Application window properly resizable
- [ ] Application closes gracefully
- [ ] No crashes or memory leaks
- [ ] Works on macOS (primary target platform)
- [ ] User-friendly error messages for invalid selections

## Dependencies Status

- ✅ JUCE 8.0.12 installed at ~/Repos/JUCE
  - ✅ juce_core
  - ✅ juce_audio_formats
  - ✅ juce_audio_basics
  - ➕ juce_gui_basics (to be added)
  - ➕ juce_gui_extra (to be added)
- ✅ C++ compiler (clang++ 17.0.0)
- ✅ CMake 4.2.1
- ✅ Milestone 1 complete (scanning functionality)
- ✅ Milestone 2 complete (conversion functionality)
- ✅ Milestone 3 complete (CHOMPI naming functionality)

## Technical Considerations

### JUCE Application Architecture

JUCE GUI applications use a message loop and event-driven architecture:
- Main thread handles GUI updates
- File processing should remain on main thread (not heavy enough to need threading)
- File choosers can be modal (block) or async (callback-based)
- Modern approach: Use async file choosers with lambdas

### Modal vs. Non-Modal File Dialogs

**Modal (blocking):**
```cpp
void selectFolder()
{
    juce::FileChooser chooser("Select folder", juce::File{});
    if (chooser.browseForDirectory())
    {
        selectedFolder = chooser.getResult();
        updateLabel();
    }
}
```

**Async (non-blocking, recommended):**
```cpp
void selectFolder()
{
    fileChooser = std::make_unique<juce::FileChooser>("Select folder");
    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
    {
        selectedFolder = fc.getResult();
        updateLabel();
    });
}
```

### Cross-Platform Considerations

While macOS is the primary target:
- JUCE provides native file dialogs on all platforms
- Use juce::File for cross-platform file handling
- Avoid platform-specific paths in code
- Test window behavior on target platform

### Layout Management

Use JUCE's layout tools for responsive design:
- **juce::FlexBox**: Modern, flexible layout system
- **setBounds()**: Manual positioning and sizing
- **Grid**: CSS Grid-like layout (JUCE 6+)

Recommendation: Start with setBounds() for simplicity, refactor to FlexBox if needed.

### Processing Performance

- Audio processing is fast enough for GUI thread
- For large batches (70+ files), consider:
  - Progress updates every N files
  - Allow processing cancellation (future enhancement)
  - Optional threading for very large batches

### Error Handling in GUI

- Display errors in status text area
- Consider modal alert windows for critical errors
- Don't crash on invalid user input
- Provide clear, actionable error messages

### Memory Management

- Use std::unique_ptr for owned components
- JUCE components use automatic memory management
- Be careful with FileChooser lifetime (keep as member variable)

## Future Considerations (Not in Milestone 4)

- **Drag-and-drop**: Drag folders directly onto window
- **Recent folders**: Remember and show recently used paths
- **Custom output location**: GUI selector for output folder
- **Preview mode**: Show what will be processed before converting
- **Progress bar**: Visual progress indicator for long operations
- **Settings panel**: Configure conversion settings via GUI
- **Dark mode**: Support macOS dark mode
- **Menu bar**: Standard File/Edit/Help menus
- **Multi-window**: Show processing log in separate window
- **Preferences**: Save user settings between sessions
- **Application icon**: Custom icon for macOS dock

## Notes

- GUI mode should feel natural for non-technical users
- CLI mode remains for power users and scripting
- Both modes share identical processing logic (no code duplication)
- Focus on simplicity - this is a utility tool, not a DAW
- Keep window layout clean and uncluttered
- Native platform integration (Finder on macOS) is important
- Error messages should guide user toward solution
- Processing output directory defaults to "converted/" folder

## Layout Mockup

```
┌─────────────────────────────────────────────────────┐
│ Lunch Box                                    [X]  │
├─────────────────────────────────────────────────────┤
│                                                     │
│          chompi pack by matt from atlanta          │
│                                                     │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Cubbi Folder:                                     │
│  ┌──────────────────┐  /path/to/samples/cubbi  │
│  │ Select Cubbi... │                              │
│  └──────────────────┘                              │
│                                                     │
│  Jammi Folder:                                     │
│  ┌──────────────────┐  /path/to/samples/jammi  │
│  │ Select Jammi... │                              │
│  └──────────────────┘                              │
│                                                     │
│            ┌───────────────────────┐               │
│            │  Process Samples      │               │
│            └───────────────────────┘               │
│                                                     │
├─────────────────────────────────────────────────────┤
│ Status:                                            │
│ ┌───────────────────────────────────────────────┐ │
│ │ Ready to process samples...                   │ │
│ │                                               │ │
│ │ Processing cubbi folder...                    │ │
│ │ Found 70 WAV files                            │ │
│ │ Bank A (14/14 slots filled)                   │ │
│ │ ...                                           │ │
│ │ Complete! Processed 112 files.                │ │
│ └───────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

## Implementation Priority

**Must Have (Milestone 4):**
- ✅ Main window with header
- ✅ Cubbi folder selection button and path display
- ✅ Jammi folder selection button and path display
- ✅ Process button
- ✅ Basic status display
- ✅ Folder validation
- ✅ CLI mode backward compatibility

**Should Have (Milestone 4+):**
- Progress indicator
- Scrollable status text area
- Clear/reset buttons
- Better error dialogs

**Nice to Have (Future):**
- Drag-and-drop support
- Recent folders list
- Custom output folder selection
- Progress bar
- Settings/preferences

---

## Refactoring Summary

### Architecture Benefits (Completed Pre-Milestone 4)

The codebase has been refactored to ensure clean separation between user interface (CLI/GUI) and processing logic. This architecture guarantees that changes to audio processing tools are immediately available to both UX options without code duplication.

**Module Separation:**

```
┌─────────────────────────────────────────────────────────────┐
│                         Main.cpp                            │
│                    (Entry Point / Router)                   │
└──────────────┬──────────────────────────┬───────────────────┘
               │                          │
               ▼                          ▼
    ┌──────────────────┐      ┌──────────────────┐
    │  CliProcessor    │      │  GuiProcessor    │
    │  (CLI Interface) │      │  (GUI Interface) │
    └────────┬─────────┘      └────────┬─────────┘
             │                         │
             └────────┬────────────────┘
                      │
                      ▼
         ┌────────────────────────┐
         │   LunchBoxProcessor      │
         │  (Shared Orchestrator) │
         └────────┬───────────────┘
                  │
      ┌───────────┼───────────┐
      ▼           ▼           ▼
┌─────────┐ ┌─────────┐ ┌─────────┐
│ Audio   │ │ Chompi  │ │ Logger  │
│Converter│ │  Namer  │ │         │
└─────────┘ └─────────┘ └─────────┘
   (Shared Processing Components)
```

**Key Design Principles:**

1. **Single Responsibility**
   - `CliProcessor`: CLI argument parsing, mode detection, CLI-specific I/O
   - `GuiProcessor`: GUI event handling, user interaction, GUI-specific feedback
   - `LunchBoxProcessor`: Orchestrates audio processing workflow
   - `AudioConverter`: Audio format conversion
   - `LunchBoxNamer`: CHOMPI naming and bank assignment

2. **Dependency Inversion**
   - Processing components don't depend on CLI or GUI
   - Both interfaces depend on processing components
   - Changes flow downward, never upward

3. **No Code Duplication**
   - Processing logic written once in `LunchBoxProcessor`
   - Both CLI and GUI call the same processing code
   - Bug fixes and features automatically benefit both interfaces

4. **Shared Data Structures**
   - `AudioConfiguration.h`: Configuration types used by all modules
   - `OperationMode` enum: Shared mode definitions
   - Clean data contracts between layers

**Verification:**

The refactoring has been verified by:
- ✅ Successful compilation of refactored code
- ✅ CLI functionality tested and working correctly
- ✅ All existing features preserved
- ✅ Clean module boundaries established
- ✅ Ready for GUI implementation without further refactoring

**Example: Making Changes**

If you need to add a new feature (e.g., sample normalization):

1. Add to `AudioConverter` or create new `AudioNormalizer` class
2. Call from `LunchBoxProcessor` orchestration
3. Feature automatically available to:
   - CLI mode (via CliProcessor)
   - GUI mode (via GuiProcessor)
   - No duplicate implementation needed

This architecture ensures the project remains maintainable as it grows.
