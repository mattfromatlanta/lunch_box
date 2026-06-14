# CLAUDE.md - AI Assistant Guide for Lunch Box

**Purpose:** This document provides context and guidelines for AI assistants (Claude, etc.) working on the Lunch Box project.

---

## Project Overview

**Lunch Box** is a dual-mode (GUI + CLI) audio sample processor for the CHOMPI sampler hardware. It converts audio files (WAV/AIFF/MP3/FLAC) to CHOMPI-compatible format (16-bit 48kHz WAV) and organizes them using the CHOMPI naming convention.

**Key Documentation:**
- [README.md](README.md) - User-facing project overview
- [HOW_TO.md](HOW_TO.md) - Comprehensive user guide

---

## Architecture

### Core Design Principle

**Separation of concerns:** Processing logic is completely independent of interface (CLI/GUI).

```
Main.cpp (Router)
    ↓
    ├─→ CliProcessor (CLI interface)
    │       ↓
    └─→ GuiProcessor (GUI interface)
            ↓
    LunchBoxProcessor (Shared orchestrator)
            ↓
    ┌───────┼───────┐
    ↓       ↓       ↓
AudioConverter  LunchBoxNamer  Logger
    (Shared processing components)
```

**Key Insight:** Changes to AudioConverter, LunchBoxNamer, or LunchBoxProcessor automatically benefit both CLI and GUI. No code duplication.

### Module Responsibilities

| Module | Purpose | Key Files |
|--------|---------|-----------|
| **LunchBoxProcessor** | Orchestrates CHOMPI workflow | Processing/LunchBoxProcessor.h/cpp |
| **AudioConverter** | Format conversion (16-bit 48kHz) + optional -6 dB peak normalization | Processing/AudioConverter.h/cpp |
| **LunchBoxNamer** | CHOMPI naming (cubbi_a1.wav, etc.) | Processing/LunchBoxNamer.h/cpp |
| **BankFolderParser** | Bank subfolder detection + assignment | Processing/BankFolderParser.h/cpp |
| **PackModel** | Single source of truth for all slot assignments ([category][bank][slot]); Pack and Bank are pure views over it | GUI/Common/PackModel.h/cpp |
| **ExportThread** | Runs a pack export off the message thread (stage-then-swap, cancellable) | GUI/Shell/ExportThread.h/cpp |
| **Logger** | Timestamped log files | Logger.h/cpp |
| **FileSystemHelper** | File utilities, format extension list | FileSystemHelper.h/cpp |
| **CliProcessor** | CLI interface and argument parsing | CLI/CliProcessor.h/cpp |
| **GuiProcessor** | GUI-processing bridge | GUI/Shell/GuiProcessor.h/cpp |
| **MainComponent** | GUI shell: tabs, view switching, undo, clipboard, export flow | GUI/Shell/MainComponent.h/cpp + `MainComponent_{View,Session,Process,Undo,Commands}.cpp` |
| **BankEditorPanel** | Pack view: 5×14 grid per category | GUI/Pack/BankEditorPanel.h/cpp + `_Selection/_Drag/_ExternalDrag` |
| **BankFocusPanel** | Bank view: single-bank 14-row focus view | GUI/Bank/BankFocusPanel.h/cpp + `_Selection/_Drag/_ExternalDrag` |
| **DragModel / DragController** | Shared drag-and-drop logic (pure model + driver) | GUI/Common/DragModel.h/cpp, DragController.h/cpp, DragHost.h |
| **PreviewPanel / AudioPreviewPlayer** | Waveform + audio playback | GUI/Preview/ |
| **MainWindow / AppMenuBar** | Application window, macOS menu bar | GUI/Shell/ |

Several GUI classes are split across multiple translation units sharing one header
(e.g. `MainComponent_View.cpp`, `BankEditorPanel_Drag.cpp`) plus a `_Private.h` for
implementation-internal helpers. Each file opens with a comment mapping the split.

### Data Flow

**CLI Mode:**
```
Command line args → CliProcessor → AudioConfiguration → LunchBoxProcessor → Output files
```

