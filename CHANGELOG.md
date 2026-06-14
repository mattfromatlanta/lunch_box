# Changelog

All notable changes to Lunch Box are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] — 2026-06-13

### Added
- **Peak normalization** — samples are normalized to -6 dB peak on export, on by
  default. The stereo balance is preserved (channel-linked peak), and silent files
  are left untouched. Disable from the CLI with `--no-normalize`.
- **Background export** — pack conversion now runs off the message thread, so the
  window stays responsive while a pack is written. The Pack button animates during
  export and the run can be cancelled with Esc. Output is written to a hidden
  staging folder and only swapped into place once complete, so an interrupted or
  cancelled export can never damage an existing pack.

### Changed
- **Single slot-assignment model** — Pack and Bank views are now pure views over one
  shared `PackModel`. This removes the cross-view sync layer and simplifies undo,
  session save/restore, and clipboard handling.
- **Keyboard shortcuts** are dispatched through the command manager's key mappings
  directly rather than relying on the macOS menu's key equivalents. This enables
  secondary bindings on macOS (e.g. Cmd+P alongside Cmd+Return) and makes
  undo/clipboard/process shortcuts work on Linux and Windows, where there is no
  native menu bar.

### Fixed
- Cleared packs are no longer incorrectly restored when switching categories.
- CLI tool installation (`--install`) now writes a LF-only shebang, fixing a
  `bad interpreter: /bin/bash^M` error.

## [1.0.0] — 2026-06-12

Initial public release.

### Added
- Dual-mode application: GUI and CLI from a single binary.
- Audio conversion to CHOMPI-compatible format (16-bit 48 kHz WAV) from WAV, AIFF,
  MP3, and FLAC input.
- CHOMPI naming convention (`cubbi_a1.wav` … `jammi_e14.wav`) with automatic
  bank/slot organization and bank-subfolder detection.
- Two GUI views: Pack (5-bank × 14-slot grid) and Bank (single-bank focus list).
- Waveform preview with click-to-play.
- Drag and drop of folders or individual files onto slots, plus paste from Finder.
- Undo / redo with 10-step history.
- Full keyboard navigation.
- Timestamped logging to the per-user log folder.
- Safe operations: original files are never modified; output is validated against
  the CHOMPI spec (channel count, 2-minute duration limit).

[1.1.0]: https://github.com/mattfromatlanta/lunch_box/releases/tag/v1.1.0
[1.0.0]: https://github.com/mattfromatlanta/lunch_box/releases/tag/v1.0.0
