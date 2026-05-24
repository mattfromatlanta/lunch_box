# CLAUDE.MD - AI Assistant Guide for Chompi Pack

**Purpose:** This document provides context and guidelines for AI assistants (Claude, etc.) working on the Chompi Pack project.

---

## Project Overview

**Chompi Pack** is a dual-mode (GUI + CLI) audio sample processor for the CHOMPI sampler hardware. It converts audio files to CHOMPI-compatible format (16-bit 48kHz WAV) and organizes them using the CHOMPI naming convention.

**Key Documentation:**
- [README.md](README.md) - User-facing project overview
- [HOW_TO.md](HOW_TO.md) - Comprehensive user guide
- [prompts/MILESTONES_OVERVIEW.md](prompts/MILESTONES_OVERVIEW.md) - Complete development roadmap

---

## Current Status

**Version:** 2.4.0
**Completed Milestones:** M1–M8, M10, M12 (major+second pass), Bank Focus View, UI Layout Overhaul, Keyboard/UX Refinements

**What Works:**
- ✅ CLI interface with full CHOMPI processing (wav/aiff/mp3/flac)
- ✅ GUI: Simple mode (folder drop zones), Advanced/Pack mode (5-bank × 14-slot grid), Bank mode (single-bank focus view)
- ✅ Audio conversion (16-bit 48kHz WAV output)
- ✅ Automatic bank assignment (A-E, 14 slots each)
- ✅ Dual categories (Cubbi / Jammi)
- ✅ Drag-drop folder selection and internal slot drag-to-reorder
- ✅ AIFF, MP3, FLAC input support
- ✅ Automatic `_double` (octave-up) generation
- ✅ Bank-subfolder detection (BankFolderParser)
- ✅ Waveform preview per slot (FocusedSlotRow, AudioThumbnail)
- ✅ Preview playback (click to play, move focus to stop)
- ✅ Persistent folder memory across sessions
- ✅ macOS menu bar (File / Edit / Settings menus)
- ✅ Source/Processing/ module layout; constants deduplication; code review passes
- ✅ Resizable window (default 525×900); unified Pack/Bank grid heights (385px each)
- ✅ Pack cells 77×77px with bank-letter prefixed labels (A1, E14); Bank rows 55px, 2-col layout
- ✅ Floating console window; shared fixed footer (Fill/Clear + output + process buttons)
- ✅ Full keyboard navigation: arrows + Shift-select, Tab (toggle Cubbi/Jammi), Esc (clear selection), Space (preview), Enter (browse), Delete/Backspace (clear)
- ✅ ApplicationCommandManager hotkeys: Cmd+Z/Shift+Z (undo/redo), Cmd+C/X/V (copy/cut/paste), Cmd+A (select all), Cmd+O (open output), Cmd+Return (process)
- ✅ 10-step undo/redo (full state snapshots, cross-view-mode)
- ✅ System clipboard paste from external apps (Finder file URLs, Sononym plain-text paths)

**See:** [prompts/MILESTONES_OVERVIEW.md](prompts/MILESTONES_OVERVIEW.md) for full roadmap.

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
    ChompiProcessor (Shared orchestrator)
            ↓
    ┌───────┼───────┐
    ↓       ↓       ↓
AudioConverter  ChompiNamer  Logger
    (Shared processing components)