**GUI Mode:**
```
PackModel (slot assignments) → ExportThread → GuiProcessor → LunchBoxProcessor → Output files
```
The Pack grid and Bank focus list are pure views over `PackModel`; export runs on a
background thread and stages output before atomically swapping it into place.

**Shared:** Both modes use `AudioConfiguration` struct and call the same `LunchBoxProcessor::processSamples()`.

---

## CHOMPI Sampler Context

**Critical to understand:** The CHOMPI sampler has specific requirements that drive all design decisions.

### Bank Structure
- **2 categories:** cubbi (percussive), jammi (chromatic)
- **5 banks per category:** A, B, C, D, E
- **14 slots per bank:** Numbered 1-14
- **Total capacity:** 70 samples per category (hardware limit)
- **Output files per category:** 70

### Naming Convention
```
{category}_{bank}{slot}.wav              # Sample

Examples:
  cubbi_a1.wav          (Cubbi, Bank A, Slot 1)
  jammi_e14.wav         (Jammi, Bank E, Slot 14)
```

**Duration Limit:** Maximum 2 minutes (120 seconds) per sample.

### Audio Requirements
- **Format:** WAV (uncompressed)
- **Bit depth:** 16-bit (hardware limitation)
- **Sample rate:** 48kHz (hardware requirement)
- **Channels:** Mono or stereo (max 2 channels)

**See:** README.md "CHOMPI Sampler Overview" section for details.

---

## Development Guidelines

### Code Style

**Naming Conventions:**
- Classes: `PascalCase` (e.g., `LunchBoxProcessor`)
- Functions: `camelCase` (e.g., `processFiles()`)
- Variables: `camelCase` (e.g., `selectedFolder`)
- Constants: `UPPER_SNAKE_CASE` or `kPascalCase`

**File Organization:**
- Header guards: `#pragma once` (not `#ifndef`)
- Includes: System → Third-party → Project
- Class layout: Public → Protected → Private

**Memory Management:**
- Use `std::unique_ptr` for owned objects
- Use JUCE's `OwnedArray` for JUCE components
- Avoid raw `new`/`delete`
- RAII for all resources

**Error Handling:**
- Return `bool` or result structs (not exceptions in hot paths)
- Log all errors with context
- Graceful degradation (skip bad files, continue processing)

**Code Comments:**
- Write brief comments explaining the purpose of classes, methods, and functions to someone reading the code for the first time
- Only use comments to document intent; never for WIP notes or change rationale
- Comments describe the code as it is, not the history of how it got there
- Favor self-documenting code with clear names over verbose comments

### Testing

The unit test suite lives in `tests/` (JUCE UnitTest framework, target `lunch_box_tests`):
- `ChompiNamerTests.cpp` — naming/bank-slot math
- `BankFolderParserTests.cpp` — bank subfolder detection
- `FileSystemHelperTests.cpp` — file utilities
- `DragModelTests.cpp` — drag-and-drop model logic
- `PackModelTests.cpp` — shared slot-assignment model (set/clear/snapshot/assignments)

**When adding features:**
- Add unit tests for new functions
- Update existing tests if behavior changes

### Build System

**CMake Configuration:**
- JUCE modules: `juce_core`, `juce_audio_formats`, `juce_audio_basics`, `juce_audio_devices`, `juce_audio_utils`, `juce_gui_basics`, `juce_gui_extra`
- App target: `lunch_box` — test target: `lunch_box_tests`
- JUCE is resolved from a sibling `../JUCE` clone or `-DJUCE_DIR=/path/to/JUCE`

**Build Process:**
```bash
mkdir -p build && cd build
cmake ..
make
```

**Output:**
- macOS: `build/lunch_box_artefacts/Lunch Box.app`
- Tests: `build/lunch_box_tests_artefacts/lunch_box_tests`

---

## Working with the Codebase

### Common Tasks

**Adding a new processing feature:**
1. Implement in appropriate module (AudioConverter, LunchBoxNamer, etc.)
2. Update LunchBoxProcessor if orchestration changes
3. No changes needed to CliProcessor or GuiProcessor (they delegate to LunchBoxProcessor)
4. Add tests
5. Update documentation

