# Chompi Pack - How to Use

**Chompi Pack** is a command-line tool for processing audio samples for the CHOMPI sampler. It converts audio files to the CHOMPI standard format (16-bit 48kHz WAV) and organizes them into the CHOMPI naming convention.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [CHOMPI Mode](#chompi-mode)
3. [Legacy Modes](#legacy-modes)
4. [Command Reference](#command-reference)
5. [Understanding CHOMPI Banks](#understanding-chompi-banks)
6. [Tips & Best Practices](#tips--best-practices)
7. [Troubleshooting](#troubleshooting)

---

## Quick Start

### Building the Project

```bash
cd chompi_pack
mkdir -p build
cd build
cmake ..
make
```

The executable will be located at: `build/chompi_pack_artefacts/chompi_pack`

### Basic CHOMPI Workflow

```bash
# Process cubbi (percussive/loop/SFX) and jammi (tuned/chromatic) samples
./chompi_pack --cubbi /path/to/cubbi/samples --jammi /path/to/jammi/samples

# Files will be converted to converted/ directory by default
# Output: cubbi_a1.wav, cubbi_a2.wav, ..., jammi_a1.wav, jammi_a2.wav, ...
```

---

## CHOMPI Mode

CHOMPI mode is the primary way to process samples for the CHOMPI sampler. It converts your audio files to 16-bit 48kHz WAV format and renames them according to the CHOMPI naming convention.

### What are Cubbi and Jammi?

- **Cubbi samples:** Percussive hits, loops, or sound effects (one-shots)
- **Jammi samples:** Tuned samples designed to be played chromatically

### Basic Usage

```bash
# Process both cubbi and jammi samples
chompi_pack --cubbi /path/to/cubbi --jammi /path/to/jammi

# Using shorthand flags (--c, --j, --o)
chompi_pack --c /path/to/cubbi --j /path/to/jammi

# Process only cubbi samples
chompi_pack --cubbi /path/to/cubbi

# Process only jammi samples
chompi_pack --jammi /path/to/jammi
```

### Custom Output Directory

By default, files are saved to `converted/`. You can specify a custom output directory:

```bash
chompi_pack --cubbi /path/to/cubbi --output /custom/output/directory

# Or using shorthand
chompi_pack --c /path/to/cubbi --o /custom/output/directory
```

### Example: Complete Workflow

```bash
# Organize your samples first
mkdir -p ~/samples/chompi_project
mkdir -p ~/samples/chompi_project/cubbi
mkdir -p ~/samples/chompi_project/jammi

# Copy percussive samples to cubbi folder
cp ~/drums/*.wav ~/samples/chompi_project/cubbi/

# Copy tuned samples to jammi folder
cp ~/synths/*.wav ~/samples/chompi_project/jammi/

# Process with chompi_pack (long form)
cd chompi_pack/build
./chompi_pack_artefacts/chompi_pack \
  --cubbi ~/samples/chompi_project/cubbi \
  --jammi ~/samples/chompi_project/jammi \
  --output ~/samples/chompi_project/output

# Or using shorthand flags (faster to type!)
./chompi_pack_artefacts/chompi_pack \
  --c ~/samples/chompi_project/cubbi \
  --j ~/samples/chompi_project/jammi \
  --o ~/samples/chompi_project/output
```

**Output:**
```
output/
├── cubbi_a1.wav
├── cubbi_a2.wav
├── ...
├── cubbi_e14.wav
├── jammi_a1.wav
├── jammi_a2.wav
└── ...
```

---

## Legacy Modes

Chompi Pack also supports legacy modes for general audio file scanning and conversion.

### Scan Mode (No Conversion)

Scan a folder to see audio file properties without converting:

```bash
chompi_pack /path/to/audio/folder
```

**Output:**
```
1. my_sample.wav
   Channels: Stereo (2)
   Bit Depth: 24-bit
   Sample Rate: 44100 Hz
   Size: 3.46 MB

2. another_sample.wav
   Channels: Mono (1)
   Bit Depth: 16-bit
   Sample Rate: 48000 Hz
   Size: 0.52 MB
```

### Convert Mode (No CHOMPI Naming)

Convert audio files to 16-bit 48kHz without CHOMPI naming:

```bash
chompi_pack --convert /path/to/audio/folder
```

Files will be converted to `converted/` directory with their **original filenames** preserved.

---

## Command Reference

### CHOMPI Mode Flags

| Flag | Shorthand | Required | Description |
|------|-----------|----------|-------------|
| `--cubbi <path>` | `--c` | No* | Path to folder containing cubbi (percussive/loop/SFX) samples |
| `--jammi <path>` | `--j` | No* | Path to folder containing jammi (tuned/chromatic) samples |
| `--output <path>` | `--o` | No | Output directory (default: `converted/`) |

*At least one of `--cubbi` or `--jammi` must be specified for CHOMPI mode.

### Legacy Mode Flags

| Command | Description |
|---------|-------------|
| `chompi_pack <folder>` | Scan audio files (no conversion) |
| `chompi_pack --convert <folder>` | Convert to 16-bit 48kHz (preserve original filenames) |

### Help

```bash
chompi_pack
# Running with no arguments displays usage information
```

---

## Understanding CHOMPI Banks

### Bank Structure

The CHOMPI sampler organizes samples into **banks** and **slots**:

- **5 banks per category:** A, B, C, D, E
- **14 slots per bank:** Numbered 1-14
- **Total capacity:** 70 samples per category (5 × 14)

### Naming Convention

Format: `{category}_{bank}{slot}.wav`

**Examples:**
- `cubbi_a1.wav` - Cubbi, Bank A, Slot 1
- `cubbi_a14.wav` - Cubbi, Bank A, Slot 14
- `cubbi_b1.wav` - Cubbi, Bank B, Slot 1
- `jammi_e14.wav` - Jammi, Bank E, Slot 14

### File Assignment

Chompi Pack automatically assigns files to banks based on **alphabetical order**:

```
Files 1-14  → Bank A (cubbi_a1.wav  - cubbi_a14.wav)
Files 15-28 → Bank B (cubbi_b1.wav  - cubbi_b14.wav)
Files 29-42 → Bank C (cubbi_c1.wav  - cubbi_c14.wav)
Files 43-56 → Bank D (cubbi_d1.wav  - cubbi_d14.wav)
Files 57-70 → Bank E (cubbi_e1.wav  - cubbi_e14.wav)
```

### Sample Output

```
Processing Cubbi samples...
Found 42 WAV files in Cubbi folder

=== Cubbi Bank Assignment ===
Bank A (14/14 slots filled)
Bank B (14/14 slots filled)
Bank C (14/14 slots filled)
Bank D (0/14 slots) - incomplete
```

---

## Tips & Best Practices

### 1. Organizing Your Samples

**Name your files with numeric prefixes** to control the order:

```bash
001_kick.wav
002_snare.wav
003_hihat.wav
...
```

This ensures they appear in your desired order in the CHOMPI banks.

### 2. Managing the 70-File Limit

Each category (cubbi/jammi) can hold **maximum 70 samples**. If you have more:

- Chompi Pack will process only the first 70 files (alphabetically)
- A warning will display: "Found 83 files. Only first 70 will be processed."
- Files 71+ will be skipped

**Strategy:** Organize samples into multiple batches:
```bash
# Batch 1: Drums
chompi_pack --cubbi ~/drums_batch1 --output ~/output/batch1

# Batch 2: Synths
chompi_pack --cubbi ~/synths_batch1 --output ~/output/batch2
```

### 3. Audio Format Recommendations

**Input formats supported:**
- WAV files only (*.wav)
- Any bit depth (8-bit, 16-bit, 24-bit, 32-bit)
- Any sample rate (44.1kHz, 48kHz, 96kHz, etc.)
- Mono or stereo (multi-channel files >2 channels are skipped)

**Output format (automatic):**
- 16-bit WAV
- 48kHz sample rate
- Channel count preserved (mono stays mono, stereo stays stereo)

### 4. Checking Before Processing

Use scan mode to preview your samples before converting:

```bash
# Preview cubbi samples
chompi_pack ~/samples/cubbi

# Check file count
ls ~/samples/cubbi/*.wav | wc -l
```

### 5. Preserving Originals

Chompi Pack **never modifies your original files**. All converted files are written to a separate output directory.

### 6. Batch Processing Workflow

```bash
# 1. Organize samples
mkdir -p project/{cubbi,jammi,output}

# 2. Copy/move samples to categories
cp ~/kicks/*.wav project/cubbi/
cp ~/bass/*.wav project/jammi/

# 3. Preview (optional)
chompi_pack project/cubbi
chompi_pack project/jammi

# 4. Process
chompi_pack --cubbi project/cubbi --jammi project/jammi --output project/output

# 5. Load output folder into CHOMPI sampler
```

---

## Troubleshooting

### "No WAV files found"

**Problem:** Chompi Pack can't find any .wav files in the specified folder.

**Solutions:**
- Check that the folder path is correct
- Ensure files have `.wav` extension (not `.WAV`, `.wave`, etc.)
- Chompi Pack searches recursively - files in subfolders will be found

### "Error: Folder does not exist"

**Problem:** The specified folder path doesn't exist.

**Solutions:**
- Verify the folder path is correct
- Use absolute paths: `/Users/username/samples/cubbi`
- Or relative paths: `../samples/cubbi`

### "Skipped: Multi-channel audio (6 channels)"

**Problem:** File has more than 2 channels.

**Solutions:**
- CHOMPI supports only mono or stereo
- Use a DAW to bounce multi-channel files to stereo before processing

### Files Not in Expected Order

**Problem:** Files are assigned to banks in unexpected order.

**Solutions:**
- Chompi Pack sorts files **alphabetically**
- Use numeric prefixes to control order: `001_`, `002_`, `003_`, etc.
- Check sorting: `ls /path/to/folder/*.wav`

### Output Files Not 16-bit 48kHz

**Problem:** This shouldn't happen, but if it does...

**Solutions:**
- Check the conversion log in `logs/chompi_pack_log_*.txt`
- Verify output file: `chompi_pack converted/` (scan mode)
- Report issue with log file attached

### Conversion Errors

**Problem:** "Error: Unable to read source file"

**Solutions:**
- File might be corrupted - try opening in a DAW
- File might not be a valid WAV file
- Check file permissions (read access required)

---

## Advanced Usage

### Processing Only Specific Banks

If you want to fill only specific banks, prepare exactly that many files:

```bash
# Only fill Bank A (14 files)
# Name files: 001.wav through 014.wav

# Only fill Banks A and B (28 files)
# Name files: 001.wav through 028.wav
```

### Using with Shell Scripts

Create a processing script:

```bash
#!/bin/bash
# process_chompi.sh

CUBBI_DIR="$HOME/samples/cubbi"
JAMMI_DIR="$HOME/samples/jammi"
OUTPUT_DIR="$HOME/samples/output"

# Create directories
mkdir -p "$OUTPUT_DIR"

# Process samples
./chompi_pack_artefacts/chompi_pack \
  --cubbi "$CUBBI_DIR" \
  --jammi "$JAMMI_DIR" \
  --output "$OUTPUT_DIR"

echo "Processing complete! Files saved to: $OUTPUT_DIR"
```

### Logging

All operations are logged to `logs/chompi_pack_log_YYYYMMDD_HHMMSS.txt`

To view the most recent log:
```bash
ls -t logs/*.txt | head -n 1 | xargs cat
```

---

## Example Workflows

### Workflow 1: Drum Kit Processing

```bash
# Organize drum samples
mkdir -p drumkit/{cubbi,output}

# Add samples with numeric prefixes for order
cp kicks/my_kick.wav drumkit/cubbi/01_kick.wav
cp snares/my_snare.wav drumkit/cubbi/02_snare.wav
cp hihats/closed_hat.wav drumkit/cubbi/03_hihat_closed.wav
cp hihats/open_hat.wav drumkit/cubbi/04_hihat_open.wav
# ... add more samples ...

# Process
chompi_pack --cubbi drumkit/cubbi --output drumkit/output

# Result:
# drumkit/output/cubbi_a1.wav (kick)
# drumkit/output/cubbi_a2.wav (snare)
# etc.
```

### Workflow 2: Chromatic Instrument

```bash
# Prepare tuned samples (e.g., sampled bass notes)
mkdir -p bass_instrument/{jammi,output}

# Copy samples named by pitch
cp samples/bass_C.wav bass_instrument/jammi/01_C.wav
cp samples/bass_Db.wav bass_instrument/jammi/02_Db.wav
cp samples/bass_D.wav bass_instrument/jammi/03_D.wav
# ... one sample per semitone ...

# Process
chompi_pack --jammi bass_instrument/jammi --output bass_instrument/output

# Load into CHOMPI and play chromatically
```

### Workflow 3: Mixed Project

```bash
# Complete project with drums (cubbi) and synth (jammi)
mkdir -p project/{cubbi,jammi,output}

# Add drums to cubbi
cp drums/*.wav project/cubbi/

# Add synth samples to jammi
cp synths/*.wav project/jammi/

# Process both categories
chompi_pack \
  --cubbi project/cubbi \
  --jammi project/jammi \
  --output project/output

# Result: All 70 cubbi slots + all 70 jammi slots filled
```

---

## Quick Reference Card

```
╔════════════════════════════════════════════════════════════════╗
║                    CHOMPI PACK QUICK REFERENCE                 ║
╠════════════════════════════════════════════════════════════════╣
║ CHOMPI MODE (Primary Usage)                                    ║
║   chompi_pack --cubbi <path> --jammi <path> --output <path>    ║
║   chompi_pack --c <path> --j <path> --o <path>  # Shorthand   ║
║                                                                 ║
║ LEGACY MODES                                                    ║
║   chompi_pack <folder>              # Scan only               ║
║   chompi_pack --convert <folder>    # Convert (no renaming)   ║
║                                                                 ║
║ BANK STRUCTURE                                                  ║
║   • 5 banks per category (A-E)                                 ║
║   • 14 slots per bank (1-14)                                   ║
║   • 70 samples max per category                                ║
║                                                                 ║
║ SAMPLE TYPES                                                    ║
║   • Cubbi: Percussive, loops, SFX                              ║
║   • Jammi: Tuned samples for chromatic play                    ║
║                                                                 ║
║ OUTPUT FORMAT                                                   ║
║   • 16-bit WAV, 48kHz                                          ║
║   • Channel count preserved (mono/stereo)                      ║
║   • Naming: {category}_{bank}{slot}.wav                        ║
║                                                                 ║
║ TIPS                                                            ║
║   • Use numeric prefixes (001_, 002_, etc.) to control order   ║
║   • Max 70 files per category (files 71+ skipped)              ║
║   • Original files never modified                              ║
║   • Logs saved to logs/ directory                              ║
╚════════════════════════════════════════════════════════════════╝
```

---

## Support & Resources

- **Project Files:** `/Users/matthewfishel/Repos/chompi_pack/`
- **Logs:** `logs/chompi_pack_log_*.txt`
- **Test Results:** `test_audio/TESTING_SUMMARY.md`
- **Planning Docs:** `prompts/milestone_*.md`

---

**Happy sampling with CHOMPI!** 🎵