```

**Key Insight:** Changes to AudioConverter, ChompiNamer, or ChompiProcessor automatically benefit both CLI and GUI. No code duplication.

### Module Responsibilities

| Module | Purpose | Key Files |
|--------|---------|-----------|
| **ChompiProcessor** | Orchestrates CHOMPI workflow | Processing/ChompiProcessor.h/cpp |
| **AudioConverter** | Format conversion (16-bit 48kHz) | Processing/AudioConverter.h/cpp |
| **ChompiNamer** | CHOMPI naming (cubbi_a1.wav, etc.) | Processing/ChompiNamer.h/cpp |
| **BankFolderParser** | Bank subfolder detection + assignment | Processing/BankFolderParser.h/cpp |
| **Logger** | Timestamped log files | Logger.h/cpp |
| **FileSystemHelper** | File utilities, format extension list | FileSystemHelper.h/cpp |
| **CliProcessor** | CLI interface and argument parsing | CLI/CliProcessor.h/cpp |
| **GuiProcessor** | GUI-processing bridge | GUI/GuiProcessor.h/cpp |
| **MainComponent** | GUI layout, tabs, mode switching | GUI/MainComponent.h/cpp |
| **BankEditorPanel** | Advanced: 5×14 grid per category | GUI/BankEditorPanel.h/cpp |
| **BankFocusPanel** | Bank: single-bank 14-row focus view | GUI/BankFocusPanel.h/cpp |
| **FocusedSlotRow** | Bank: single slot row with waveform | GUI/FocusedSlotRow.h/cpp |
| **PreviewPanel** | Waveform + audio playback | GUI/PreviewPanel.h/cpp |
| **MainWindow** | Application window | GUI/MainWindow.h/cpp |

### Data Flow

**CLI Mode:**
```
Command line args → CliProcessor → AudioConfiguration → ChompiProcessor → Output files
```

**GUI Mode:**
```
Folder selections → GuiProcessor → AudioConfiguration → ChompiProcessor → Output files
```

**Shared:** Both modes use `AudioConfiguration` struct and call the same `ChompiProcessor::processSamples()`.

---

## CHOMPI Sampler Context

**Critical to understand:** The CHOMPI sampler has specific requirements that drive all design decisions.

### Bank Structure
- **2 categories:** cubbi (percussive), jammi (chromatic)
- **5 banks per category:** A, B, C, D, E
- **14 slots per bank:** Numbered 1-14
- **Total capacity:** 70 samples per category (hardware limit)
- **Output files per category:** 140 (70 base + 70 optimized)

### Naming Convention
```
{category}_{bank}{slot}.wav              # Base sample
{category}_{bank}{slot}_double.wav       # Optimized sample (pitched up)

Examples:
  cubbi_a1.wav          (Cubbi, Bank A, Slot 1 - base)
  cubbi_a1_double.wav   (Cubbi, Bank A, Slot 1 - optimized)
  cubbi_a14.wav         (Cubbi, Bank A, Slot 14 - base)
  cubbi_a14_double.wav  (Cubbi, Bank A, Slot 14 - optimized)
  cubbi_b1.wav          (Cubbi, Bank B, Slot 1 - base)
  cubbi_b1_double.wav   (Cubbi, Bank B, Slot 1 - optimized)
  jammi_e14.wav         (Jammi, Bank E, Slot 14 - base)
  jammi_e14_double.wav  (Jammi, Bank E, Slot 14 - optimized)
