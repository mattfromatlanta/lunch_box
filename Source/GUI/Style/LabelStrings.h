// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

// Centralised user-visible UI strings. All button labels, menu items, tooltips,
// context-menu entries, dialog text, and static labels live here so the whole
// catalogue can be reviewed in one place for consistency and tone.
//
// Runtime status/log lines (those concatenated with dynamic values like file
// counts or paths) are intentionally NOT in here — they belong with the code
// that builds them.
namespace LunchBoxLabels
{
    // ── Window / app shell ──────────────────────────────────
    inline constexpr const char* kConsoleTitle     = "Console";
    inline constexpr const char* kConsoleInitial   = "Ready to process samples...\n";

    // ── Top-bar tabs ────────────────────────────────────────
    inline constexpr const char* kTabPack          = "Pack";
    inline constexpr const char* kTabBank          = "Bank";
    inline constexpr const char* kTabCubbi         = "Cubbi";
    inline constexpr const char* kTabJammi         = "Jammi";

    inline constexpr const char* kTipTabPack       = "Pack view: 5-bank x 14-slot grid";
    inline constexpr const char* kTipTabBank       = "Bank view: focused single-bank waveform list";
    inline constexpr const char* kTipTabCubbi      = "Cubbi: percussive samples (Tab to toggle)";
    inline constexpr const char* kTipTabJammi      = "Jammi: chromatic / melodic samples (Tab to toggle)";

    // ── Footer buttons ──────────────────────────────────────
    inline constexpr const char* kBtnProcess       = "Pack";
    inline constexpr const char* kBtnFill          = "Fill";
    inline constexpr const char* kBtnClear         = "Clear";

    inline constexpr const char* kTipBtnProcess    = "Convert and export all samples to CHOMPI format (Cmd+Return)";
    inline constexpr const char* kTipBtnFill       = "Auto-fill empty slots from a folder";
    inline constexpr const char* kTipBtnClear      = "Clear all slots in the current view";

    // ── Slot tooltips ───────────────────────────────────────
    inline constexpr const char* kTipSlotPack      = "Drop a file, click to browse, or paste (Cmd+V)";
    inline constexpr const char* kTipSlotBank      = "Drop a file, double-click to browse, or paste (Cmd+V)";

    // ── Context menu (slot right-click) ────────────────────
    inline constexpr const char* kCtxBrowse        = "Browse for File...";
    inline constexpr const char* kCtxPreview       = "Preview";
    inline constexpr const char* kCtxClearSlot     = "Clear Slot";

    // ── File chooser titles ─────────────────────────────────
    inline constexpr const char* kChooseAudioFile        = "Select Audio File";
    inline constexpr const char* kChooseAutoFillFolder   = "Select Folder to Auto-Fill";
    inline constexpr const char* kChooseAutoFillBankFolder = "Select Folder to Auto-Fill From";
    inline constexpr const char* kChooseOutputFolder     = "Please select a home folder for your pack.";

    // ── Export / overwrite confirmation dialogs ────────────
    inline constexpr const char* kDlgFolderNotEmpty         = " is not empty.";
    inline constexpr const char* kDlgFolderNotEmptyBody     = "The export folder already exists:\n";
    inline constexpr const char* kDlgFolderNotEmptyWillDel  = " will be moved to the Trash and replaced.\n\nContinue?";
    inline constexpr const char* kDlgOutsideHomeTitle       = "Folder Outside Home Directory";
    inline constexpr const char* kDlgOutsideHomeBody        = "\" is outside your home directory.\n\n";
    inline constexpr const char* kDlgOutsideHomeSuffix      = "\" folder will be created there. Are you sure?";

