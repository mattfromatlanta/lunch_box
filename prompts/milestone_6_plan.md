# Milestone 6: Bank-Specific Folder Organization

## Objective
Add intelligent folder parsing that recognizes bank-specific subfolders (A, B, C, D, E) and automatically assigns samples within those folders to the corresponding CHOMPI banks, while unsorted samples use remaining available banks.

## Requirements

### Folder Structure Recognition

**Recognized Folder Names:**
- Single-letter folders: `A`, `B`, `C`, `D`, `E` (case-insensitive)
- Full word folders: `bank_a`, `bank_A`, `Bank A`, etc.
- Both cubbi and jammi folders can contain bank subfolders

**Example Input Structure:**
```
cubbi/
├── A/                    # Bank A samples
│   ├── kick.wav
│   ├── snare.wav
│   └── hihat.wav
├── B/                    # Bank B samples
│   ├── clap.wav
│   └── rim.wav
├── tom1.wav              # Unsorted - uses next available bank
├── tom2.wav              # Unsorted - uses next available bank
└── misc/                 # Non-bank folder - treated as unsorted
    └── fx.wav
```

**Expected Output:**
```
converted/
├── cubbi_a1.wav         ← kick.wav (from A/ folder)
├── cubbi_a2.wav         ← snare.wav (from A/ folder)
├── cubbi_a3.wav         ← hihat.wav (from A/ folder)
├── cubbi_b1.wav         ← clap.wav (from B/ folder)
├── cubbi_b2.wav         ← rim.wav (from B/ folder)
├── cubbi_c1.wav         ← tom1.wav (unsorted, uses next available)
├── cubbi_c2.wav         ← tom2.wav (unsorted, uses next available)
└── cubbi_c3.wav         ← fx.wav (from misc/, treated as unsorted)
```

### Processing Rules

1. **Bank Folder Priority**
   - Samples in bank folders (A-E) are assigned to those specific banks
   - Bank folders processed first (A→B→C→D→E)
   - Samples within each bank folder sorted alphabetically

2. **Unsorted Samples**
   - Samples in root or non-bank folders are "unsorted"
   - Unsorted samples fill remaining slots in available banks
   - If bank A has 10 samples, unsorted starts filling A slot 11

3. **Bank Overflow Handling**
   - If bank folder has >14 samples, only first 14 used
   - Log warning about skipped samples
   - Continue processing other banks

4. **Mixed Mode**
   - Bank folders and unsorted samples can coexist
   - Bank assignments are deterministic
   - Clear logging of assignment logic

5. **Validation**
   - Empty bank folders are ignored
   - Invalid bank names (F, G, etc.) treated as unsorted
   - Nested bank folders not recognized (only top-level)

## Technical Architecture

### New Module: BankFolderParser

```cpp
class BankFolderParser
{
public:
    struct BankAssignment
    {
        juce::File sourceFile;
        char bankLetter;        // 'a', 'b', 'c', 'd', 'e'
        int slotNumber;         // 1-14
        bool fromBankFolder;    // true if from bank folder, false if unsorted
        juce::String sourcePath; // For logging
    };

    BankFolderParser(Logger& logger);

    // Parse a folder and return bank-specific assignments
    juce::Array<BankAssignment> parseFolderStructure(
        const juce::File& sourceFolder,
        LunchBoxNamer::Category category
    );

private:
    Logger& logger;

    // Detect if folder name represents a bank
    static bool isBankFolder(const juce::String& folderName, char& bankLetter);

    // Get all bank folders in source folder
    void findBankFolders(const juce::File& sourceFolder,
                        std::map<char, juce::Array<juce::File>>& bankFiles);

    // Get unsorted files (not in bank folders)
    void findUnsortedFiles(const juce::File& sourceFolder,
                          const std::set<juce::String>& bankFolderNames,
                          juce::Array<juce::File>& unsortedFiles);

    // Assign files to banks
    juce::Array<BankAssignment> createAssignments(
        const std::map<char, juce::Array<juce::File>>& bankFiles,
        const juce::Array<juce::File>& unsortedFiles
    );
};
```

### Integration with LunchBoxProcessor

Update `LunchBoxProcessor::processCategory()`:

**Current Flow:**
1. Scan folder for all WAV files
2. Sort alphabetically
3. Assign to banks sequentially (1-14 → A, 15-28 → B, etc.)

**New Flow:**
1. Scan folder for bank-specific subfolders
2. Scan for unsorted files
3. Assign bank-folder files to their banks
4. Fill remaining slots with unsorted files
5. Generate CHOMPI filenames

### Data Structures

```cpp
// Bank-specific file collection
struct BankCollection
{
    std::map<char, juce::Array<juce::File>> bankFiles;  // 'a' → [files in A/]
    juce::Array<juce::File> unsortedFiles;              // Files not in banks
};

// Assignment tracking
struct BankSlotStatus
{
    char bank;          // 'a'-'e'
    int slotsUsed;      // 0-14
    int slotsAvailable; // 14 - slotsUsed
};
```

## Implementation Steps

### Phase 1: Bank Folder Detection

