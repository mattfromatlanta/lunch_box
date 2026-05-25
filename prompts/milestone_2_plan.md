# Milestone 2: Audio File Conversion

## Objective
Add audio format conversion processing to convert WAV files to a standardized format (16-bit, 48kHz) while preserving channel configuration.

## Requirements

### Input
- Use existing scanner functionality from Milestone 1
- Accept WAV files with various bit depths and sample rates
- Process mono and stereo files only
- Skip files with more than 2 channels

### Conversion Specifications
- **Target Format:** WAV (uncompressed)
- **Target Bit Depth:** 16-bit
- **Target Sample Rate:** 48000 Hz (48kHz)
- **Channel Handling:**
  - Mono files → Mono output
  - Stereo files → Stereo output
  - Multi-channel (>2) → Skip with log message

### Output
- Create output directory for converted files
- Preserve original filenames with optional suffix/prefix
- Original files must NOT be modified
- Log all conversion operations (success, skipped, errors)

### Output Format Example
```
Converting: song1.wav
   Original: Stereo (2), 24-bit, 44100 Hz
   Target:   Stereo (2), 16-bit, 48000 Hz
   Output:   converted/song1.wav
   Status:   ✓ Converted successfully

Skipping: surround_mix.wav
   Reason:   Multi-channel audio (6 channels) - not supported

Converting: recording.wav
   Original: Mono (1), 16-bit, 44100 Hz
   Target:   Mono (1), 16-bit, 48000 Hz
   Output:   converted/recording.wav
   Status:   ✓ Converted successfully (sample rate only)
```

## Technical Architecture

### JUCE Modules Required (Already Included)
- **juce_core**: File system operations
- **juce_audio_formats**: Audio reading/writing (AudioFormatReader, AudioFormatWriter, WavAudioFormat)
- **juce_audio_basics**: Sample rate conversion (ResamplingAudioSource)

### Key Classes to Use

1. **juce::WavAudioFormat**
   - Purpose: Create WAV file writers with specific parameters
   - Methods:
     - `createWriterFor()` to instantiate writer with target specs
     - `getPossibleSampleRates()` and `getPossibleBitDepths()` for validation

2. **juce::AudioFormatWriter**
   - Purpose: Write audio data to files
   - Methods:
     - `writeFromAudioReader()` for easy conversion from reader to writer
     - Automatically handles format conversions and buffering
   - Parameters: sample rate, number of channels, bit depth

3. **juce::AudioFormatReaderSource**
   - Purpose: Wrap AudioFormatReader as an AudioSource
   - Enables integration with ResamplingAudioSource

4. **juce::ResamplingAudioSource**
   - Purpose: Convert sample rates on-the-fly
   - Methods:
     - `setResamplingRatio()` to set conversion ratio
     - `prepareToPlay()` and `getNextAudioBlock()` for processing
   - Ratio calculation: sourceRate / targetRate

### Conversion Workflow

```
1. Read source file → AudioFormatReader
2. Check channel count → Skip if > 2
3. If sample rate conversion needed:
   a. Wrap reader in AudioFormatReaderSource
   b. Wrap source in ResamplingAudioSource
   c. Set resampling ratio (sourceRate / 48000.0)
4. Create WavAudioFormat writer:
   - 16-bit depth
   - 48000 Hz sample rate
   - Same channel count as source
5. Convert:
   - If resampling: Read blocks from ResamplingAudioSource and write
   - If no resampling: Use writeFromAudioReader() directly
6. Close writer and verify output
7. Log results
```

### Program Structure (Additions)

```
lunch_box/
├── Source/
│   ├── Main.cpp                    # (Existing)
│   ├── Logger.h/cpp                # (Existing)
│   ├── AudioScanner.h/cpp          # (Existing)
│   └── AudioConverter.h/cpp        # NEW - Conversion logic
├── converted/                      # NEW - Output directory for converted files
└── dev_assets/unsorted/            # (Existing test files)
```

### New AudioConverter Module

```cpp
class AudioConverter
{
public:
    AudioConverter(Logger& logger);

    // Convert a single audio file
    struct ConversionResult
    {
        bool success;
        bool skipped;
        juce::String message;
    };

    ConversionResult convertFile(const juce::File& sourceFile,
                                 const juce::File& outputFolder,
                                 juce::AudioFormatManager& formatManager);

    // Batch convert multiple files
    void convertFiles(const juce::Array<juce::File>& files,
                     const juce::File& outputFolder,
                     juce::AudioFormatManager& formatManager);

private:
    Logger& logger;

    // Constants for target format
    static constexpr int TARGET_BIT_DEPTH = 16;
    static constexpr double TARGET_SAMPLE_RATE = 48000.0;
    static constexpr int MAX_CHANNELS = 2;

    // Helper methods
    bool needsConversion(const juce::AudioFormatReader* reader);
    bool createOutputDirectory(const juce::File& outputFolder);
    juce::File generateOutputPath(const juce::File& sourceFile,
                                  const juce::File& outputFolder);
};
```

## Implementation Steps

### Phase 1: JUCE Framework Research ✓
**Status:** Complete (documented above)

Key findings:
- `AudioFormatWriter::writeFromAudioReader()` provides simple conversion path
- `ResamplingAudioSource` handles sample rate conversion
- `WavAudioFormat::createWriterFor()` creates writers with specific parameters
- Bit depth conversion handled automatically by JUCE

