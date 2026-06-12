# Roadmap

Plans beyond the current release. Timing is best-effort; items can move.

## 1.1

- **Background-threaded export** — move pack processing off the message thread
  (`juce::ThreadWithProgressWindow` or similar) so the UI stays responsive during
  export, with a progress bar and a cancel button. Today the window freezes for
  the duration of the export (a few seconds for typical packs of one-shots).
- **Register command shortcuts directly** — attach the `ApplicationCommandManager`
  key mappings as a key listener instead of relying on the macOS menu's key
  equivalents. Fixes secondary bindings on macOS (e.g. Cmd+P) and makes
  undo/clipboard/process shortcuts work at all on Linux/Windows, where there is
  no native menu bar.

## Later / unscheduled

- **Shared slot-assignment model** — extract the slot grid state out of the view
  components into a single observable model, making Pack and Bank pure views.
  Removes the cross-view sync layer and shrinks undo/session/clipboard code.
- **First-class Linux and Windows support** — both currently compile (Linux in CI)
  but are untested and unpackaged; clipboard import from the system file manager
  is macOS-only.

Suggestions welcome — open a [discussion](https://github.com/mattfromatlanta/lunch_box/discussions).