    // ── Processing status lines ────────────────────────────
    inline constexpr const char* kStatusProcessStart    = "\n=== Starting CHOMPI Processing ===";
    inline constexpr const char* kStatusProcessComplete = "\n=== Processing Complete ===";
    inline constexpr const char* kStatusProcessFailed   = "\n=== Processing Failed ===";
    inline constexpr const char* kStatusErrorPrefix     = "Error: ";
    inline constexpr const char* kStatusCubbiLabel      = "  Cubbi: ";
    inline constexpr const char* kStatusJammiLabel      = "  Jammi: ";
    inline constexpr const char* kStatusTotalLabel      = "  Total: ";
    inline constexpr const char* kStatusDoublesLabel    = "  Doubles: ";
    inline constexpr const char* kStatusOutputLabel     = "  Output: ";
    inline constexpr const char* kStatusSamplesUnit     = " samples";
    inline constexpr const char* kStatusOptimizedSuffix = " optimized versions created";

    // ── PackNameOverlay dialog ─────────────────────────────
    inline constexpr const char* kDlgPackNamePrompt   = "Name your pack.";
    inline constexpr const char* kDlgContinue         = "Continue";
    inline constexpr const char* kDlgCancel           = "Cancel";

    // ── Preview panel ───────────────────────────────────────
    inline constexpr const char* kPreviewLabel        = "PREVIEW";
    inline constexpr const char* kPreviewLabelPrefix  = "PREVIEW  |  ";   // followed by filename
    inline constexpr const char* kBtnPlay             = "Play";
    inline constexpr const char* kBtnPause            = "Pause";
    inline constexpr const char* kBtnStop             = "Stop";

    // ── Waveform empty state ────────────────────────────────
    inline constexpr const char* kWaveformEmpty       = "select a folder to preview";

    // ── App menu bar ────────────────────────────────────────
    inline constexpr const char* kMenuFile            = "File";
    inline constexpr const char* kMenuEdit            = "Edit";
    inline constexpr const char* kMenuSettings        = "Settings";

    inline constexpr const char* kMenuChangeOutput    = "Change Output Folder...";
    inline constexpr const char* kMenuShowRuntimeLogs = "Show Runtime Logs";
    inline constexpr const char* kMenuShowLogFolder   = "Show Log Folder";
    inline constexpr const char* kMenuClearStatusLog  = "Clear Status Log";

    // ── ApplicationCommand info (name / description) ───────
    inline constexpr const char* kCmdUndo             = "Undo";
    inline constexpr const char* kCmdUndoDesc         = "Undo last action";
    inline constexpr const char* kCmdRedo             = "Redo";
    inline constexpr const char* kCmdRedoDesc         = "Redo last undone action";
    inline constexpr const char* kCmdCopy             = "Copy";
    inline constexpr const char* kCmdCopyDesc         = "Copy selected samples";
    inline constexpr const char* kCmdCut              = "Cut";
    inline constexpr const char* kCmdCutDesc          = "Cut selected samples";
    inline constexpr const char* kCmdPaste            = "Paste";
    inline constexpr const char* kCmdPasteDesc        = "Paste samples";
    inline constexpr const char* kCmdSelectAll        = "Select All";
    inline constexpr const char* kCmdSelectAllDesc    = "Select all slots";
    inline constexpr const char* kCmdOpenOutput       = "Open Export Home";
    inline constexpr const char* kCmdOpenOutputDesc   = "Open output folder in Finder";
    inline constexpr const char* kCmdProcess          = "Process Samples";
    inline constexpr const char* kCmdProcessDesc      = "Convert and export all samples";
    inline constexpr const char* kCmdFill             = "Fill Slots";
    inline constexpr const char* kCmdFillDesc         = "Auto-fill empty slots from a folder";
    inline constexpr const char* kCmdClear            = "Clear Slots";
    inline constexpr const char* kCmdClearDesc        = "Clear selected slots";
    inline constexpr const char* kCmdShowConsole      = "Show Console";
    inline constexpr const char* kCmdHideConsole      = "Hide Console";
    inline constexpr const char* kCmdToggleConsoleDesc= "Toggle the console window";
}
