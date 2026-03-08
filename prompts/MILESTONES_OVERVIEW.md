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

### Milestone 8: Individual Sample Management
**Status:** Complete (out-of-plan, 2026-03-08 session)
**Description:** Advanced mode with 5-bank × 14-slot grid (BankSlotComponent, BankRowComponent, BankEditorPanel). External file drop, internal slot-to-slot drag-reorder, right-click context menu, browse-for-file, clear, sort A-Z, auto-fill from folder. Mode toggle (Simple / Advanced). Category tabs (Cubbi / Jammi).

**Also completed in this session (out-of-plan):**
- **Output folder redesign:** Default `~/Desktop/chompis`, editable name field, decompose selected path into base+name, clean-before-export toggle.
- **Open Output button:** Activates after successful export; opens output folder in Finder (inside the folder).
- **Persistent folder memory:** Last cubbi, jammi, and output folders remembered across sessions via `ApplicationProperties`. Advanced-mode file pickers recall last cubbi/jammi location and start inside the folder.
- **Auto-Fill from Folder uses BankFolderParser:** Same bank-subfolder detection and overflow logging logic as the exporter. Overflow files reported to the status log.
- **Future note (not yet implemented):** Chompi Pack should eventually let the user select a default application for opening the output folder (e.g. Finder, a DAW, a file manager). This should be added as a dedicated future milestone or sub-task under M9 polish.

### Bank Focus View (unplanned addition, completed 2026-03-08)
**Status:** Complete
**Branch:** 0.3
**Description:** A third tab alongside Simple and Advanced. Shows one bank at a time as a vertical list of 14 slot rows, each rendering the sample's waveform. Cubbi/Jammi toggle and A–E bank selector sit above and to the left of the slot list respectively.

**Key design decisions:**
- Bank selector (A-E) is a left column (~36px)
- Cubbi/Jammi tabs are shared with Advanced mode — no re-render on switch; selection persists across mode changes
- No scrolling — 14 rows sized to fit on a 14" M1 MacBook Pro (820px window, ~12% vertical spare)
- Window grows to 820px when Bank tab is active (740px for other tabs)
- Status log hidden in Bank mode; replaced by single-line `bankStatusLabel`
- Each `FocusedSlotRow` owns a static `juce::AudioThumbnail`; shares `AudioFormatManager`/`AudioThumbnailCache` from `PreviewPanel`
- Drag-to-reorder within the active bank (no cross-bank drag)
- Clicking a filled slot starts preview playback; clicking empty stops it
- Data sync: Bank ↔ Advanced panels sync on tab switch via `getAssignments()` / `setSlot()`; export reuses `processFilesAdvanced()` after sync
- Re-entrant flush bug fixed: `isPopulating` flag prevents `flushRowsToStorage()` from overwriting new bank storage during `populateRowsFromStorage()`

**New files:**
- `Source/GUI/FocusedSlotRow.h/.cpp` — single 34px slot with waveform thumbnail, file drop, right-click menu, drag-to-reorder callbacks
- `Source/GUI/BankFocusPanel.h/.cpp` — full panel: bank selector, 14 rows, sort/fill/clear, internal `slots[2][5][14]` storage

**Modified files:**
- `Source/GUI/MainComponent.h/.cpp` — ViewMode enum (Simple/Advanced/Bank), third tab, shared category tabs, window resize, sync logic, `bankStatusLabel`
- `Source/GUI/BankEditorPanel.h/.cpp` — `onPreviewStop` callback, `notifyPreviewForSelection()`, `setSlotFile()`
- `Source/GUI/PreviewPanel.h/.cpp` — exposed `getFormatManager()` / `getThumbnailCache()`
- `CMakeLists.txt` — 4 new source files

**Not yet supported (future work):**
- Drag between banks in Bank mode

---

### UI Layout Overhaul (unplanned, completed 2026-03-09)
**Status:** Complete
**Branch:** 0.3
**Description:** Comprehensive layout and styling pass to establish a fixed, proportional UI.

**Changes:**
- Fixed window size: 1118×700px (no resize; `setResizable(false, false)`)
- Pack (Advanced) cells: 77×77px; Bank focus rows: 55px tall; both grids lock to 385px total height so the shared footer never moves
- Pack grid: removed row header label column (was 38px); bank letter embedded in each cell label (e.g. "A1", "E14") at 16pt
- Bank focus view: restructured to two columns of 7 rows (matching Pack grid height)
- Bank A–E selector buttons: styled with `SlotStyleLAF` (LookAndFeel_V4 subclass) to match slot cell appearance — rounded rect, same palette, toggle state border; 32px wide
- Nav row order: Cubbi/Jammi first, then Pack/Bank (left-to-right)
- Console refactored to a floating `DocumentWindow` (`ConsoleWindow`) rather than an inline panel; toggle opens/closes it
- Fill and Clear buttons moved from individual panels into a shared footer component; footer height is fixed regardless of active view (no button position shift on mode toggle)
- Output section: "OUTPUT FOLDER" label moved inline with the path button; full output path label removed from UI; name editor widened from 180px to 540px

