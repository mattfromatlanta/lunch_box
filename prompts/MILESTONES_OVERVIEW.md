# Chompi Pack - Milestones Overview

**Project Roadmap: Milestones 1-13**

---

## Completed Milestones ✅

### Milestone 1: Audio File Scanner
**Status:** Complete
**Description:** CLI app scanning folders for WAV files and reporting metadata.

### Milestone 2: Audio Format Conversion
**Status:** Complete
**Description:** Convert WAV files to 16-bit 48kHz while preserving channel configuration.

### Milestone 3: CHOMPI Sampler Naming Schema
**Status:** Complete
**Description:** CHOMPI naming convention (cubbi_a1.wav, etc.) with automatic bank assignment.

### Milestone 4: GUI Application
**Status:** Complete (+ Addendum)
**Description:** Graphical interface with folder selection, processing controls, status display, and output folder selection.

### Milestone 5: Additional Input Format Support + Optimized Sample Generation
**Status:** Complete
**Description:** AIFF, MP3, FLAC input support. Automatic `_double` (octave-up) generation for every sample. Duration validation (max 2 minutes). Complete CHOMPI library output (base + optimized).

### Milestone 6: Bank-Specific Folder Organization
**Status:** Complete
**Description:** Detect bank subfolders (A–E, bank_a, Bank A, etc.) and assign samples to matching banks. Unsorted files fill empty banks first, then remaining space in folder-banks. Overflow files logged by name. BankFolderParser module.

### Milestone 7: Drag and Drop Folder Selection
**Status:** Complete
**Description:** FolderDropZone component with FileDragAndDropTarget. Folders dragged from Finder populate cubbi, jammi, or output zones with visual hover feedback. Button selection still available.

### Milestone 9 (partial): GUI Look and Feel — Base Theme
**Status:** Substantially Complete
**Description:** Dark navy color palette, monospaced status log, section labels, typography hierarchy, accent-colored process button, styled drop zones. Remaining: tooltips, transition animations.

### Milestone 10: Sample Waveform and Play Preview
**Status:** Complete
**Description:** WaveformDisplay (AudioThumbnail), AudioPreviewPlayer (AudioTransportSource + AudioDeviceManager), PreviewPanel with play/pause/stop controls and file info. Auto-previews first file when a folder is selected.

### Out-of-Plan Work Completed (2026-03-08 session)
- **Milestone 8 (Individual Sample Management):** Advanced mode with 5-bank × 14-slot grid (BankSlotComponent, BankRowComponent, BankEditorPanel). External file drop, internal slot-to-slot drag-reorder, right-click context menu, browse-for-file, clear, sort A-Z, auto-fill from folder. Mode toggle (Simple / Advanced). Category tabs (Cubbi / Jammi).
- **Output folder redesign:** Default `~/Desktop/chompis`, editable name field, decompose selected path into base+name, clean-before-export toggle.
- **Open Output button:** Activates after successful export; opens output folder in Finder (inside the folder).
- **Persistent folder memory:** Last cubbi, jammi, and output folders remembered across sessions via `ApplicationProperties`. Advanced-mode file pickers recall last cubbi/jammi location and start inside the folder.
- **Auto-Fill from Folder uses BankFolderParser:** Same bank-subfolder detection and overflow logging logic as the exporter. Overflow files reported to the status log.
- **Future note (not yet implemented):** Chompi Pack should eventually let the user select a default application for opening the output folder (e.g. Finder, a DAW, a file manager). This should be added as a dedicated future milestone or sub-task under M9 polish.

### Out-of-Plan Work Completed (2026-03-07 session)
- **macOS menu bar:** File menu (Open Cubbi/Jammi/Output folders, Process Samples) and Settings menu (Show Runtime Logs toggle, Show Log Folder in Finder, Clear Status Log) via AppMenuBar / MenuBarModel.
- **Runtime log toggle:** Logger callback wired to GUI status window. Settings → Show Runtime Logs enables per-file debug output alongside high-level status messages.
- **Bank distribution fix:** Corrected unsorted-file distribution to fill empty banks before touching folder-bank overflow slots. Overflow files logged individually by name.
- **Processing result improvements:** Output count now reflects source samples processed (not base + double). Doubles verified and reported. Modal confirmation dialogs removed; all results go to the status log.
- **audio_dev_mcp tools:** Added `build_project`, `launch_project`, `get_repos` tools. Approved melatonin_inspector as a trusted dependency.

---

## Remaining Milestones 📋

### Milestone 9 (remainder): GUI Polish
**Priority:** Low-Medium
**Estimated Effort:** 1-2 days
**Description:** Complete the remaining GUI polish items not covered in the base theme work.

**Remaining items:**
- Tooltips on all controls (folder zones, process button, status area)
- Hover/focus transition animations on drop zones and buttons
- Any remaining spacing or alignment refinements

