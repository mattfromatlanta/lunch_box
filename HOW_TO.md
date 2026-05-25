c# Lunch Box - How to Use

**Lunch Box** is a command-line tool for processing audio samples for the CHOMPI sampler. It converts audio files to the CHOMPI standard format (16-bit 48kHz WAV) and organizes them into the CHOMPI naming convention.

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
cd lunch_box
mkdir -p build
cd build
cmake ..
make
```

The executable will be located at: `build/lunch_box_artefacts/lunch_box`

### Basic CHOMPI Workflow

```bash
# Process cubbi (percussive/loop/SFX) and jammi (tuned/chromatic) samples
./lunch_box --cubbi /path/to/cubbi/samples --jammi /path/to/jammi/samples

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
lunch_box --cubbi /path/to/cubbi --jammi /path/to/jammi

# Using shorthand flags (--c, --j, --o)
lunch_box --c /path/to/cubbi --j /path/to/jammi

# Process only cubbi samples
lunch_box --cubbi /path/to/cubbi

# Process only jammi samples
lunch_box --jammi /path/to/jammi
```

### Custom Output Directory

By default, files are saved to `converted/`. You can specify a custom output directory:

```bash
lunch_box --cubbi /path/to/cubbi --output /custom/output/directory

# Or using shorthand
lunch_box --c /path/to/cubbi --o /custom/output/directory
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

# Process with lunch_box (long form)
cd lunch_box/build
./lunch_box_artefacts/lunch_box \
  --cubbi ~/samples/chompi_project/cubbi \
  --jammi ~/samples/chompi_project/jammi \
  --output ~/samples/chompi_project/output

# Or using shorthand flags (faster to type!)
./lunch_box_artefacts/lunch_box \
  --c ~/samples/chompi_project/cubbi \
  --j ~/samples/chompi_project/jammi \
  --o ~/samples/chompi_project/output
```

**Output:**
```
output/
├── cubbi_a1.wav            # Base sample
├── cubbi_a1_double.wav     # Optimized (pitched up 1 octave)
├── cubbi_a2.wav            # Base sample
├── cubbi_a2_double.wav     # Optimized (pitched up 1 octave)
├── ...
├── cubbi_e14.wav           # Base sample (70th slot)
├── cubbi_e14_double.wav    # Optimized (70th slot)
├── jammi_a1.wav            # Base sample
├── jammi_a1_double.wav     # Optimized (pitched up 1 octave)
├── jammi_a2.wav
├── jammi_a2_double.wav
└── ...
```

**Note:** Each input sample generates TWO files:
- **Base sample:** Standard CHOMPI name (e.g., `cubbi_a1.wav`)
- **Optimized sample:** Pitched up one octave with `_double` suffix (e.g., `cubbi_a1_double.wav`)

The optimized samples enable pitch-shifting features on the CHOMPI hardware.

---

## Legacy Modes

Lunch Box also supports legacy modes for general audio file scanning and conversion.

### Scan Mode (No Conversion)

Scan a folder to see audio file properties without converting:

```bash
lunch_box /path/to/audio/folder
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
lunch_box --convert /path/to/audio/folder
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
| `lunch_box <folder>` | Scan audio files (no conversion) |
| `lunch_box --convert <folder>` | Convert to 16-bit 48kHz (preserve original filenames) |

### Help

```bash
lunch_box
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

**Base samples:**
Format: `{category}_{bank}{slot}.wav`

**Optimized samples:**
Format: `{category}_{bank}{slot}_double.wav` (pitched up one octave)

**Examples:**
- `cubbi_a1.wav` - Cubbi, Bank A, Slot 1 (base)
- `cubbi_a1_double.wav` - Cubbi, Bank A, Slot 1 (optimized)
- `cubbi_a14.wav` - Cubbi, Bank A, Slot 14 (base)
- `cubbi_a14_double.wav` - Cubbi, Bank A, Slot 14 (optimized)
- `cubbi_b1.wav` - Cubbi, Bank B, Slot 1 (base)
- `cubbi_b1_double.wav` - Cubbi, Bank B, Slot 1 (optimized)
- `jammi_e14.wav` - Jammi, Bank E, Slot 14 (base)
- `jammi_e14_double.wav` - Jammi, Bank E, Slot 14 (optimized)

**What are optimized samples?**
The CHOMPI hardware can pitch samples up or down. Optimized samples (`_double`) are pre-generated versions pitched up one octave (double playback speed, half duration). The hardware automatically creates these if not present, but Lunch Box generates them proactively for complete libraries.

### File Assignment

Lunch Box automatically assigns files to banks based on **alphabetical order**:

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

Generated 84 files (42 base + 42 optimized)
```

**Output Explanation:**
- **Input:** 42 files
- **Output:** 84 files (42 base samples + 42 optimized `_double` versions)
- **Banks Used:** A (full), B (full), C (full)
- **Bank D Status:** Incomplete (0 out of 14 slots)

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