```

### Optimized Samples
**Critical CHOMPI Feature:** The CHOMPI hardware automatically creates "optimized" versions of samples if not present in the library. An optimized sample is:
- Pitched up one octave (double playback speed)
- Half the duration of the base sample
- Saved with `_double` suffix
- Generated to enable pitch-shifting on the hardware

**Chompi Pack Behavior:** As of Milestone 5, Chompi Pack proactively generates both base and optimized versions for every input sample, creating complete CHOMPI libraries without requiring the hardware to generate files.

**Duration Limits:**
- Base samples: Maximum 2 minutes (120 seconds)
- Optimized samples: Maximum 1 minute (60 seconds) - automatically half of base

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
- Classes: `PascalCase` (e.g., `ChompiProcessor`)
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

### Testing

**Current Status:**
- ✅ `test_chompi_namer` exists (unit tests for ChompiNamer)
- ⚠️  Limited coverage on other modules

**When adding features:**
- Add unit tests for new functions
- Add integration tests for new workflows
- Update existing tests if behavior changes

**See:** [prompts/milestone_11_plan.md](prompts/milestone_11_plan.md) for comprehensive testing plans.

### Build System

**CMake Configuration:**
- Console app changed to GUI app in Milestone 4
- JUCE modules: `juce_core`, `juce_audio_formats`, `juce_audio_basics`, `juce_gui_basics`, `juce_gui_extra`
- Target name: `chompi_pack`
- Test target: `test_chompi_namer`

**Build Process:**
```bash
mkdir -p build && cd build
cmake ..
make
```

**Output:**
- macOS: `chompi_pack_artefacts/Chompi Pack.app`
- Tests: `test_chompi_namer_artefacts/test_chompi_namer`

---

## Working with the Codebase

### Common Tasks

**Adding a new processing feature:**
1. Implement in appropriate module (AudioConverter, ChompiNamer, etc.)
2. Update ChompiProcessor if orchestration changes
3. No changes needed to CliProcessor or GuiProcessor (they delegate to ChompiProcessor)
4. Add tests
5. Update documentation

**Adding a new GUI feature:**
1. Modify MainComponent.h/cpp
2. Add UI elements in constructor
3. Update `resized()` for layout
4. Add callbacks/handlers
5. Test in both modes (with/without feature)

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
- Always generate both base and optimized (_double) versions
- Never compromise quality for speed
- Maximum 2-minute duration per sample

**File safety:**
- Never modify input files
- Never overwrite without warning
- Always log operations
- Handle disk full gracefully

---

## Milestone Plans

**All future development is documented in detailed milestone plans:**

| Milestone | Plan Document | Status |
|-----------|---------------|--------|
| M1-M4 | [milestone_1-4_plan.md](prompts/) | ✅ Complete |
| M5 | [milestone_5_plan.md](prompts/milestone_5_plan.md) | ✅ Complete |
| M6 | [milestone_6_plan.md](prompts/milestone_6_plan.md) | ✅ Complete |
| M7 | [milestone_7_plan.md](prompts/milestone_7_plan.md) | ✅ Complete |
| M8 | [milestone_8_plan.md](prompts/milestone_8_plan.md) | ✅ Complete |
| M9 | [milestone_9_plan.md](prompts/milestone_9_plan.md) | 🔄 Base theme done; tooltips/animations remaining |
| M10 | [milestone_10_plan.md](prompts/milestone_10_plan.md) | ✅ Complete |
| M11 | [milestone_11_plan.md](prompts/milestone_11_plan.md) | 📋 Planned |
| M12 | [milestone_12_plan.md](prompts/milestone_12_plan.md) | 🔄 Major refactor done; clang-tidy remaining |
| M13 | [milestone_13_plan.md](prompts/milestone_13_plan.md) | 📋 Planned |

**Before implementing any feature:** Read the relevant milestone plan. Each plan includes:
- Detailed technical architecture
- Implementation steps (phases)
- Success criteria
- Code examples
- Testing requirements

**See:** [prompts/MILESTONES_OVERVIEW.md](prompts/MILESTONES_OVERVIEW.md) for roadmap and dependencies.

---

## Key Files Reference

### Essential Reading

| File | Purpose | When to Read |
|------|---------|--------------|
| [README.md](README.md) | Project overview, quick start | Understanding project scope |
| [HOW_TO.md](HOW_TO.md) | User guide, examples | Understanding user workflows |
| [Source/Main.cpp](Source/Main.cpp) | Entry point, mode detection | Understanding app initialization |
| [Source/Processing/ChompiProcessor.cpp](Source/Processing/ChompiProcessor.cpp) | Core processing logic | Understanding CHOMPI processing |
| [Source/AudioConfiguration.h](Source/AudioConfiguration.h) | Shared data structures | Understanding data flow |
| [Source/GUI/MainComponent.cpp](Source/GUI/MainComponent.cpp) | GUI layout, tabs, mode switching | Understanding GUI architecture |
| [Source/GUI/BankFocusPanel.cpp](Source/GUI/BankFocusPanel.cpp) | Bank focus view | Understanding Bank tab |

### Documentation Map

```
chompi_pack/
├── README.md                          # User-facing project overview
├── HOW_TO.md                          # Comprehensive user guide
├── CLAUDE.md                          # This file (AI assistant guide)
│
├── prompts/                           # Development planning
│   ├── MILESTONES_OVERVIEW.md         # Complete roadmap (always up to date)
│   ├── milestone_1_plan.md            # Audio scanner (complete)
│   ├── milestone_2_plan.md            # Format conversion (complete)
│   ├── milestone_3_plan.md            # CHOMPI naming (complete)
│   ├── milestone_4_plan.md            # GUI application (complete)
│   ├── milestone_5_plan.md            # Format support AIFF/MP3/FLAC (complete)
│   ├── milestone_6_plan.md            # Bank folder organization (complete)
│   ├── milestone_7_plan.md            # Drag and drop (complete)
│   ├── milestone_8_plan.md            # Individual sample management (complete)
│   ├── milestone_9_plan.md            # GUI polish (base done, tooltips remaining)
│   ├── milestone_10_plan.md           # Waveform/preview (complete)
│   ├── milestone_11_plan.md           # Testing (planned)
│   ├── milestone_12_plan.md           # Code review (major pass done)
│   └── milestone_13_plan.md           # Open source prep (planned)
│
├── Source/                            # C++ source code
│   ├── Main.cpp                       # Entry point
│   ├── AudioConfiguration.h           # Shared config struct
│   ├── FileSystemHelper.h/cpp         # File utilities, format extension list
│   ├── Logger.h/cpp                   # Logging system
│   ├── CLI/                           # CLI interface
│   │   ├── CliProcessor.h/cpp         # CLI argument parsing + processing
│   ├── GUI/                           # GUI interface
│   │   ├── MainWindow.h/cpp           # Application window
│   │   ├── MainComponent.h/cpp        # Main UI (tabs, mode switching, sync)
│   │   ├── GuiProcessor.h/cpp         # GUI-processing bridge
│   │   ├── BankEditorPanel.h/cpp      # Advanced: 5×14 grid per category
│   │   ├── BankRowComponent.h/cpp     # Advanced: one bank row (14 slots)
│   │   ├── BankSlotComponent.h/cpp    # Advanced: individual slot cell
│   │   ├── BankFocusPanel.h/cpp       # Bank: single-bank focus view
│   │   ├── FocusedSlotRow.h/cpp       # Bank: single slot row with waveform
│   │   ├── PreviewPanel.h/cpp         # Waveform + audio playback
│   │   └── FolderDropZone.h/cpp       # Simple: folder drop zone
│   └── Processing/                    # Core processing modules
│       ├── ChompiProcessor.h/cpp      # Processing orchestrator
│       ├── AudioConverter.h/cpp       # Format conversion
│       ├── ChompiNamer.h/cpp          # CHOMPI naming + constants
│       └── BankFolderParser.h/cpp     # Bank subfolder detection
│
├── dev_assets/                        # Test samples
└── logs/                              # Operation logs
```

---

## Common Pitfalls

### 1. Breaking Abstraction Boundaries
❌ **Wrong:** GUI calling AudioConverter directly
✅ **Right:** GUI calls GuiProcessor, which calls ChompiProcessor, which calls AudioConverter

### 2. Duplicating Processing Logic
❌ **Wrong:** Implementing processing differently in CLI vs GUI
✅ **Right:** Both interfaces call the same ChompiProcessor code

### 3. Ignoring CHOMPI Constraints
❌ **Wrong:** Supporting 16 slots per bank or custom naming
✅ **Right:** Always 14 slots, always CHOMPI naming convention

### 4. Not Updating Both Modes
❌ **Wrong:** Adding feature to GUI only
✅ **Right:** Ensure feature available (or gracefully absent) in both modes

### 5. Assuming Single Platform
❌ **Wrong:** Using macOS-specific paths or APIs
✅ **Right:** Use JUCE cross-platform abstractions

---

## Testing Changes

### Before Committing

```bash
# Build
cd build
make