1. **Create BankFolderParser Class**
   - Create `Source/BankFolderParser.h`
   - Create `Source/BankFolderParser.cpp`
   - Add to CMakeLists.txt

2. **Implement Bank Name Recognition**
   ```cpp
   bool BankFolderParser::isBankFolder(const juce::String& name, char& bank)
   {
       auto lower = name.toLowerCase().trim();

       // Single letter: "a", "b", "c", "d", "e"
       if (lower.length() == 1 && lower[0] >= 'a' && lower[0] <= 'e')
       {
           bank = lower[0];
           return true;
       }

       // Patterns: "bank_a", "bank a", "Bank A", etc.
       if (lower.startsWith("bank"))
       {
           auto suffix = lower.fromFirstOccurrenceOf("bank", false, false)
                             .trim().removeCharacters("_- ");
           if (suffix.length() == 1 && suffix[0] >= 'a' && suffix[0] <= 'e')
           {
               bank = suffix[0];
               return true;
           }
       }

       return false;
   }
   ```

3. **Test Bank Name Recognition**
   - Unit tests for various naming patterns
   - Case sensitivity tests
   - Invalid name rejection tests

### Phase 2: File Collection

1. **Implement Bank Folder Scanning**
   ```cpp
   void BankFolderParser::findBankFolders(
       const juce::File& sourceFolder,
       std::map<char, juce::Array<juce::File>>& bankFiles)
   {
       // Get immediate subdirectories
       auto subdirs = sourceFolder.findChildFiles(
           juce::File::findDirectories,
           false  // Not recursive - only top level
       );

       for (const auto& dir : subdirs)
       {
           char bankLetter;
           if (isBankFolder(dir.getFileName(), bankLetter))
           {
               // Find audio files in this bank folder
               juce::Array<juce::File> files;
               dir.findChildFiles(files,
                                juce::File::findFiles,
                                true,  // Recursive within bank folder
                                "*.wav");

               // Sort alphabetically
               files.sort();

               // Store in map
               bankFiles[bankLetter] = files;

               logger.logLine("Found bank folder: " + dir.getFileName() +
                            " (" + juce::String(files.size()) + " files)");
           }
       }
   }
   ```

2. **Implement Unsorted File Collection**
   ```cpp
   void BankFolderParser::findUnsortedFiles(
       const juce::File& sourceFolder,
       const std::set<juce::String>& bankFolderNames,
       juce::Array<juce::File>& unsortedFiles)
   {
       // Find all audio files in source folder
       juce::Array<juce::File> allFiles;
       sourceFolder.findChildFiles(allFiles,
                                 juce::File::findFiles,
                                 true,
                                 "*.wav");

       // Filter out files that are in bank folders
       for (const auto& file : allFiles)
       {
           bool inBankFolder = false;
           auto parent = file.getParentDirectory();

           // Check if file is inside a bank folder
           while (parent != sourceFolder && parent.exists())
           {
               if (bankFolderNames.count(parent.getFileName()))
               {
                   inBankFolder = true;
                   break;
               }
               parent = parent.getParentDirectory();
           }

           if (!inBankFolder)
           {
               unsortedFiles.add(file);
           }
       }

       // Sort unsorted files alphabetically
       unsortedFiles.sort();
   }
   ```

### Phase 3: Bank Assignment Logic

1. **Implement Assignment Algorithm**
   ```cpp
   juce::Array<BankAssignment> BankFolderParser::createAssignments(
       const std::map<char, juce::Array<juce::File>>& bankFiles,
       const juce::Array<juce::File>& unsortedFiles)
   {
       juce::Array<BankAssignment> assignments;
       std::map<char, int> slotCounts;  // Track slots used per bank

       // Initialize slot counts
       for (char b = 'a'; b <= 'e'; ++b)
           slotCounts[b] = 0;

       // Phase 1: Assign files from bank folders
       for (char bank = 'a'; bank <= 'e'; ++bank)
       {
           if (bankFiles.count(bank))
           {
               const auto& files = bankFiles.at(bank);
               int count = 0;

               for (const auto& file : files)
               {
                   if (count >= 14)
                   {
                       logger.logLine("Warning: Bank " + juce::String(bank).toUpperCase() +
                                    " has more than 14 files. Skipping extras.");
                       break;
                   }

                   BankAssignment assignment;
                   assignment.sourceFile = file;
                   assignment.bankLetter = bank;
                   assignment.slotNumber = count + 1;
                   assignment.fromBankFolder = true;
                   assignment.sourcePath = file.getRelativePathFrom(sourceFolder);

                   assignments.add(assignment);
                   count++;
               }

               slotCounts[bank] = count;
           }
       }

       // Phase 2: Assign unsorted files to available slots
       int unsortedIndex = 0;
       for (char bank = 'a'; bank <= 'e' && unsortedIndex < unsortedFiles.size(); ++bank)
       {
           int slotsUsed = slotCounts[bank];
           int slotsAvailable = 14 - slotsUsed;

           for (int slot = 0; slot < slotsAvailable && unsortedIndex < unsortedFiles.size(); ++slot)
           {
               BankAssignment assignment;
               assignment.sourceFile = unsortedFiles[unsortedIndex];
               assignment.bankLetter = bank;
               assignment.slotNumber = slotsUsed + slot + 1;
               assignment.fromBankFolder = false;
               assignment.sourcePath = unsortedFiles[unsortedIndex].getFileName();

               assignments.add(assignment);
               unsortedIndex++;
           }
       }

       // Warn if some unsorted files couldn't fit
       if (unsortedIndex < unsortedFiles.size())
       {
           int skipped = unsortedFiles.size() - unsortedIndex;
           logger.logLine("Warning: " + juce::String(skipped) +
                        " unsorted files skipped (70 file limit reached)");
       }

       return assignments;
   }
   ```