### Phase 2: Output Directory Setup
1. Create output directory structure
   - Default: `converted/` in working directory
   - Allow command-line argument for custom location (future enhancement)

2. Implement directory validation
   - Check if output directory exists
   - Create if missing
   - Verify write permissions

3. Implement filename handling
   - Preserve original filename
   - Optional: Add suffix like `_converted` or `_16bit_48k`
   - Handle naming conflicts (overwrite vs. skip)

### Phase 3: AudioConverter Module
1. Create AudioConverter.h header
   - Define AudioConverter class interface
   - Define ConversionResult struct
   - Declare public conversion methods
   - Declare private helper methods

2. Create AudioConverter.cpp implementation
   - Implement constructor
   - Implement single file conversion logic
   - Implement batch conversion wrapper
   - Implement helper methods

3. Implement conversion logic
   - Read source file with AudioFormatReader
   - Check channel count and skip if > 2
   - Determine if sample rate/bit depth conversion needed
   - Create output file path
   - Setup ResamplingAudioSource if needed
   - Create WavAudioFormat writer with target specs
   - Perform conversion
   - Handle errors gracefully

### Phase 4: Integration with Main Application
1. Update Main.cpp
   - Add conversion step after scanning
   - Create AudioConverter instance
   - Call batch conversion method
   - Pass scanned files and output directory

2. Update command-line interface (optional)
   - Add flag to enable/disable conversion (e.g., `--convert`)
   - Add option to specify output directory
   - Maintain backward compatibility

3. Enhance logging
   - Log conversion progress for each file
   - Show before/after specs
   - Display success/skip/error counts
   - Report total processing time

### Phase 5: Testing & Validation
1. Test with various source formats
   - 16-bit 44.1kHz → No conversion needed (just copy?)
   - 24-bit 44.1kHz → Bit depth conversion only
   - 16-bit 96kHz → Sample rate conversion only
   - 24-bit 192kHz → Both conversions
   - Mono and stereo files

2. Test edge cases
   - Already 16-bit 48kHz (should still process or skip?)
   - Multi-channel files (should skip)
   - Corrupted files (should handle gracefully)
   - Very large files (memory management)
   - Zero-length files

3. Validate output quality
   - Verify bit depth with hex editor or audio analysis tool
   - Verify sample rate with MediaInfo or similar
   - Listen to files for quality issues
   - Check file sizes are reasonable

4. Performance testing
   - Time conversion of large batches
   - Monitor memory usage
   - Test with 10+ GB of audio files

## Build Instructions (Updated)

### CMake Build Process
```bash
cd lunch_box
mkdir -p build converted  # Create output directory
cd build
cmake ..
cmake --build .
```

### Running the Application
```bash
# Scan only (existing functionality)
./lunch_box /path/to/audio/folder

# Future: Scan and convert
./lunch_box --convert /path/to/audio/folder
./lunch_box --convert --output /custom/output /path/to/audio/folder
```

## Success Criteria

- [ ] Application compiles without errors
- [ ] Successfully converts various WAV formats to 16-bit 48kHz
- [ ] Preserves channel configuration (mono→mono, stereo→stereo)
- [ ] Correctly skips multi-channel files (>2 channels)
- [ ] Creates output directory if missing
- [ ] Original files remain unmodified
- [ ] Converted files are valid and playable
- [ ] Accurate conversion logging (before/after specs)
- [ ] Handles errors gracefully (corrupted files, disk full, etc.)
- [ ] Performance is acceptable (processes files in reasonable time)
- [ ] Clean, readable console output
- [ ] Log files capture all conversion operations

## Dependencies Status
- ✅ JUCE 8.0.12 installed at ~/Repos/JUCE
- ✅ C++ compiler (clang++ 17.0.0)
- ✅ CMake 4.2.1
- ✅ Milestone 1 complete (scanning functionality)

## Technical Considerations

### Sample Rate Conversion Quality
- ResamplingAudioSource uses interpolation for quality
- Higher quality = more CPU usage
- Consider adding quality settings in future

### Bit Depth Conversion
- 24-bit → 16-bit: JUCE handles dithering automatically
- May want to add dithering options later
- Possible clipping if levels too high (monitor for this)

### Memory Management
- Large files: process in chunks to avoid loading entire file
- ResamplingAudioSource handles buffering internally
- Monitor memory usage during testing

### File Naming Strategy
Options to consider:
1. **Same name** (simple, risk of confusion with originals)
2. **Suffix** (e.g., `song_converted.wav` - clear but longer)
3. **Preserve structure** (e.g., `converted/subfolder/song.wav` - maintains organization)

Recommendation: Start with option 1 (same name) in separate directory for simplicity.

### Error Handling Scenarios
- Source file locked/in use
- Insufficient disk space
- Write permission denied
- Invalid audio data
- Unexpected format variations

All should log error and continue processing remaining files.

## Future Considerations (Not in Milestone 2)
- Support for other input formats (AIFF, FLAC, MP3)
- Multiple output formats
- Batch processing with progress bars
- GUI interface for conversion settings
- Custom sample rates and bit depths
- Normalization/gain adjustment
- Metadata preservation
- Parallel processing for faster conversion
- Dry-run mode (preview without converting)

## Notes
- Focus on WAV-to-WAV conversion only for now
- Keep conversion logic separate from scanning logic (modularity)
- Maintain existing command-line interface for backward compatibility
- Log everything for debugging and user feedback
- Original scanner functionality must continue to work unchanged
