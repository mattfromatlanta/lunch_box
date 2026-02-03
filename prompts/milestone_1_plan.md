# Milestone 1: Audio File Scanner

## Objective
Create a command-line application that scans a folder for WAV audio files and reports their properties (stereo/mono, bit rate, sample rate).

## Requirements

### Input
- Accept a folder path as a command-line argument
- Validate that the provided path exists and is a directory
- Handle recursive scanning of subdirectories

### Output
For each WAV file found, display:
1. File path (relative or absolute)
2. Channel configuration (mono/stereo)
3. Bit depth (e.g., 16-bit, 24-bit, 32-bit)
4. Sample rate (e.g., 44.1kHz, 48kHz, 96kHz)

### Output Format Example
```
Scanning: /path/to/audio/folder
Found 3 WAV files:

1. song1.wav
   Channels: Stereo (2)
   Bit Depth: 16-bit
   Sample Rate: 44100 Hz

2. subfolder/recording.wav
   Channels: Mono (1)
   Bit Depth: 24-bit
   Sample Rate: 48000 Hz

3. sample.wav
   Channels: Stereo (2)
   Bit Depth: 32-bit
   Sample Rate: 96000 Hz
```

## Technical Architecture

### JUCE Modules Required
- **juce_core**: File system operations, string handling, console I/O
- **juce_audio_formats**: Audio file reading and format detection
- **juce_audio_basics**: Audio buffer and channel handling

### Key Classes to Use

1. **juce::File**
   - Purpose: Handle file system navigation
   - Methods:
     - `File::getCurrentWorkingDirectory()` or direct path construction
     - `exists()` and `isDirectory()` for validation
     - `findChildFiles()` for recursive directory scanning with wildcard "*.wav"

2. **juce::AudioFormatManager**
   - Purpose: Manage audio format readers
   - Setup: Call `registerBasicFormats()` to enable WAV/AIFF support
   - Method: `createReaderFor(File)` to instantiate file readers

3. **juce::AudioFormatReader**
   - Purpose: Read audio file metadata
   - Properties:
     - `sampleRate`: Sample rate in Hz
     - `numChannels`: Channel count (1=mono, 2=stereo, etc.)
     - `bitsPerSample`: Bit depth of the audio file
     - `lengthInSamples`: Total samples (useful for duration calculation)

### Program Structure

```
chompi_pack/
├── CMakeLists.txt           # JUCE CMake project configuration
├── Source/
│   ├── Main.cpp             # Entry point, argument parsing
│   ├── AudioScanner.h       # Scanner class declaration
│   └── AudioScanner.cpp     # Scanner implementation
└── README.md                # Build and usage instructions
```

### Core Components

#### 1. Main.cpp
- Parse command-line arguments
- Validate folder path
- Instantiate AudioScanner
- Display results

#### 2. AudioScanner Class
Responsibilities:
- Initialize AudioFormatManager
- Scan directory for WAV files
- Collect file metadata
- Format and return results

Methods:
```cpp
class AudioScanner
{
public:
    AudioScanner();
    ~AudioScanner();

    // Scan directory and return list of audio file info
    std::vector<AudioFileInfo> scanDirectory(const juce::File& directory);

private:
    juce::AudioFormatManager formatManager;

    // Extract metadata from a single file
    AudioFileInfo analyzeFile(const juce::File& file);
};

struct AudioFileInfo
{
    juce::String filePath;
    int numChannels;
    int bitsPerSample;
    double sampleRate;
    bool isValid;
    juce::String errorMessage;
};
```

## Implementation Steps

### Phase 1: Project Setup
1. Create CMakeLists.txt with JUCE integration
   - Set minimum CMake version (3.22+)
   - Add JUCE as subdirectory or via FetchContent
   - Link required JUCE modules
   - Create console application target

2. Configure build system
   - Test basic compilation
   - Verify JUCE modules load correctly

### Phase 2: File System Scanner
1. Implement directory validation
   - Check if path exists
   - Verify it's a directory (not a file)
   - Handle permission errors

2. Implement recursive WAV file search
   - Use `File::findChildFiles()` with "*.wav" wildcard
   - Set search to recursive
   - Store results in array

### Phase 3: Audio File Analysis
1. Initialize AudioFormatManager
   - Register basic formats (WAV/AIFF)
   - Handle initialization errors

2. Implement metadata extraction
   - Create reader for each file
   - Extract: numChannels, bitsPerSample, sampleRate
   - Handle unsupported/corrupted files gracefully

3. Format channel information
   - 1 channel: "Mono (1)"
   - 2 channels: "Stereo (2)"
   - n channels: "Multi-channel (n)"

### Phase 4: Output Formatting
1. Display scan progress
   - Show directory being scanned
   - Report file count

2. Format individual file information
   - Clean, readable output
   - Consistent spacing and alignment
   - Handle long file paths

3. Handle edge cases
   - No WAV files found
   - All files corrupted/unreadable
   - Permission denied errors

### Phase 5: Testing & Validation
1. Test with various WAV files
   - Different bit depths (16, 24, 32)
   - Different sample rates (44.1k, 48k, 96k, 192k)
   - Mono and stereo files

2. Test error conditions
   - Invalid directory path
   - Empty directory
   - Corrupted WAV files
   - Permission issues

3. Test edge cases
   - Very large directories
   - Nested subdirectories
   - Special characters in filenames

## Build Instructions (Planned)

### CMake Build Process
```bash
cd chompi_pack
mkdir build
cd build
cmake ..
cmake --build .
```

### Running the Application
```bash
./chompi_pack /path/to/audio/folder
```

## Success Criteria

- [ ] Application compiles without errors
- [ ] Successfully scans directories recursively
- [ ] Correctly identifies all WAV files
- [ ] Accurately reports channel count
- [ ] Accurately reports bit depth
- [ ] Accurately reports sample rate
- [ ] Handles errors gracefully (invalid paths, corrupted files)
- [ ] Clean, readable console output

## Dependencies Status
- ✅ JUCE 8.0.12 installed at ~/Repos/JUCE
- ✅ C++ compiler (clang++ 17.0.0)
- ✅ CMake 4.2.1

## Future Considerations (Not in Milestone 1)
- Support for other audio formats (AIFF, FLAC, MP3)
- File conversion capabilities (Milestone 2)
- GUI interface (Later milestone)
- Batch processing with progress bars
- Output to CSV/JSON for further processing

## Notes
- Focus on WAV files only for now (simplest format, most common)
- Command-line interface keeps scope manageable
- Establishes foundation for conversion functionality in next milestone
- No audio playback needed - just metadata reading