2. **Update LunchBoxProcessor**
   - Replace current sorting logic
   - Use BankFolderParser for file assignments
   - Update logging to show bank folder vs unsorted

### Phase 4: Logging and Feedback

1. **Enhanced Bank Assignment Display**
   ```
   Processing Cubbi samples...

   === Folder Structure Analysis ===
   Found bank folder: A/ (12 files)
   Found bank folder: B/ (8 files)
   Found 15 unsorted files

   === Cubbi Bank Assignment ===
   Bank A (14/14 slots filled)
     Slots 1-12: From A/ folder
     Slots 13-14: From unsorted files

   Bank B (14/14 slots filled)
     Slots 1-8: From B/ folder
     Slots 9-14: From unsorted files

   Bank C (9/14 slots filled)
     Slots 1-9: From unsorted files

   Total files processed: 37
     From bank folders: 20
     From unsorted: 17
   ```

2. **GUI Status Updates**
   - Show bank folder detection
   - Display assignment breakdown
   - Highlight mixed bank/unsorted processing

### Phase 5: Testing

1. **Unit Tests**
   - Bank name recognition
   - File collection logic
   - Assignment algorithm
   - Edge cases (empty banks, overflow)

2. **Integration Tests**
   - Simple: Only bank folders (no unsorted)
   - Simple: Only unsorted (no bank folders)
   - Mixed: Bank folders + unsorted
   - Overflow: Bank folder >14 files
   - Full: All banks (A-E) with files

3. **Real-World Tests**
   - User-organized sample library
   - Partially organized library
   - Completely unsorted library
   - Verify backward compatibility

## Example Scenarios

### Scenario 1: Fully Organized

**Input:**
```
cubbi/
├── A/
│   ├── 01_kick.wav
│   ├── ...
│   └── 14_crash.wav    (14 files total)
├── B/
│   └── ...             (14 files)
├── C/
│   └── ...             (14 files)
├── D/
│   └── ...             (14 files)
└── E/
    └── ...             (14 files)
```

**Output:** Each bank filled with files from corresponding folder

### Scenario 2: Partially Organized

**Input:**
```
cubbi/
├── A/
│   └── ...             (5 files)
├── B/
│   └── ...             (10 files)
├── kick_01.wav
├── snare_01.wav
└── ...                 (20 unsorted files)
```

**Output:**
- Bank A: 5 from A/ + 9 unsorted
- Bank B: 10 from B/ + 4 unsorted
- Bank C: 14 unsorted
- etc.

### Scenario 3: No Bank Folders (Backward Compatible)

**Input:**
```
cubbi/
├── sample_01.wav
├── sample_02.wav
└── ...                 (50 files)
```

**Output:** Same as current behavior - sequential bank assignment

## Success Criteria

- [ ] Bank folder names recognized (A-E, bank_a, Bank A, etc.)
- [ ] Files in bank folders assigned to correct banks
- [ ] Unsorted files fill remaining slots
- [ ] Assignment algorithm deterministic
- [ ] Overflow handled gracefully (>14 per bank)
- [ ] Empty bank folders ignored
- [ ] Invalid bank names treated as unsorted
- [ ] Logging shows folder structure and assignments
- [ ] GUI displays bank folder detection
- [ ] CLI processes bank folders correctly
- [ ] Backward compatibility maintained
- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] Documentation updated

## Dependencies

- ✅ Milestone 4 complete (GUI and CLI)
- ➕ New BankFolderParser module
- ➕ Updated LunchBoxProcessor logic

## Future Considerations (Not in Milestone 6)

- **Nested bank folders** (Bank A/Kicks/, Bank A/Snares/)
- **Bank templates** (save/load folder structures)
- **Visual bank editor** (GUI for managing assignments)
- **Drag-and-drop reordering** (within banks)
- **Bank folder creation** (auto-create A-E folders)

## Notes

- Focus on top-level bank folders only
- Keep backward compatibility (unsorted-only still works)
- Clear logging is critical for user understanding
- Deterministic behavior important for reproducibility
- Bank folders are optional - not required

## Estimated Impact

**Code Changes:** Moderate
- New BankFolderParser: ~200 lines
- LunchBoxProcessor updates: ~50 lines
- Logging updates: ~30 lines
- Tests: ~150 lines

**User Benefit:** High
- Organized libraries maintain organization
- Reduces post-processing work
- Predictable bank assignments
- Flexible (supports organized + unsorted)
