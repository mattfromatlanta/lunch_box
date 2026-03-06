# Chompi Pack

**Audio sample processor for the CHOMPI sampler**

Chompi Pack is a dual-mode application (GUI and CLI) that converts audio files to the CHOMPI sampler format (16-bit 48kHz WAV) and organizes them using the CHOMPI naming convention.

---

## Features

- ✓ **GUI Application:** User-friendly graphical interface with folder selection
- ✓ **CLI Support:** Full command-line interface for scripting and automation
- ✓ **CHOMPI Mode:** Automatic bank assignment and naming for CHOMPI sampler
- ✓ **Audio Conversion:** Convert any WAV file to 16-bit 48kHz format
- ✓ **Dual Categories:** Process cubbi (percussive/loop/SFX) and jammi (tuned/chromatic) samples
- ✓ **Alphabetical Sorting:** Automatically sorts files for predictable bank assignment
- ✓ **70-File Management:** Handles the CHOMPI 70-sample-per-category limit
- ✓ **Channel Preservation:** Maintains mono/stereo configuration
- ✓ **Legacy Modes:** Scan-only and general conversion modes
- ✓ **Batch Processing:** Process entire sample libraries in one command
- ✓ **Safe Operations:** Never modifies original files

---

## Quick Start

### Build

```bash
cd chompi_pack
mkdir -p build
cd build
cmake ..
make
```

### Usage

#### GUI Mode (Default)

```bash
# Launch GUI application (double-click or run without arguments)
open "chompi_pack_artefacts/Chompi Pack.app"
```

The GUI provides:
- Folder selection buttons for cubbi and jammi samples
- Output folder selection (optional, defaults to `converted/`)
- Visual feedback showing selected paths
- Process button to convert samples
- Status display with progress and results

#### CLI Mode (With Arguments)

```bash
# Process samples for CHOMPI sampler
"chompi_pack_artefacts/Chompi Pack.app/Contents/MacOS/Chompi Pack" \
  --cubbi /path/to/cubbi/samples \
  --jammi /path/to/jammi/samples \
  --output /path/to/output

# Using shorthand flags (--c, --j, --o)
"chompi_pack_artefacts/Chompi Pack.app/Contents/MacOS/Chompi Pack" \
  --c /path/to/cubbi/samples \
  --j /path/to/jammi/samples \
  --o /path/to/output

# Scan audio files (no conversion)
"chompi_pack_artefacts/Chompi Pack.app/Contents/MacOS/Chompi Pack" /path/to/samples

# Convert without CHOMPI naming
"chompi_pack_artefacts/Chompi Pack.app/Contents/MacOS/Chompi Pack" --convert /path/to/samples
```

### Output

```
output/
├── cubbi_a1.wav            # Bank A, Slot 1 (base sample)
├── cubbi_a1_double.wav     # Bank A, Slot 1 (optimized - pitched up)
├── cubbi_a2.wav            # Bank A, Slot 2 (base sample)
├── cubbi_a2_double.wav     # Bank A, Slot 2 (optimized - pitched up)
├── ...
├── cubbi_a14.wav           # Bank A, Slot 14 (base sample)
├── cubbi_a14_double.wav    # Bank A, Slot 14 (optimized - pitched up)
├── cubbi_b1.wav            # Bank B, Slot 1 (base sample)
├── cubbi_b1_double.wav     # Bank B, Slot 1 (optimized - pitched up)
├── ...
├── cubbi_e14.wav           # Bank E, Slot 14 - 70th slot (base sample)
├── cubbi_e14_double.wav    # Bank E, Slot 14 (optimized - pitched up)
├── jammi_a1.wav            # Jammi base sample
├── jammi_a1_double.wav     # Jammi optimized sample
├── jammi_a2.wav
├── jammi_a2_double.wav
└── ...
```

**Note:** Each input sample generates TWO output files:
- **Base sample:** Standard CHOMPI naming (e.g., `cubbi_a1.wav`)
- **Optimized sample:** Pitched up one octave with `_double` suffix (e.g., `cubbi_a1_double.wav`)
- Total output: 140 files per category (70 base + 70 optimized)

---

## Documentation

📖 **[Complete User Guide (HOW_TO.md)](HOW_TO.md)**

Comprehensive guide covering:
- CHOMPI mode usage
- Legacy modes
- Command reference
- Understanding CHOMPI banks
- Tips & best practices
- Troubleshooting
- Example workflows

📋 **[Testing Summary (dev_assets/TESTING_SUMMARY.md)](dev_assets/TESTING_SUMMARY.md)**

Complete test results and validation report

---

## CHOMPI Sampler Overview

The CHOMPI sampler organizes samples into two categories:

### Cubbi Samples
Percussive, loop, or SFX samples (one-shots)
- Examples: Drum hits, sound effects, percussion

### Jammi Samples
Tuned samples designed to be played chromatically
- Examples: Synth notes, bass samples, melodic instruments

### Bank Structure
- **5 banks per category:** A, B, C, D, E
- **14 slots per bank:** Numbered 1-14
- **Total capacity:** 70 samples per category (hardware limit)
- **Output files:** 140 per category (70 base + 70 optimized `_double` versions)

### Naming Convention
```
{category}_{bank}{slot}.wav              # Base sample
{category}_{bank}{slot}_double.wav       # Optimized sample (pitched up)

Examples:
  cubbi_a1.wav          # Cubbi, Bank A, Slot 1 (base)
  cubbi_a1_double.wav   # Cubbi, Bank A, Slot 1 (optimized)
  cubbi_a14.wav         # Cubbi, Bank A, Slot 14 (base)
  cubbi_a14_double.wav  # Cubbi, Bank A, Slot 14 (optimized)
  cubbi_b1.wav          # Cubbi, Bank B, Slot 1 (base)
  cubbi_b1_double.wav   # Cubbi, Bank B, Slot 1 (optimized)
  jammi_e14.wav         # Jammi, Bank E, Slot 14 (base)
  jammi_e14_double.wav  # Jammi, Bank E, Slot 14 (optimized)
```

**Optimized Samples:**
- CHOMPI hardware automatically creates `_double` files if not present
- Chompi Pack generates them proactively for complete libraries
- Optimized = pitched up one octave (double playback speed, half duration)
- Both base and optimized files are loaded into CHOMPI's library

---

## Technical Specifications

### Input
- **Format:** WAV files (*.wav)
- **Bit depth:** Any (8-bit, 16-bit, 24-bit, 32-bit)
- **Sample rate:** Any (44.1kHz, 48kHz, 96kHz, etc.)
- **Channels:** Mono or Stereo (files with >2 channels are skipped)
- **Duration:** Maximum 2 minutes (120 seconds) per sample

### Output
- **Format:** WAV (two files per input)
- **Bit depth:** 16-bit (both base and optimized)
- **Sample rate:** 48kHz (both base and optimized)
- **Channels:** Preserved from source (mono→mono, stereo→stereo)
- **Base sample:** Standard conversion to 16-bit 48kHz
- **Optimized sample:** Pitched up one octave (half duration), appends `_double` suffix
- **Total files:** 2× input count (e.g., 35 inputs → 70 outputs per category)

### Performance
- **Speed:** ~44 MB/sec conversion throughput
- **Memory:** Efficient block processing (4096 samples)
- **Processing:** One file at a time (no memory bloat)

---

## Project Structure

```
chompi_pack/
├── Source/              # C++ source files
│   ├── Main.cpp                    # Application entry point
│   ├── AudioConfiguration.h        # Shared configuration types
│   ├── CLI/                        # Command-line interface
│   │   ├── CliProcessor.h/cpp
│   ├── GUI/                        # Graphical interface
│   │   ├── MainWindow.h/cpp
│   │   ├── MainComponent.h/cpp
│   │   └── GuiProcessor.h/cpp
│   ├── AudioConverter.h/cpp        # Audio format conversion
│   ├── ChompiNamer.h/cpp           # CHOMPI naming logic
│   ├── ChompiProcessor.h/cpp       # Processing orchestrator
│   ├── Logger.h/cpp                # Logging system
│   └── FileSystemHelper.h/cpp      # File utilities
├── build/               # Build directory (CMake)
│   ├── chompi_pack_artefacts/
│   │   └── Chompi Pack.app         # macOS GUI application
│   └── test_chompi_namer_artefacts/
│       └── test_chompi_namer       # Unit tests
├── converted/           # Default output directory
├── logs/                # Operation logs
├── dev_assets/          # Test samples
├── prompts/             # Milestone planning documents
├── CMakeLists.txt       # Build configuration
├── README.md            # This file
├── HOW_TO.md            # User guide
└── .gitignore
```

---

## Requirements

### System Requirements
- **OS:** macOS, Linux, or Windows
- **Compiler:** C++17 compatible compiler
  - macOS: Clang (via Xcode Command Line Tools)
  - Linux: GCC 7+ or Clang 5+
  - Windows: MSVC 2017+ or MinGW

### Dependencies
- **CMake:** 3.22 or higher
- **JUCE Framework:** 8.0.12 or higher
  - juce_core
  - juce_audio_formats
  - juce_audio_basics

### Installing Dependencies (macOS)

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake
brew install cmake

