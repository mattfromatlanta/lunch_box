# Milestone 3: CHOMPI Sampler Naming Schema

## Objective
Implement CHOMPI sampler naming convention for output files, organizing converted audio into cubbi and jammi banks with strict naming schema.

## CHOMPI Sampler Overview

### Bank Structure
CHOMPI uses two categories of sample banks:
- **cubbi banks**: percussive, loop, or SFX samples
- **jammi banks**: tuned samples to play chromatically

Each category has:
- **5 banks**: A, B, C, D, E
- **14 slots per bank**: Numbered 1-14
- **Total per category**: 70 samples (5 banks × 14 slots)

### Naming Convention
```
{category}_{bank}{slot}.wav
```

**Components:**
- `category`: "cubbi" or "jammi" (lowercase)
- `bank`: "a", "b", "c", "d", or "e" (lowercase)
- `slot`: 1-14 (no leading zeros)

**Examples:**
- `cubbi_a1.wav` - Cubbi bank A, slot 1
- `cubbi_a12.wav` - Cubbi bank A, slot 12
- `jammi_e3.wav` - Jammi bank E, slot 3
- `jammi_c14.wav` - Jammi bank C, slot 14

### Valid File Names
```
Cubbi banks:
  Bank A: cubbi_a1.wav through cubbi_a14.wav
  Bank B: cubbi_b1.wav through cubbi_b14.wav
  Bank C: cubbi_c1.wav through cubbi_c14.wav
  Bank D: cubbi_d1.wav through cubbi_d14.wav
  Bank E: cubbi_e1.wav through cubbi_e14.wav

Jammi banks:
  Bank A: jammi_a1.wav through jammi_a14.wav
  Bank B: jammi_b1.wav through jammi_b14.wav
  Bank C: jammi_c1.wav through jammi_c14.wav
  Bank D: jammi_d1.wav through jammi_d14.wav
  Bank E: jammi_e1.wav through jammi_e14.wav
```

## Requirements

### Input Structure
- **Two input folders**: One for cubbi samples, one for jammi samples
- **Command-line specification**: Separate arguments for each folder
- **File selection**: First 70 .wav files in alphabetical order from each folder
- **Processing**: Each folder processed independently

### Output Structure
- **Single output directory**: All processed files in one location
- **Naming**: Files renamed according to CHOMPI convention
- **Format**: All files 16-bit 48kHz WAV (from Milestone 2)

### Command-Line Interface
```bash
# New usage with cubbi and jammi folders
lunch_box --cubbi /path/to/cubbi/samples --jammi /path/to/jammi/samples

# With output directory specification (optional)
lunch_box --cubbi /path/to/cubbi --jammi /path/to/jammi --output /custom/output

# Only cubbi samples (jammi optional)
lunch_box --cubbi /path/to/cubbi/samples

# Only jammi samples (cubbi optional)
lunch_box --jammi /path/to/jammi/samples

# Backward compatibility (scan only, no conversion)
lunch_box /path/to/audio/folder

# Legacy conversion mode
lunch_box --convert /path/to/audio/folder
```

### Processing Rules
1. **Alphabetical sorting**: Files sorted case-insensitive alphabetically
2. **First 70 files**: Only process first 70 .wav files per category
3. **Automatic assignment**:
   - Files 1-14 → Bank A (slots 1-14)
   - Files 15-28 → Bank B (slots 1-14)
   - Files 29-42 → Bank C (slots 1-14)
   - Files 43-56 → Bank D (slots 1-14)
   - Files 57-70 → Bank E (slots 1-14)
4. **Skip files beyond 70**: Log warning if more than 70 files present
5. **Handle fewer than 70**: Process all available files, log incomplete banks

### Output Example
```
Processing cubbi samples...
Source: /path/to/cubbi/samples (83 files found, will process first 70)

Bank A (14/14 slots filled):
  cubbi_a1.wav  ← sample_001.wav
  cubbi_a2.wav  ← sample_002.wav
  ...
  cubbi_a14.wav ← sample_014.wav

Bank B (14/14 slots filled):
  cubbi_b1.wav  ← sample_015.wav
  ...

Bank E (14/14 slots filled):
  cubbi_e1.wav  ← sample_057.wav
  ...
  cubbi_e14.wav ← sample_070.wav

Skipped: 13 files (beyond 70 file limit)

Processing jammi samples...
Source: /path/to/jammi/samples (42 files found)

Bank A (14/14 slots filled)
Bank B (14/14 slots filled)
Bank C (14/14 slots filled)
Bank D (0/14 slots) - incomplete

Total output: 112 files
  Cubbi: 70 files
  Jammi: 42 files
```

## Technical Architecture