# Test GUI
open "chompi_pack_artefacts/Chompi Pack.app"
# Verify: folder selection, processing, output

# Test CLI
"chompi_pack_artefacts/Chompi Pack.app/Contents/MacOS/Chompi Pack" \
  --cubbi ../dev_assets/cubbi \
  --jammi ../dev_assets/jammi
# Verify: same output as GUI

# Run unit tests
./test_chompi_namer_artefacts/test_chompi_namer
```

### Regression Checks

- ✅ CLI still processes test folders correctly
- ✅ GUI opens without crashes
- ✅ Output files are valid 16-bit 48kHz WAV
- ✅ CHOMPI naming correct (cubbi_a1.wav, etc.)
- ✅ Logs created in logs/ directory

---

## Project Philosophy

### Design Principles

1. **User convenience over features**
   - Simple mode should stay simple
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

1. **Read relevant milestone plan** - Likely already documented
2. **Check existing code** - Pattern probably exists elsewhere
3. **Review git history** - See how similar features were added
4. **Check JUCE docs** - Framework has most functionality needed

### JUCE Resources

- **Tutorials:** https://docs.juce.com/master/tutorial_getting_started.html
- **API Docs:** https://docs.juce.com/master/
- **Forum:** https://forum.juce.com/

### Project Specifics

- **CHOMPI Hardware:** https://creditor.technology (for understanding target device)
- **User Workflows:** See HOW_TO.md examples
- **Architecture Decisions:** See milestone plan "Technical Architecture" sections

---

## Quick Reference

### Build Commands
```bash
# Full rebuild
cd build && cmake .. && make