**Key files modified:**
- `Source/GUI/MainComponent.h/.cpp` — window size, footer layout, ConsoleWindow, nav order, output section
- `Source/GUI/BankEditorPanel.h/.cpp` — removed button row, ROW_HEIGHT=77, ROW_GAP=0
- `Source/GUI/BankFocusPanel.h/.cpp` — SlotStyleLAF, two-column layout, triggerAutoFill/triggerClear, ROW_HEIGHT=55
- `Source/GUI/BankRowComponent.h/.cpp` — removed bankLabel + LABEL_WIDTH
- `Source/GUI/BankSlotComponent.h/.cpp` — bankLetter member, combined cell label at 16pt, updated palette
- `Source/GUI/FocusedSlotRow.cpp` — slot number font 16pt, rowEmptyBg matches Pack

---

### Out-of-Plan Work Completed (2026-03-07 session)
- **macOS menu bar:** File menu (Open Cubbi/Jammi/Output folders, Process Samples) and Settings menu (Show Runtime Logs toggle, Show Log Folder in Finder, Clear Status Log) via AppMenuBar / MenuBarModel.
- **Runtime log toggle:** Logger callback wired to GUI status window. Settings → Show Runtime Logs enables per-file debug output alongside high-level status messages.
- **Bank distribution fix:** Corrected unsorted-file distribution to fill empty banks before touching folder-bank overflow slots. Overflow files logged individually by name.
- **Processing result improvements:** Output count now reflects source samples processed (not base + double). Doubles verified and reported. Modal confirmation dialogs removed; all results go to the status log.
- **audio_dev_mcp tools:** Added `build_project`, `launch_project`, `get_repos` tools. Approved melatonin_inspector as a trusted dependency.

### Milestone 12 (partial): Code Review and Refactoring
**Status:** Substantially Complete (0.3 branch, 2026-03-07 session)
**Description:** Proactive code review and refactor pass on the full codebase.

**Completed:**
- `Source/Processing/` subfolder created; AudioConverter, BankFolderParser, ChompiNamer, ChompiProcessor moved there
- `SLOTS_PER_BANK`, `NUM_BANKS`, `MAX_FILES_PER_CATEGORY` promoted to `public static constexpr` on ChompiNamer — single source of truth (removed 3 duplicate definitions)
- `ChompiProcessor::runConversions()` private method extracts ~60-line duplicate conversion loop shared between `processCategory` and `processCategoryFromAssignments`
- `sortCellsRowMajor()` file-scope helper in BankEditorPanel replaces two identical lambdas
- `MainComponent::appendProcessingResult()` consolidates duplicate success-status block
- `BankSlotComponent::isSupportedAudioFile()` now delegates to `FileSystemHelper::getSupportedAudioExtensions()` (no more hardcoded extension list)
- `GuiProcessor::processFiles()` result counting bug fixed (was scanning output folder; now uses `ProcessingResult` from ChompiProcessor directly)
- All include paths updated for new folder layout; CMakeLists.txt updated

**Remaining (if desired):**
- clang-tidy static analysis pass
- Instruments memory/performance profiling
- Const-correctness and error-handling completeness audit

---

## Remaining Milestones 📋


---

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
**Priority:** Low (substantially completed in 0.3 refactor)
**Estimated Effort:** 1-2 days remaining
**Description:** The structural refactor (0.3 branch) completed the major code-quality pass. Remaining items are optional polish.

**Already done (do not re-do):**
- Source folder restructuring (Processing/ subfolder)
- Constants deduplication
- Duplicate logic extraction
- Include path consistency
- Bug fixes surfaced during review

**Remaining (optional):**
- clang-tidy static analysis pass
- Instruments memory/performance profiling
- Const-correctness audit
- Error-handling completeness check

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
DONE:  M1 → M2 → M3 → M4 → M5 → M6 → M7 → M8 → M10 → M12(major) → Bank Focus View → UI Layout Overhaul
                                                   ↓
                                              M9 (base theme done)
```

**Recommended remaining order:**
1. **M9 finish** — tooltips + animations (quick, 1-2 days)
2. **M11** — unit testing (before open source)
3. **M12 finish** — clang-tidy + profiling (optional polish, 1-2 days)
4. **M13** — open source release

## Version Roadmap

- **v1.1.0:** M1–M4 complete
- **v1.2.0:** M5 complete (format support + optimized samples)
- **v1.3.0:** M6 complete (bank folder organization)
- **v2.0.0:** M7, M8, M9 (base), M10 complete + menu bar, runtime logs, bank distribution fixes
- **v2.1.0:** M12 structural refactor (0.3 branch); Source/Processing/ layout, constants dedup, bug fixes
- **v2.3.0:** Current — UI Layout Overhaul (fixed window, unified grid heights, 2-col bank view, floating console, footer polish)
- **v2.2.0:** Bank Focus View complete (third tab, waveform rows, shared category tabs, re-entrant flush fix)
- **v2.3.0:** M9 finish + M11 (testing)
- **v3.0.0:** M13 (open source release)

## Effort Summary (Remaining)

| Milestone | Effort | Priority | Value |
|-----------|--------|----------|-------|
| M9 finish | 1-2 days | Low-Medium | Medium |
| M11 | 1-2 weeks | High | High |
| M12 finish | 1-2 days | Low | Low |
| M13 | 1-2 weeks | High | High |

**Remaining total:** ~3-5 weeks of development

---

**Last Updated:** 2026-03-09
**Current Version:** v2.3.0
**In Progress:** M9 finish → M11 (unit testing)