**Already done (do not re-implement):**
- Dark color palette and color theme system
- Typography hierarchy and section labels
- Drop zone styling and hover highlight
- Menu bar and settings
- Monospaced status log

---

### Milestone 8: Individual Sample Management
**Priority:** Medium-High
**Estimated Effort:** 1-2 weeks
**Description:** Advanced mode with a visual bank editor for slot-by-slot sample management alongside the existing folder-based simple mode.

**Key Features:**
- Toggle between Simple Mode (current folder-based) and Advanced Mode (slot editor)
- Bank editor showing all 70 slots across 5 banks (A–E, 14 slots each)
- Drag individual audio files from Finder into specific slots
- Reorder samples within and between banks via drag
- Browse file into a specific slot via right-click / context menu
- Auto-fill, clear bank, and sort operations
- Per-slot waveform thumbnail (reuses WaveformDisplay)
- Per-slot preview on click (reuses AudioPreviewPlayer)

**UI Structure:**
```
Simple Mode (default):    [Cubbi Zone] [Jammi Zone] [Output Zone] → Process
Advanced Mode:            [Bank Editor Panel — 5 banks × 14 slots per category]
                          [Preview Panel] → Process
```

**Technical Changes:**
- BankSlotComponent — single slot (waveform thumb, filename, drag target)
- BankRowComponent — 14 slots for one bank
- BankEditorPanel — 5 banks, tabbed cubbi/jammi
- Mode toggle button in MainComponent
- BankEditorPanel feeds directly into ChompiProcessor (same backend)

**Dependencies:** PreviewPanel and WaveformDisplay already complete (M10).

---

### Milestone 11: Unit Testing
**Priority:** High (before open source)
**Estimated Effort:** 1-2 weeks
**Description:** Comprehensive unit test coverage for all core modules.

**Test Scope:**
- ChompiNamer — bank calculations, slot numbering, naming
- AudioConverter — format conversion, `_double` generation, duration validation
- BankFolderParser — folder detection, bank assignment, unsorted distribution, overflow
- FileSystemHelper — file discovery, extension filtering
- Logger — file creation, formatting, callback
- ChompiProcessor — end-to-end integration

**Goals:**
- 80%+ code coverage on core modules
- Unit tests run in <100ms each
- Integration tests run in <30s
- CI/CD pipeline (GitHub Actions)

---

### Milestone 12: Code Review and Refactoring
**Priority:** High (before open source)
**Estimated Effort:** 1-2 weeks
**Description:** Code review pass for quality, consistency, and maintainability.

**Review Areas:**
- Naming conventions (consistent PascalCase/camelCase throughout)
- Const correctness
- Error handling completeness
- Memory management (ownership, RAII)
- Function length and complexity
- Dead code removal
- Documentation / inline comments where logic is non-obvious

**Tools:**
- clang-tidy (static analysis)
- Instruments (memory/performance profiling on macOS)

---

### Milestone 13: Open Source Publication Preparation
**Priority:** High (final step)
**Estimated Effort:** 1-2 weeks
**Description:** Prepare for public release.

**Requirements:**
- MIT license + license headers in all source files
- Professional README (user-facing, with screenshots)
- CONTRIBUTING.md
- Issue and PR templates
- CI/CD pipeline building on macOS (GitHub Actions)
- Release build (signed .app or DMG for macOS)

**Documentation:**
- Updated HOW_TO.md
- Developer guide (architecture overview)
- Changelog

---

## Current Status and Next Steps

```
DONE:  M1 → M2 → M3 → M4 → M5 → M6 → M7 → M10
                                        ↓
                                   M9 (base theme done)
```

**Recommended remaining order:**
1. **M9 finish** — tooltips + animations (quick, 1-2 days)
2. **M8** — individual sample management (the major remaining feature)
3. **M11** — unit testing (before adding more complexity)
4. **M12** — code review and refactor
5. **M13** — open source release

## Version Roadmap

- **v1.1.0:** M1–M4 complete
- **v1.2.0:** M5 complete (format support + optimized samples)
- **v1.3.0:** M6 complete (bank folder organization)
- **v2.0.0:** Current — M7, M9 (base), M10 complete + menu bar, runtime logs, bank distribution fixes
- **v2.1.0:** M9 finish + M8 (advanced slot editor)
- **v2.2.0:** M11 + M12 (testing + review)
- **v3.0.0:** M13 (open source release)

## Effort Summary (Remaining)

| Milestone | Effort | Priority | Value |
|-----------|--------|----------|-------|
| M9 finish | 1-2 days | Low-Medium | Medium |
| M8 | 1-2 weeks | Medium-High | Very High |
| M11 | 1-2 weeks | High | High |
| M12 | 1-2 weeks | High | High |
| M13 | 1-2 weeks | High | High |

**Remaining total:** ~5-9 weeks of development

---

**Last Updated:** 2026-03-07
**Current Version:** v2.0.0
**Next Milestone:** M9 finish (tooltips/animations) → M8 (individual sample management)
