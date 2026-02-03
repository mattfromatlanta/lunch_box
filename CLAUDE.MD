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

**Version:** 1.1.0
**Completed Milestones:** 1-4 (+ Milestone 4 Addendum)

**What Works:**
- ✅ CLI interface with full CHOMPI processing
- ✅ GUI application with folder selection
- ✅ Audio conversion (16-bit 48kHz WAV output)
- ✅ Automatic bank assignment (A-E, 14 slots each)
- ✅ Dual categories (cubbi/jammi)
- ✅ Drag-drop folder selection (Milestone 4 Addendum)

**See:** [MILESTONE_4_SUMMARY.md](MILESTONE_4_SUMMARY.md) and [MILESTONE_4_ADDENDUM.md](MILESTONE_4_ADDENDUM.md)

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
| **ChompiProcessor** | Orchestrates CHOMPI workflow | ChompiProcessor.h/cpp |
| **AudioConverter** | Format conversion (16-bit 48kHz) | AudioConverter.h/cpp |
| **ChompiNamer** | CHOMPI naming (cubbi_a1.wav, etc.) | ChompiNamer.h/cpp |
| **Logger** | Timestamped log files | Logger.h/cpp |
| **FileSystemHelper** | File utilities | FileSystemHelper.h/cpp |
| **CliProcessor** | CLI interface and argument parsing | CLI/CliProcessor.h/cpp |
| **GuiProcessor** | GUI-processing bridge | GUI/GuiProcessor.h/cpp |
| **MainComponent** | GUI layout and controls | GUI/MainComponent.h/cpp |
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
- **Total capacity:** 70 samples per category

### Naming Convention
```
{category}_{bank}{slot}.wav

Examples:
  cubbi_a1.wav    (Cubbi, Bank A, Slot 1)
  cubbi_a14.wav   (Cubbi, Bank A, Slot 14)
  cubbi_b1.wav    (Cubbi, Bank B, Slot 1)
  jammi_e14.wav   (Jammi, Bank E, Slot 14)
```

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
- Never compromise quality for speed

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
| M5 | [milestone_5_plan.md](prompts/milestone_5_plan.md) | 📋 Planned |
| M6 | [milestone_6_plan.md](prompts/milestone_6_plan.md) | 📋 Planned |
| M7 | [milestone_7_plan.md](prompts/milestone_7_plan.md) | 📋 Planned |
| M8 | [milestone_8_plan.md](prompts/milestone_8_plan.md) | 📋 Planned |
| M9 | [milestone_9_plan.md](prompts/milestone_9_plan.md) | 📋 Planned |
| M10 | [milestone_10_plan.md](prompts/milestone_10_plan.md) | 📋 Planned |
| M11 | [milestone_11_plan.md](prompts/milestone_11_plan.md) | 📋 Planned |
| M12 | [milestone_12_plan.md](prompts/milestone_12_plan.md) | 📋 Planned |
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
| [Source/ChompiProcessor.cpp](Source/ChompiProcessor.cpp) | Core processing logic | Understanding CHOMPI processing |
| [Source/AudioConfiguration.h](Source/AudioConfiguration.h) | Shared data structures | Understanding data flow |

### Documentation Map

```
chompi_pack/
├── README.md                          # User-facing project overview
├── HOW_TO.md                          # Comprehensive user guide
├── CLAUDE.MD                          # This file (AI assistant guide)
├── MILESTONE_4_SUMMARY.md             # GUI implementation summary
├── MILESTONE_4_ADDENDUM.md            # Output folder selection addition
│
├── prompts/                           # Development planning
│   ├── MILESTONES_OVERVIEW.md         # Complete roadmap
│   ├── milestone_1_plan.md            # Audio scanner (complete)
│   ├── milestone_2_plan.md            # Format conversion (complete)
│   ├── milestone_3_plan.md            # CHOMPI naming (complete)
│   ├── milestone_4_plan.md            # GUI application (complete)
│   ├── milestone_5_plan.md            # Format support (AIFF/MP3/FLAC)
│   ├── milestone_6_plan.md            # Bank folder organization
│   ├── milestone_7_plan.md            # Drag and drop
│   ├── milestone_8_plan.md            # Individual sample management
│   ├── milestone_9_plan.md            # GUI polish
│   ├── milestone_10_plan.md           # Waveform/preview
│   ├── milestone_11_plan.md           # Testing review
│   ├── milestone_12_plan.md           # Code review
│   └── milestone_13_plan.md           # Open source prep
│
├── Source/                            # C++ source code
│   ├── Main.cpp                       # Entry point
│   ├── AudioConfiguration.h           # Shared config struct
│   ├── CLI/                           # CLI interface
│   │   ├── CliProcessor.h
│   │   └── CliProcessor.cpp
│   ├── GUI/                           # GUI interface
│   │   ├── MainWindow.h/cpp           # Application window
│   │   ├── MainComponent.h/cpp        # Main UI
│   │   └── GuiProcessor.h/cpp         # GUI-processing bridge
│   ├── AudioConverter.h/cpp           # Format conversion
│   ├── ChompiNamer.h/cpp              # CHOMPI naming
│   ├── ChompiProcessor.h/cpp          # Processing orchestrator
│   ├── Logger.h/cpp                   # Logging system
│   └── FileSystemHelper.h/cpp         # File utilities
│
├── test_audio/                        # Test samples
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
  --cubbi ../test_audio/cubbi \
  --jammi ../test_audio/jammi
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
Source/GUI/MainComponent.cpp       # Main GUI logic
Source/CLI/CliProcessor.cpp        # CLI logic
Source/ChompiProcessor.cpp         # Shared processing
Source/AudioConverter.cpp          # Audio conversion
Source/ChompiNamer.cpp             # CHOMPI naming
prompts/milestone_X_plan.md        # Feature plans
```

### Key Constants
```cpp
MAX_FILES_PER_CATEGORY = 70        // CHOMPI limit
SLOTS_PER_BANK = 14                // CHOMPI structure
NUM_BANKS = 5                      // A, B, C, D, E
TARGET_SAMPLE_RATE = 48000.0       // CHOMPI requirement
TARGET_BIT_DEPTH = 16              // CHOMPI requirement
```

---

## Version History

- **v1.1.0** (Current) - GUI application with output folder selection
- **v1.0.0** - Initial release with CLI and CHOMPI processing
- **Milestones 1-3** - Foundation (scanner, converter, namer)
- **Milestone 4** - GUI application

**Next:** Milestone 5 (Format support: AIFF, MP3, FLAC)

---

**Last Updated:** February 2, 2026
**Maintainer:** Matt from Atlanta
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