# Install JUCE (if not already installed)
git clone https://github.com/juce-framework/JUCE.git
cd JUCE
git checkout 8.0.12
```

Update `CMakeLists.txt` with your JUCE path:
```cmake
add_subdirectory(/path/to/JUCE ${CMAKE_BINARY_DIR}/JUCE)
```

---

## Development

### Building

```bash
mkdir -p build
cd build
cmake ..
make
```

### Running Tests

```bash
# Unit tests for ChompiNamer
./test_chompi_namer_artefacts/test_chompi_namer

# Integration tests
./chompi_pack_artefacts/chompi_pack --cubbi ../dev_assets/cubbi
```

### Project Milestones

✅ **Milestone 1:** Audio file scanner with metadata reporting
✅ **Milestone 2:** Audio format conversion (16-bit 48kHz)
✅ **Milestone 3:** CHOMPI sampler naming schema and bank organization
✅ **Milestone 4:** GUI application with folder selection and processing

---

## Logging

All operations are logged to timestamped files in the `logs/` directory:

```
logs/
├── chompi_pack_log_20260131_223032.txt
├── chompi_pack_log_20260131_223437.txt
└── ...
```

Log format:
```
chompi_pack_log_YYYYMMDD_HHMMSS.txt
```

Logs include:
- Operation mode
- File counts and paths
- Bank assignments
- Conversion progress
- Success/error statistics

---

## Examples

### Example 1: Basic CHOMPI Processing

```bash
# Long form
./chompi_pack --cubbi ~/drums --jammi ~/synths

# Shorthand form
./chompi_pack --c ~/drums --j ~/synths
```

Output:
```
Processing Cubbi samples...
Found 42 WAV files in Cubbi folder

=== Cubbi Bank Assignment ===
Bank A (14/14 slots filled)
Bank B (14/14 slots filled)
Bank C (14/14 slots filled)
Generated 84 files (42 base + 42 optimized)

Processing Jammi samples...
Found 12 WAV files in Jammi folder

=== Jammi Bank Assignment ===
Bank A (12/14 slots filled)
Generated 24 files (12 base + 12 optimized)

CHOMPI processing complete!
Total output: 108 files (54 base + 54 optimized)
```

### Example 2: Custom Output Directory

```bash
# Long form
./chompi_pack --cubbi ~/drums --output ~/Desktop/chompi_output

# Shorthand form
./chompi_pack --c ~/drums --o ~/Desktop/chompi_output
```

### Example 3: Legacy Scan Mode

```bash
./chompi_pack ~/samples
```

Output:
```
1. kick_001.wav
   Channels: Mono (1)
   Bit Depth: 24-bit
   Sample Rate: 44100 Hz
   Size: 0.52 MB

2. snare_002.wav
   Channels: Stereo (2)
   Bit Depth: 16-bit
   Sample Rate: 48000 Hz
   Size: 0.38 MB
```

---

## Troubleshooting

### Common Issues

**"No WAV files found"**
- Ensure folder contains .wav files
- Check file extensions (.wav, not .WAV or .wave)

**"Error: Folder does not exist"**
- Verify folder path is correct
- Use absolute paths or correct relative paths

**Files not in expected order**
- Files are sorted alphabetically
- Use numeric prefixes: 001_, 002_, 003_, etc.

**"Skipped: Multi-channel audio"**
- CHOMPI supports only mono or stereo
- Convert multi-channel files to stereo first

See [HOW_TO.md](HOW_TO.md) for detailed troubleshooting.

---

## Known Limitations

1. **File Format:** Only WAV files supported currently (AIFF, FLAC, MP3 support planned in Milestone 5)
2. **Channel Count:** Maximum 2 channels (mono or stereo)
3. **Sample Duration:** Maximum 2 minutes per sample (optimized versions will be 1 minute)
4. **70-Slot Limit:** Maximum 70 samples per category (by CHOMPI hardware design)
5. **Output Volume:** 140 files per category (70 base + 70 optimized)
6. **Recursive Search:** All .wav files in subdirectories are included

---

## Future Enhancements

Potential features for future versions:

- [ ] Dry-run mode (preview without converting)
- [ ] Progress bar for large batches
- [ ] Additional format support (AIFF, FLAC)
- [ ] Metadata preservation
- [ ] Custom bank sizes
- [ ] GUI version with drag-and-drop
- [ ] Batch project management

---

## License

*Add license information here*

---

## Contributing

*Add contribution guidelines here*

---

## Credits

Built with [JUCE Framework](https://juce.com/)

---

## Support

For issues, questions, or feature requests:
- Check the [HOW_TO guide](HOW_TO.md)
- Review [test results](dev_assets/TESTING_SUMMARY.md)
- Examine logs in `logs/` directory
- Check planning documents in `prompts/` directory

---

**Version:** 1.1.0 (Milestone 4 Complete - GUI Application)
**Status:** Production Ready ✓
