# Lunch Box

**Audio sample processor for the CHOMPI sampler**

Lunch Box is a dual-mode (GUI + CLI) application that converts audio files to
CHOMPI-compatible format (16-bit 48kHz WAV) and organizes them using the CHOMPI
naming convention.

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL_v3-blue.svg)](LICENSE)

---

## Features

- **Two GUI Modes** — Pack (5-bank x 14-slot grid), Bank (single-bank focus view)
- **CLI Support** — Full command-line interface for scripting and automation
- **Format Support** — WAV, AIFF, MP3, FLAC input
- **CHOMPI Bank Assignment** — Automatic bank/slot organization with bank-subfolder detection
- **Dual Categories** — Process Cubbi (percussive) and Jammi (chromatic) samples independently
- **Waveform Preview** — Click-to-play preview. Waveform previews in bank mode
- **Drag and Drop** — Drag folders or individual files directly onto slots
- **Undo / Redo** — 10-step history across all view modes
- **Keyboard Navigation** — Full keyboard control (Cmd+Z/C/X/V/A, arrows, Tab, Space, Enter)
- **Safe Operations** — Never modifies original files

---

## Quick Start

### Prerequisites

- C++17 compiler (Clang on macOS, GCC/Clang on Linux, MSVC on Windows)
- CMake 3.22+
- [JUCE](https://github.com/juce-framework/JUCE) 8.0.12+

### Build

```bash
git clone https://github.com/mattfromatlanta/lunch_box.git
cd lunch_box

# Update the JUCE path in CMakeLists.txt to point at your local JUCE install, then:
mkdir -p build && cd build
cmake ..
make
```

### Run (macOS)

```bash
open "build/lunch_box_artefacts/Lunch Box.app"
```

### CLI Mode

```bash
"build/lunch_box_artefacts/Lunch Box.app/Contents/MacOS/Lunch Box" \
  --cubbi /path/to/cubbi/samples \
  --jammi /path/to/jammi/samples \
  --output /path/to/output

# Shorthand flags
lunch_box --c /path/to/cubbi --j /path/to/jammi --o /path/to/output

# Help
lunch_box --help
```

---

## CHOMPI Sampler Overview

The CHOMPI sampler organizes samples into two categories, each with five banks (A-E)
of 14 slots — 70 samples per category.

| Category | Use |
|----------|-----|
| **Cubbi** | Percussive, one-shots, loops, SFX |
| **Jammi** | Tuned/chromatic samples |

### Naming Convention

```
cubbi_a1.wav          # Cubbi, Bank A, Slot 1 (base)
cubbi_a1_double.wav   # Cubbi, Bank A, Slot 1 (optimized -- pitched up one octave)
jammi_e14.wav         # Jammi, Bank E, Slot 14 (base)
jammi_e14_double.wav  # Jammi, Bank E, Slot 14 (optimized)
```

Lunch Box generates both base and `_double` versions for every input sample so your
CHOMPI library is complete without requiring the hardware to generate them.

---

## Audio Specifications

|               | Input             | Output              |
|---------------|-------------------|---------------------|
| **Format**    | WAV, AIFF, MP3, FLAC | WAV              |
| **Bit depth** | Any               | 16-bit              |
| **Sample rate** | Any             | 48kHz               |
| **Channels**  | Mono / Stereo     | Preserved           |
| **Max duration** | 120 sec        | 120 sec base / 60 sec _double |

---

## Output Structure

```
output/
+-- cubbi_a1.wav          # Bank A, Slot 1 (base)
+-- cubbi_a1_double.wav   # Bank A, Slot 1 (optimized)
+-- cubbi_a2.wav
+-- cubbi_a2_double.wav
+-- ...
+-- cubbi_e14.wav         # Bank E, Slot 14 (base)
+-- cubbi_e14_double.wav  # Bank E, Slot 14 (optimized)
+-- jammi_a1.wav
+-- jammi_a1_double.wav
+-- ...
```

Each input generates two output files. Maximum 140 files per category (70 base + 70 optimized).

---

## Project Structure

```
lunch_box/
+-- Source/
|   +-- Main.cpp                       # Entry point, GUI/CLI routing
|   +-- AudioConfiguration.h           # Shared config struct
|   +-- FileSystemHelper.h/cpp         # File utilities
|   +-- Logger.h/cpp                   # Timestamped logging
|   +-- CLI/
|   |   +-- CliProcessor.h/cpp         # CLI argument parsing
|   +-- GUI/
|   |   +-- MainWindow.h/cpp
|   |   +-- MainComponent.h/cpp        # Tabs, mode switching
|   |   +-- GuiProcessor.h/cpp         # GUI to processing bridge
|   |   +-- BankEditorPanel.h/cpp      # Pack mode: 5x14 grid
|   |   +-- BankFocusPanel.h/cpp       # Bank mode: single-bank view
|   |   +-- FocusedSlotRow.h/cpp       # Bank mode: slot row with waveform
|   |   +-- BankRowComponent.h/cpp
|   |   +-- BankSlotComponent.h/cpp
|   |   +-- PreviewPanel.h/cpp         # Waveform + playback
|   |   +-- FolderDropZone.h/cpp       # Simple mode drop zones
|   +-- Processing/
|       +-- LunchBoxProcessor.h/cpp      # Processing orchestrator
|       +-- AudioConverter.h/cpp       # Format conversion
|       +-- LunchBoxNamer.h/cpp          # CHOMPI naming + constants
|       +-- BankFolderParser.h/cpp     # Bank subfolder detection
+-- tests/                             # Unit tests (JUCE UnitTest framework)
+-- CMakeLists.txt
+-- README.md
+-- HOW_TO.md                          # Detailed user guide
+-- CONTRIBUTING.md
+-- CODE_OF_CONDUCT.md
+-- LICENSE
```

---

## Logging

All operations are logged to timestamped files in the `logs/` directory:

```
logs/lunch_box_log_YYYYMMDD_HHMMSS.txt
```

---

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

---

## License

GNU Affero General Public License v3.0 -- see [LICENSE](LICENSE) for details.

This project uses [JUCE](https://juce.com/), licensed under the AGPLv3.

This project uses the [Poppins](https://fonts.google.com/specimen/Poppins) typeface by The Poppins Project Authors, licensed under the [SIL Open Font License 1.1](fonts/OFL.txt).

This project uses the [Fredoka](https://fonts.google.com/specimen/Fredoka) typeface by The Fredoka Project Authors, licensed under the [SIL Open Font License 1.1](fonts/OFL-Fredoka.txt).

---

## Author

Made by **Matt from Atlanta**

- [GitHub](https://github.com/mattfromatlanta)
- [Bluesky](https://bsky.app/profile/mattfromatlanta.bsky.social)
- [Bandcamp](https://angsttanks.bandcamp.com/)

---

## Acknowledgements

- [JUCE Framework](https://juce.com/) -- cross-platform C++ application framework
- [CHOMPI](https://creditor.technology) -- the sampler hardware this tool is built for