# Just compile
cd build && make

# Run GUI
open "chompi_pack_artefacts/Chompi Pack.app"

# Run CLI
"chompi_pack_artefacts/Chompi Pack.app/Contents/MacOS/Chompi Pack" --help

# Run tests
./test_chompi_namer_artefacts/test_chompi_namer
```

### Common File Paths
```
Source/GUI/MainComponent.cpp           # Main GUI logic (tabs, mode switching)
Source/GUI/BankFocusPanel.cpp          # Bank tab panel
Source/GUI/BankEditorPanel.cpp         # Advanced tab grid
Source/CLI/CliProcessor.cpp            # CLI logic
Source/Processing/ChompiProcessor.cpp  # Shared processing orchestrator
Source/Processing/AudioConverter.cpp   # Audio conversion
Source/Processing/ChompiNamer.cpp      # CHOMPI naming + constants
prompts/MILESTONES_OVERVIEW.md         # Always-current roadmap
prompts/milestone_X_plan.md            # Individual feature plans
```

### Key Constants
```cpp
MAX_SLOTS_PER_CATEGORY = 70        // CHOMPI hardware bank limit
SLOTS_PER_BANK = 14                // CHOMPI structure
NUM_BANKS = 5                      // A, B, C, D, E
TARGET_SAMPLE_RATE = 48000.0       // CHOMPI requirement
TARGET_BIT_DEPTH = 16              // CHOMPI requirement
MAX_DURATION_SECONDS = 120.0       // Maximum 2 minutes per sample
OUTPUT_FILES_PER_INPUT = 2         // Base + optimized (_double)
MAX_OUTPUT_FILES_PER_CATEGORY = 140 // 70 base + 70 optimized
```

---

## Version History

- **v2.3.0** (Current) - UI Layout Overhaul: resizable window (default 525×900), unified grid heights, 2-col bank view, floating console, footer, output section polish
- **v2.2.0** - Bank Focus View (third tab, waveform rows, shared category tabs)
- **v2.1.0** - M12 structural refactor: Source/Processing/ layout, constants deduplication, bug fixes
- **v2.0.0** - M7, M8, M9 (base), M10 complete; menu bar, runtime logs, bank distribution fixes
- **v1.3.0** - M6: Bank-subfolder detection (BankFolderParser)
- **v1.2.0** - M5: AIFF/MP3/FLAC support + `_double` generation
- **v1.1.0** - M4: GUI application with folder selection and output
- **v1.0.0** - M1–M3: CLI with CHOMPI processing, naming, conversion

**Next:** M9 finish (tooltips/animations) → M11 (unit testing) → M13 (open source)

---

**Author:** [Matt from Atlanta](https://github.com/mattfromatlanta) — [Bluesky](https://bsky.app/profile/mattfromatlanta.bsky.social) · [Bandcamp](https://angsttanks.bandcamp.com/)
**AI Assistant:** This document is specifically for AI assistants working on the codebase

---

## For Future AI Assistants

This project has clean separation between interface (CLI/GUI) and processing logic. When adding features:

1. **Check if it's processing logic** → Add to core modules (AudioConverter, ChompiNamer, ChompiProcessor)
2. **Check if it's interface** → Add to CliProcessor or GuiProcessor

The milestone plans are comprehensive. Read them before implementing anything. They contain:
- Complete technical architecture
- Step-by-step implementation guidance
- Success criteria
- Testing requirements

The code is well-structured. Follow existing patterns. Don't reinvent wheels.

Good luck! 🎵