**Adding a new GUI feature:**
1. Find the owning panel (Shell / Pack / Bank / Preview) — keep panel logic in the panel
2. Shared visual constants go in `GUI/Style/` (UIColours, UIConstants, LunchBoxFonts, LabelStrings)
3. Keyboard/menu commands route through `MainComponent_Commands.cpp` (ApplicationCommandManager)
4. State that must survive view switches belongs in MainComponent's sync layer (`MainComponent_View.cpp`)

**Adding a new CLI option:**
1. Update CliProcessor argument parsing
2. Update AudioConfiguration if needed
3. Update usage message
4. Test CLI mode

### Important Constraints

**Never break CLI mode:**
- CLI is used in scripts/automation
- Backward compatibility critical
- Test CLI after any changes to shared code

**CHOMPI format is fixed:**
- Always 16-bit 48kHz WAV output
- Always CHOMPI naming convention
- Always 14 slots per bank, 5 banks
- Never compromise quality for speed
- Maximum 2-minute duration per sample

**File safety:**
- Never modify input files
- Never overwrite without warning
- Always log operations
- Handle disk full gracefully

---

## Source Layout

```
lunch_box/
├── README.md                          # User-facing project overview
├── HOW_TO.md                          # Comprehensive user guide
├── CLAUDE.md                          # This file (AI assistant guide)
│
├── Source/
│   ├── Main.cpp                       # Entry point, GUI/CLI routing
│   ├── AudioConfiguration.h           # Shared config struct
│   ├── FileSystemHelper.h/cpp         # File utilities, format extension list
│   ├── Logger.h/cpp                   # Logging system
│   ├── CLI/
│   │   └── CliProcessor.h/cpp         # CLI argument parsing + processing
│   ├── GUI/
│   │   ├── Shell/                     # MainWindow, MainComponent (+ split TUs),
│   │   │                              # AppMenuBar, GuiProcessor, ExportThread,
│   │   │                              # ConsoleWindow, ClipboardHelper, overlays,
│   │   │                              # footer buttons
│   │   ├── Pack/                      # BankEditorPanel (+ split TUs),
│   │   │                              # BankRowComponent, BankSlotComponent
│   │   ├── Bank/                      # BankFocusPanel (+ split TUs), FocusedSlotRow
│   │   ├── Common/                    # PackModel, DragModel, DragController,
│   │   │                              # DragHost, ClipboardEntry
│   │   ├── Preview/                   # PreviewPanel, AudioPreviewPlayer,
│   │   │                              # WaveformDisplay
│   │   └── Style/                     # UIColours, UIConstants, LunchBoxFonts,
│   │                                  # LabelStrings, FooterButtonLAF
│   └── Processing/
│       ├── LunchBoxProcessor.h/cpp    # Processing orchestrator
│       ├── AudioConverter.h/cpp       # Format conversion
│       ├── LunchBoxNamer.h/cpp        # CHOMPI naming + constants
│       └── BankFolderParser.h/cpp     # Bank subfolder detection
│
└── tests/                             # Unit tests (JUCE UnitTest, lunch_box_tests)
```

---

## Common Pitfalls

### 1. Breaking Abstraction Boundaries
❌ **Wrong:** GUI calling AudioConverter directly
✅ **Right:** GUI calls GuiProcessor, which calls LunchBoxProcessor, which calls AudioConverter

### 2. Duplicating Processing Logic
❌ **Wrong:** Implementing processing differently in CLI vs GUI
✅ **Right:** Both interfaces call the same LunchBoxProcessor code

### 3. Ignoring CHOMPI Constraints
❌ **Wrong:** Supporting 16 slots per bank or custom naming
✅ **Right:** Always 14 slots, always CHOMPI naming convention

### 4. Not Updating Both Modes
❌ **Wrong:** Adding feature to GUI only
✅ **Right:** Ensure feature available (or gracefully absent) in both modes

### 5. Assuming Single Platform
❌ **Wrong:** Using macOS-specific paths or APIs unguarded
✅ **Right:** Use JUCE cross-platform abstractions; isolate platform code (see ClipboardHelper.mm / ClipboardHelper.cpp)