### New Module: LunchBoxNamer

```cpp
class LunchBoxNamer
{
public:
    enum class Category
    {
        Cubbi,
        Jammi
    };

    struct BankSlot
    {
        char bank;      // 'a', 'b', 'c', 'd', 'e'
        int slot;       // 1-14
    };

    LunchBoxNamer(Logger& logger);

    // Generate CHOMPI filename for a given index and category
    juce::String generateFileName(Category category, int fileIndex);

    // Process a category folder and return mapping of source files to output names
    struct FileMapping
    {
        juce::File sourceFile;
        juce::String outputFileName;
        BankSlot bankSlot;
    };

    juce::Array<FileMapping> processCategory(const juce::File& sourceFolder,
                                             Category category);

    // Convert file index (0-69) to bank and slot
    static BankSlot indexToBankSlot(int index);

    // Convert bank and slot to filename component (e.g., "a1", "b14")
    static juce::String bankSlotToString(const BankSlot& bankSlot);

private:
    Logger& logger;

    static constexpr int MAX_FILES_PER_CATEGORY = 70;
    static constexpr int SLOTS_PER_BANK = 14;
    static constexpr int NUM_BANKS = 5;

    static const char* BANK_LETTERS[NUM_BANKS];  // {'a', 'b', 'c', 'd', 'e'}
};
```

### Updated AudioConverter

Add method to support custom output filenames:
```cpp
ConversionResult convertFileWithName(const juce::File& sourceFile,
                                     const juce::File& outputFolder,
                                     const juce::String& outputFileName,
                                     juce::AudioFormatManager& formatManager);
```

### Program Flow

```
1. Parse command-line arguments
   ├─ Check for --cubbi and --jammi flags
   ├─ Extract folder paths
   └─ Extract optional --output path

2. For each specified category (cubbi, jammi):
   ├─ Scan folder for .wav files
   ├─ Sort files alphabetically
   ├─ Take first 70 files
   ├─ Generate CHOMPI filenames (a1-e14)
   └─ Create file mappings

3. Display processing plan
   ├─ Show bank assignments
   ├─ Warn about skipped files (>70)
   └─ Warn about incomplete banks (<70)

4. Convert and rename files
   ├─ Process each mapped file
   ├─ Convert to 16-bit 48kHz
   ├─ Save with CHOMPI filename
   └─ Log results

5. Display summary
   ├─ Total files processed
   ├─ Files per category
   └─ Bank fill status
```

## Implementation Steps

### Phase 1: LunchBoxNamer Module
1. Create LunchBoxNamer.h header
   - Define Category enum
   - Define BankSlot struct
   - Define FileMapping struct
   - Declare public methods

2. Create LunchBoxNamer.cpp implementation
   - Implement indexToBankSlot() logic
   - Implement bankSlotToString() formatting
   - Implement generateFileName() method
   - Implement processCategory() with sorting and limiting

3. Add unit tests for naming logic
   - Test index 0 → "a1"
   - Test index 13 → "a14"
   - Test index 14 → "b1"
   - Test index 69 → "e14"
   - Test invalid indices

### Phase 2: Command-Line Parsing Update
1. Update AudioScanner.h/cpp
   - Add cubbi folder parameter
   - Add jammi folder parameter
   - Add output folder parameter
   - Update usage message

2. Implement argument parsing
   - Parse --cubbi flag and path
   - Parse --jammi flag and path
   - Parse --output flag and path (optional)
   - Validate at least one category specified

3. Update usage message
   ```
   Usage: lunch_box [OPTIONS]

   Options:
     --cubbi <path>    Process cubbi samples from folder
     --jammi <path>    Process jammi samples from folder
     --output <path>   Output directory (default: converted/)

   CHOMPI Mode (requires --cubbi and/or --jammi):
     lunch_box --cubbi /samples/cubbi --jammi /samples/jammi

   Legacy Modes:
     lunch_box <folder>              Scan only
     lunch_box --convert <folder>    Convert without renaming
   ```

### Phase 3: Integration
1. Update Main.cpp
   - Add CHOMPI mode detection
   - Process cubbi folder if specified
   - Process jammi folder if specified
   - Pass file mappings to converter

2. Update AudioConverter
   - Add convertFileWithName() method
   - Support custom output filenames
   - Preserve existing conversion functionality

3. Create processing pipeline
   - LunchBoxNamer generates mappings
   - AudioConverter processes with custom names
   - Logger reports progress

### Phase 4: Testing & Validation
1. Test with various file counts
   - Exactly 70 files per category
   - More than 70 files (test skipping)
   - Fewer than 70 files (test incomplete banks)
   - Mixed counts (70 cubbi, 42 jammi)

