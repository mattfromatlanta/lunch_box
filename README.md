# Chompi Pack

**Audio sample processor for the CHOMPI sampler**

Chompi Pack is a command-line tool that converts audio files to the CHOMPI sampler format (16-bit 48kHz WAV) and organizes them using the CHOMPI naming convention.

---

## Features

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

```bash
# Process samples for CHOMPI sampler
./chompi_pack_artefacts/chompi_pack \
  --cubbi /path/to/cubbi/samples \
  --jammi /path/to/jammi/samples \
  --output /path/to/output

# Using shorthand flags (--c, --j, --o)
./chompi_pack_artefacts/chompi_pack \
  --c /path/to/cubbi/samples \
  --j /path/to/jammi/samples \
  --o /path/to/output

# Scan audio files (no conversion)
./chompi_pack_artefacts/chompi_pack /path/to/samples

# Convert without CHOMPI naming
./chompi_pack_artefacts/chompi_pack --convert /path/to/samples
```

### Output

```
output/
├── cubbi_a1.wav      # Bank A, Slot 1
├── cubbi_a2.wav      # Bank A, Slot 2
├── ...
├── cubbi_a14.wav     # Bank A, Slot 14
├── cubbi_b1.wav      # Bank B, Slot 1
├── ...
├── cubbi_e14.wav     # Bank E, Slot 14 (70th file)
├── jammi_a1.wav
├── jammi_a2.wav
└── ...
```

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

📋 **[Testing Summary (test_audio/TESTING_SUMMARY.md)](test_audio/TESTING_SUMMARY.md)**

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
- **Total capacity:** 70 samples per category

### Naming Convention
```
{category}_{bank}{slot}.wav

Examples:
  cubbi_a1.wav   # Cubbi, Bank A, Slot 1
  cubbi_a14.wav  # Cubbi, Bank A, Slot 14
  cubbi_b1.wav   # Cubbi, Bank B, Slot 1
  jammi_e14.wav  # Jammi, Bank E, Slot 14
```

---

## Technical Specifications

### Input
- **Format:** WAV files (*.wav)
- **Bit depth:** Any (8-bit, 16-bit, 24-bit, 32-bit)
- **Sample rate:** Any (44.1kHz, 48kHz, 96kHz, etc.)
- **Channels:** Mono or Stereo (files with >2 channels are skipped)

### Output
- **Format:** WAV
- **Bit depth:** 16-bit
- **Sample rate:** 48kHz
- **Channels:** Preserved from source (mono→mono, stereo→stereo)

### Performance
- **Speed:** ~44 MB/sec conversion throughput
- **Memory:** Efficient block processing (4096 samples)
- **Processing:** One file at a time (no memory bloat)

---

## Project Structure

```
chompi_pack/
├── Source/              # C++ source files
│   ├── Main.cpp
│   ├── AudioScanner.cpp/.h
│   ├── AudioConverter.cpp/.h
│   ├── ChompiNamer.cpp/.h
│   ├── Logger.cpp/.h
│   └── FileSystemHelper.cpp/.h
├── build/               # Build directory (CMake)
│   ├── chompi_pack_artefacts/
│   │   └── chompi_pack  # Executable
│   └── test_chompi_namer_artefacts/
│       └── test_chompi_namer  # Unit tests
├── converted/           # Default output directory
├── logs/                # Operation logs
├── test_audio/          # Test samples
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
./chompi_pack_artefacts/chompi_pack --cubbi ../test_audio/cubbi
```

### Project Milestones

✅ **Milestone 1:** Audio file scanner with metadata reporting
✅ **Milestone 2:** Audio format conversion (16-bit 48kHz)
✅ **Milestone 3:** CHOMPI sampler naming schema and bank organization

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

Processing Jammi samples...
Found 12 WAV files in Jammi folder

=== Jammi Bank Assignment ===
Bank A (12/14 slots filled)

CHOMPI processing complete!
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

1. **File Format:** Only WAV files supported (no AIFF, FLAC, MP3, etc.)
2. **Channel Count:** Maximum 2 channels (mono or stereo)
3. **70-File Limit:** Maximum 70 samples per category (by CHOMPI design)
4. **Recursive Search:** All .wav files in subdirectories are included

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
- Review [test results](test_audio/TESTING_SUMMARY.md)
- Examine logs in `logs/` directory
- Check planning documents in `prompts/` directory

---

**Version:** 1.0.0 (Milestone 3 Complete)
**Status:** Production Ready ✓