### 2. Managing the 70-Slot Limit

Each category (cubbi/jammi) has **70 slots** (CHOMPI hardware limit). However, each slot produces **2 files**:
- 1 base sample
- 1 optimized sample (`_double` suffix)

**Capacity:**
- Maximum input: 70 samples per category
- Output files: 140 per category (70 base + 70 optimized)
- Total output: 280 files maximum (140 cubbi + 140 jammi)

**If you have more than 70 files:**
- Lunch Box will process only the first 70 files (alphabetically)
- A warning will display: "Found 83 files. Only first 70 will be processed."
- Files 71+ will be skipped
- Output: 140 files (70 slots × 2 versions each)

**Strategy:** Organize samples into multiple batches:
```bash
# Batch 1: Drums (70 files → 140 output files)
lunch_box --cubbi ~/drums_batch1 --output ~/output/batch1

# Batch 2: Synths (70 files → 140 output files)
lunch_box --cubbi ~/synths_batch1 --output ~/output/batch2
```

### 3. Audio Format Recommendations

**Input formats supported:**
- WAV files only (*.wav) - AIFF, MP3, FLAC support coming in Milestone 5
- Any bit depth (8-bit, 16-bit, 24-bit, 32-bit)
- Any sample rate (44.1kHz, 48kHz, 96kHz, etc.)
- Mono or stereo (multi-channel files >2 channels are skipped)
- **Maximum duration:** 2 minutes (120 seconds) per sample

**Output format (automatic):**
- **Base samples:** 16-bit WAV at 48kHz
- **Optimized samples:** 16-bit WAV at 48kHz, pitched up one octave (half duration)
- Channel count preserved (mono stays mono, stereo stays stereo)
- Two files per input: base + `_double` (optimized)

**Duration limits:**
- Input samples must be under 2 minutes
- Optimized versions will be under 1 minute (half of base duration)
- Files exceeding 2 minutes are skipped with warning

### 4. Checking Before Processing

Use scan mode to preview your samples before converting:

```bash
# Preview cubbi samples
lunch_box ~/samples/cubbi

# Check file count
ls ~/samples/cubbi/*.wav | wc -l
```

### 5. Preserving Originals

Lunch Box **never modifies your original files**. All converted files are written to a separate output directory.

### 6. Batch Processing Workflow

```bash
# 1. Organize samples
mkdir -p project/{cubbi,jammi,output}

# 2. Copy/move samples to categories
cp ~/kicks/*.wav project/cubbi/
cp ~/bass/*.wav project/jammi/

# 3. Preview (optional)
lunch_box project/cubbi
lunch_box project/jammi

# 4. Process
lunch_box --cubbi project/cubbi --jammi project/jammi --output project/output

# 5. Load output folder into CHOMPI sampler
```

---

## Troubleshooting

### "No WAV files found"

**Problem:** Lunch Box can't find any .wav files in the specified folder.

**Solutions:**
- Check that the folder path is correct
- Ensure files have `.wav` extension (not `.WAV`, `.wave`, etc.)
- Lunch Box searches recursively - files in subfolders will be found

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
- Lunch Box sorts files **alphabetically**
- Use numeric prefixes to control order: `001_`, `002_`, `003_`, etc.
- Check sorting: `ls /path/to/folder/*.wav`

### Output Files Not 16-bit 48kHz

**Problem:** This shouldn't happen, but if it does...

**Solutions:**
- Check the conversion log in `logs/lunch_box_log_*.txt`
- Verify output file: `lunch_box converted/` (scan mode)
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
./lunch_box_artefacts/lunch_box \
  --cubbi "$CUBBI_DIR" \
  --jammi "$JAMMI_DIR" \
  --output "$OUTPUT_DIR"

echo "Processing complete! Files saved to: $OUTPUT_DIR"
```

### Logging

All operations are logged to `logs/lunch_box_log_YYYYMMDD_HHMMSS.txt`

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
lunch_box --cubbi drumkit/cubbi --output drumkit/output

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
lunch_box --jammi bass_instrument/jammi --output bass_instrument/output

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
lunch_box \
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
║   lunch_box --cubbi <path> --jammi <path> --output <path>    ║
║   lunch_box --c <path> --j <path> --o <path>  # Shorthand   ║
║                                                                 ║
║ LEGACY MODES                                                    ║
║   lunch_box <folder>              # Scan only               ║
║   lunch_box --convert <folder>    # Convert (no renaming)   ║
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

- **Project Files:** `/path/to/lunch_box/`
- **Logs:** `logs/lunch_box_log_*.txt`
- **Test Results:** `dev_assets/TESTING_SUMMARY.md`
- **Planning Docs:** `prompts/milestone_*.md`

---

**Happy sampling with CHOMPI!** 🎵