---

## Testing Changes

### Before Committing

```bash
# Build
cd build
make

# Test GUI
open "lunch_box_artefacts/Lunch Box.app"
# Verify: slot assignment, processing, output

# Test CLI
"lunch_box_artefacts/Lunch Box.app/Contents/MacOS/Lunch Box" \
  --cubbi /path/to/cubbi/samples \
  --jammi /path/to/jammi/samples
# Verify: same output as GUI

# Run unit tests
./lunch_box_tests_artefacts/lunch_box_tests
```

### Regression Checks

- ✅ CLI still processes test folders correctly
- ✅ GUI opens without crashes
- ✅ Output files are valid 16-bit 48kHz WAV
- ✅ CHOMPI naming correct (cubbi_a1.wav, etc.)
- ✅ Logs created in the per-user log folder (`~/Library/Logs/Lunch Box` on macOS)

---

## Project Philosophy

### Design Principles

1. **User convenience over features**
   - Power features should be optional
   - Don't force complexity

2. **Quality over speed**
   - Never compromise audio quality
   - Correct bit depth/sample rate critical
   - Validate everything

3. **Clarity over cleverness**
   - Readable code > compact code
   - Obvious algorithms > optimized obscurity
   - Good names > comments

4. **Reliability over features**
   - Never crash (handle all errors)
   - Never lose data (never modify originals)
   - Never surprise user (clear feedback)

### User-Centric Thinking

**Remember:** Users are audio producers preparing samples for a hardware sampler. They need:
- **Speed:** Fast workflow (drag-drop, batch processing)
- **Quality:** Perfect audio (no degradation)
- **Trust:** Reliable, predictable results
- **Clarity:** Clear feedback, helpful errors

---

## Getting Help

### When Stuck

1. **Check existing code** - Pattern probably exists elsewhere
2. **Review git history** - See how similar features were added
3. **Check JUCE docs** - Framework has most functionality needed

### JUCE Resources

- **Tutorials:** https://docs.juce.com/master/tutorial_getting_started.html
- **API Docs:** https://docs.juce.com/master/
- **Forum:** https://forum.juce.com/

### Project Specifics

- **CHOMPI Hardware:** https://creditor.technology (for understanding target device)
- **User Workflows:** See HOW_TO.md examples

---

## Quick Reference

### Build Commands
```bash
# Full rebuild
cd build && cmake .. && make

# Run GUI
open "lunch_box_artefacts/Lunch Box.app"

# Run CLI
"lunch_box_artefacts/Lunch Box.app/Contents/MacOS/Lunch Box" --help

# Run tests
./lunch_box_tests_artefacts/lunch_box_tests
```

### Key Constants (Processing/LunchBoxNamer.h, Processing/AudioConverter.h)
```cpp
SLOTS_PER_BANK = 14                // CHOMPI structure
NUM_BANKS = 5                      // A, B, C, D, E
MAX_FILES_PER_CATEGORY = 70        // CHOMPI hardware limit
TARGET_SAMPLE_RATE = 48000.0       // CHOMPI requirement
TARGET_BIT_DEPTH = 16              // CHOMPI requirement
MAX_CHANNELS = 2                   // mono or stereo only
MAX_DURATION_SECONDS = 120.0       // Maximum 2 minutes per sample
```

---

**Author:** [Matt from Atlanta](https://github.com/mattfromatlanta) — [Bluesky](https://bsky.app/profile/mattfromatlanta.bsky.social) · [Bandcamp](https://angsttanks.bandcamp.com/)
**AI Assistant:** This document is specifically for AI assistants working on the codebase

---

## For Future AI Assistants

This project has clean separation between interface (CLI/GUI) and processing logic. When adding features:

1. **Check if it's processing logic** → Add to core modules (AudioConverter, LunchBoxNamer, LunchBoxProcessor)
2. **Check if it's interface** → Add to CliProcessor or the GUI panels

The code is well-structured. Follow existing patterns. Don't reinvent wheels.

Good luck! 🎵