2. Test alphabetical sorting
   - Files with numbers (001.wav, 002.wav)
   - Files with names (kick.wav, snare.wav)
   - Mixed case filenames
   - Special characters in names

3. Validate output
   - All filenames follow CHOMPI convention
   - Files in correct numerical order
   - All files 16-bit 48kHz
   - Original files preserved

4. Test edge cases
   - Empty cubbi or jammi folder
   - Only cubbi specified (no jammi)
   - Only jammi specified (no cubbi)
   - Invalid folder paths

5. Test backward compatibility
   - Legacy scan mode still works
   - Legacy --convert mode still works
   - New CHOMPI mode doesn't break old functionality

## File Organization

### Input Structure (Example)
```
project/
├── samples/
│   ├── cubbi/              # Cubbi source samples
│   │   ├── kick_001.wav
│   │   ├── snare_002.wav
│   │   ├── ...
│   │   └── perc_083.wav    # 83 files (will process first 70)
│   │
│   └── jammi/              # Jammi source samples
│       ├── ambient_a.wav
│       ├── bass_b.wav
│       ├── ...
│       └── vocal_z.wav     # 42 files (all will be processed)
```

### Output Structure
```
converted/
├── cubbi_a1.wav
├── cubbi_a2.wav
├── ...
├── cubbi_a14.wav
├── cubbi_b1.wav
├── ...
├── cubbi_e14.wav   # 70 cubbi files
├── jammi_a1.wav
├── jammi_a2.wav
├── ...
├── jammi_c14.wav   # 42 jammi files (only 3 complete banks)
└── README.md
```

## Success Criteria

- [ ] LunchBoxNamer module correctly generates all 70 filenames per category
- [ ] Files sorted alphabetically (case-insensitive)
- [ ] First 70 files selected per category
- [ ] Files beyond 70 logged and skipped
- [ ] Correct bank assignment (A-E)
- [ ] Correct slot assignment (1-14)
- [ ] Output filenames match CHOMPI convention exactly
- [ ] All converted files are 16-bit 48kHz
- [ ] Single output directory contains all files
- [ ] Cubbi and jammi files can be processed independently
- [ ] Command-line interface intuitive and clear
- [ ] Backward compatibility maintained
- [ ] Comprehensive logging of all operations
- [ ] Handle incomplete banks gracefully

## Technical Considerations

### Alphabetical Sorting
- Use case-insensitive comparison
- Handle platform differences (macOS/Linux/Windows)
- JUCE String::compareNatural() may be useful
- Consider numeric vs. lexicographic sorting

### Bank Assignment Algorithm
```cpp
// Given file index (0-based):
int index = ...;  // 0-69

// Calculate bank (0-4)
int bankIndex = index / 14;
char bank = 'a' + bankIndex;  // 'a', 'b', 'c', 'd', 'e'

// Calculate slot (1-14)
int slot = (index % 14) + 1;

// Example:
// index 0  → bank 0, slot 1  → a1
// index 13 → bank 0, slot 14 → a14
// index 14 → bank 1, slot 1  → b1
// index 27 → bank 1, slot 14 → b14
// index 69 → bank 4, slot 14 → e14
```

### Error Handling
- Missing cubbi or jammi folder: Skip that category, warn user
- No files in folder: Skip category, warn user
- Fewer than 14 files: Fill partial bank, log incomplete status
- Conversion errors: Log error, continue with remaining files
- Duplicate filenames after sorting: Should not occur with index-based naming

### Memory Considerations
- Process files one at a time (not all 140 in memory)
- Release reader/writer resources after each file
- Sort file list in memory (lightweight - just file paths)

## Future Considerations (Not in Milestone 3)

- **Interactive mode**: Let user assign files to specific banks/slots
- **Bank preview**: Show which samples are assigned where before processing
- **Bank templates**: Save/load bank assignments
- **Metadata tagging**: Embed bank/slot info in WAV metadata
- **Dry-run mode**: Show what would be processed without converting
- **Custom bank sizes**: Support different slot counts per bank
- **GUI**: Visual bank manager with drag-and-drop

## Dependencies
- ✅ Milestone 1: Scanning functionality
- ✅ Milestone 2: Conversion functionality
- ✅ FileSystemHelper: Directory operations
- ✅ Logger: Operation logging
- ✅ AudioConverter: File conversion

## Notes
- CHOMPI naming convention is strict - no variations allowed
- Banks must be filled sequentially (A→B→C→D→E)
- Slots within banks must be sequential (1→14)
- Original source filenames are preserved in logs for reference
- Users should organize source files in desired playback order
- Alphabetical sorting means users can use numbered prefixes (001_, 002_, etc.)
