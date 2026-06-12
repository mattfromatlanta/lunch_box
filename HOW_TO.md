# Lunch Box - How to Use

**Lunch Box** prepares audio samples for the CHOMPI sampler. It converts audio files
(WAV, AIFF, MP3, FLAC) to the CHOMPI standard format (16-bit 48kHz WAV) and names them
using the CHOMPI naming convention. It runs as a desktop app (GUI) or from the command
line (CLI) — both produce identical output.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Using the App (GUI)](#using-the-app-gui)
3. [Using the Command Line (CLI)](#using-the-command-line-cli)
4. [Understanding CHOMPI Banks](#understanding-chompi-banks)
5. [Audio Formats](#audio-formats)
6. [Tips & Best Practices](#tips--best-practices)
7. [Troubleshooting](#troubleshooting)

---

## Quick Start

### Building the Project

```bash
cd lunch_box

# Clone JUCE alongside the repo (or pass -DJUCE_DIR=/path/to/JUCE to cmake)
git clone --branch 8.0.12 --depth 1 https://github.com/juce-framework/JUCE.git ../JUCE

mkdir -p build && cd build
cmake ..
make
```

On macOS this produces the app bundle `build/lunch_box_artefacts/Lunch Box.app`
(the CLI lives inside it at `Contents/MacOS/Lunch Box`).

### Run the App

```bash
open "build/lunch_box_artefacts/Lunch Box.app"
```

### Or Process from the Terminal

```bash
"build/lunch_box_artefacts/Lunch Box.app/Contents/MacOS/Lunch Box" \
  --cubbi /path/to/cubbi/samples \
  --jammi /path/to/jammi/samples \
  --output /path/to/output
```

Tip: run with `--install` (may need `sudo`) to add a `lunch_box` command to
`/usr/local/bin` so you can invoke it from anywhere.

---

## Using the App (GUI)

The app has two views, switchable with the **Pack / Bank** tabs (or `Tab`):

- **Pack view** — all 5 banks × 14 slots of the current category at once.
  Best for laying out a whole pack.
- **Bank view** — one bank at a time, each slot as a full-width row with a
  waveform preview. Best for fine-tuning and auditioning.

The **Cubbi / Jammi** tabs (or `Shift+Tab`) switch between the two sample
categories. Each category is its own independent 70-slot grid.

### Filling Slots

- **Drag and drop** — drop audio files or folders onto slots. Folders with
  bank subfolders (`A`–`E`, `bank_a`, `Bank A`, ...) are routed to the matching banks.
- **Browse** — click a slot (Pack) or double-click a row (Bank), or press `Return`
  on the focused slot.
- **Paste** — copy files in Finder (or paths from apps like Sononym) and press `Cmd+V`.
- **Fill** — the Fill button (`Cmd+F`) auto-fills empty slots from a folder,
  alphabetically.

Slots can be rearranged by dragging within the grid; multi-slot selections move
together and displaced samples shift out of the way.

### Previewing

Press `Space` (or click a filled slot) to play a sample. Bank view shows a waveform
for every filled slot.

### Exporting

Press the **Pack** button (`Cmd+Return`), name your pack, and choose its home folder.
Lunch Box converts every assigned sample, generates the `_double` (octave-up) versions,
and writes the complete pack — your original files are never modified.

### Keyboard Shortcuts

Click the LUNCH BOX header in the app to see the shortcut overlay, or check this table:

| Key | Action |
|-----|--------|
| Arrows | Move focus |
| Shift+Arrows | Expand selection |
| Tab | Pack / Bank view |
| Shift+Tab | Cubbi / Jammi |
| Cmd+Up / Down | Change bank (Bank view) |
| Space | Preview |
| Return | Browse for file |
| Delete / Backspace | Clear slot |
| Esc | Clear selection |
| Cmd+Z / Cmd+Shift+Z | Undo / Redo (10 steps) |
| Cmd+C / X / V | Copy / Cut / Paste |
| Cmd+A | Select all |
| Cmd+F | Fill |
| Cmd+Return | Process |
| Cmd+O | Open output folder |

---

## Using the Command Line (CLI)

### Options

| Flag | Shorthand | Description |
|------|-----------|-------------|
| `--cubbi <path>` | `--c` | Folder of cubbi (percussive/loop/SFX) samples |
| `--jammi <path>` | `--j` | Folder of jammi (tuned/chromatic) samples |
| `--output <path>` | `--o` | Output directory (default: `./converted/`) |
| `--install` | | Install the `lunch_box` command to `/usr/local/bin` |
| `--help` | `-h` | Show usage |

At least one of `--cubbi` or `--jammi` is required.

### Examples

```bash
# Process both categories
lunch_box --cubbi ~/samples/cubbi --jammi ~/samples/jammi --output ~/packs/my_pack

# Shorthand flags
lunch_box --c ~/samples/cubbi --j ~/samples/jammi --o ~/packs/my_pack

# One category only
lunch_box --jammi ~/samples/bass_notes
```

### Bank Subfolders

If your source folder contains subfolders named after banks, files are routed to
those banks; loose files fill the remaining slots:

```
my_samples/
├── A/            → Bank A        (also recognised: bank_a, Bank A)
│   ├── kick.wav
│   └── snare.wav
├── B/            → Bank B
│   └── hat.wav
└── extra.wav     → first free slot
```

Without bank subfolders, files are assigned sequentially in alphabetical order.

### Scripting

```bash
#!/bin/bash
# process_chompi.sh

lunch_box \
  --cubbi "$HOME/samples/cubbi" \
  --jammi "$HOME/samples/jammi" \
  --output "$HOME/samples/output"
```

The CLI exits non-zero on failure, so it composes cleanly with `&&` and CI scripts.

---

## Understanding CHOMPI Banks

### Bank Structure

- **2 categories:** cubbi (percussive) and jammi (chromatic)
- **5 banks per category:** A, B, C, D, E
- **14 slots per bank:** numbered 1–14
- **Total capacity:** 70 samples per category

### Naming Convention

```
{category}_{bank}{slot}.wav           # base sample
{category}_{bank}{slot}_double.wav    # optimized sample (pitched up one octave)
```

Examples: `cubbi_a1.wav`, `cubbi_a1_double.wav`, `jammi_e14.wav`, `jammi_e14_double.wav`.

**What are optimized samples?** The CHOMPI hardware pitches samples up and down.
Optimized (`_double`) samples are pre-generated versions pitched up one octave
(double playback speed, half duration). The hardware creates these itself when
missing, but Lunch Box generates them up front so your library is complete.

### File Assignment (sequential mode)

```
Files 1-14  → Bank A
Files 15-28 → Bank B
Files 29-42 → Bank C
Files 43-56 → Bank D
Files 57-70 → Bank E
```

Each input sample produces **two** output files (base + `_double`), so a full
category is 140 files.

---

## Audio Formats

**Input:**
- WAV, AIFF, MP3, FLAC
- Any bit depth and sample rate
- Mono or stereo (files with more than 2 channels are skipped)
- Maximum duration: 2 minutes per sample

**Output (automatic):**
- 16-bit WAV at 48kHz
- Channel count preserved
- `_double` versions are half the duration (under 1 minute)

---

## Tips & Best Practices

### Control the Order with Numeric Prefixes

Files are sorted alphabetically, so prefixes set the slot order:

```
001_kick.wav
002_snare.wav
003_hihat.wav
```

### Managing the 70-Slot Limit

If a folder holds more than 70 files, only the first 70 (alphabetically) are
processed and a warning is logged. Split larger collections into multiple packs:

```bash
lunch_box --cubbi ~/drums_batch1 --output ~/output/batch1
lunch_box --cubbi ~/drums_batch2 --output ~/output/batch2
```

### Originals Are Safe

Lunch Box never modifies source files; everything is written to the output folder.

### Logging

All operations are logged to timestamped files in the per-user log folder —
`~/Library/Logs/Lunch Box` on macOS. In the app, **Settings → Show Log Folder**
opens it directly. To view the most recent log from the terminal:

```bash
ls -t ~/Library/Logs/Lunch\ Box/*.txt | head -n 1 | xargs cat
```

---

## Troubleshooting

### "No audio files found"

- Check the folder path is correct
- Supported extensions: `.wav`, `.aiff`, `.aif`, `.mp3`, `.flac`
- Subfolders are searched, so nesting is fine

### "Error: Folder does not exist"

- Verify the path; absolute paths (`/Users/you/samples/cubbi`) are safest

### "Skipped: Multi-channel audio (6 channels)"

- CHOMPI supports mono and stereo only — bounce multi-channel files to stereo first

### "Skipped: Duration ... exceeds 2-minute limit"

- Trim the sample below 120 seconds and re-run

### Files Not in Expected Order

- Sorting is alphabetical — add numeric prefixes (`001_`, `002_`, ...)
- Check with `ls /path/to/folder`

### "Error: Unable to read source file"

- The file may be corrupted or mislabeled — try opening it in a DAW
- Check read permissions

If something still looks wrong, open an issue and attach the matching log file
from the log folder (`~/Library/Logs/Lunch Box` on macOS).

---

**Happy sampling with CHOMPI!** 🎵
