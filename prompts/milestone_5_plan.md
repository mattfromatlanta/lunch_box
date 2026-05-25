# Milestone 5: Additional Input Format Support + Optimized Sample Generation

## Objective
Expand input format support beyond WAV to include AIFF, MP3, and FLAC formats, and automatically generate CHOMPI-optimized samples (pitched up one octave) for all outputs, enabling users to process audio files from diverse sources and create complete CHOMPI libraries.

## Requirements

### Supported Input Formats

**Current:**
- WAV (.wav) - 16/24/32-bit, any sample rate

**New Additions:**
- **AIFF (.aiff, .aif)** - Apple Interchange File Format
- **MP3 (.mp3)** - MPEG Audio Layer 3 (compressed)
- **FLAC (.flac)** - Free Lossless Audio Codec

### Output Format

**Base Samples:**
- WAV, 16-bit, 48kHz
- Mono or stereo preserved
- Standard CHOMPI naming: `cubbi_a1.wav`, `jammi_b5.wav`, etc.

**Optimized Samples (New):**
- WAV, 16-bit, 48kHz
- Pitched up one octave (double speed)
- Appends `_double` suffix: `cubbi_a1_double.wav`, `jammi_b5_double.wav`, etc.
- Automatically generated for every input sample
- CHOMPI hardware creates these if absent; we generate them proactively

**Output Volume:**
- Previous: 70 files per category max (70 cubbi + 70 jammi = 140 total)
- New: 140 files per category max (70 base + 70 optimized per category = 280 total)
- Input limit: 70 samples per category (hardware bank limit unchanged)
- Output ratio: 2 files per input sample (base + optimized)

### Processing Requirements

1. **Automatic Format Detection**
   - Detect format from file extension
   - Validate format compatibility
   - Report format in logs/status

2. **Quality Preservation**
   - Lossless formats (AIFF, FLAC) → preserve quality
   - Lossy formats (MP3) → accept quality as-is
   - All formats → convert to 16-bit 48kHz WAV output

3. **Duration Validation**
   - Maximum duration: 2 minutes (120 seconds) for base samples
   - Rationale: Optimized samples will be 1 minute (60 seconds) at double speed
   - Samples exceeding limit → skip with warning
   - Report duration in validation logs

4. **Optimized Sample Generation**
   - For each input sample, generate TWO output files:
     - Base sample: `{category}_{bank}{slot}.wav` (e.g., `cubbi_a1.wav`)
     - Optimized sample: `{category}_{bank}{slot}_double.wav` (e.g., `cubbi_a1_double.wav`)
   - Optimized sample = pitched up one octave (double playback speed)
   - Both files: 16-bit 48kHz WAV format
   - Preserves mono/stereo configuration in both versions

5. **Error Handling**
   - Invalid/corrupted files → skip with warning
   - Unsupported bit rates → skip with warning
   - DRM-protected files → skip with warning
   - Samples exceeding duration limit → skip with warning

6. **File Discovery**
   - Update recursive search to find: *.wav, *.aiff, *.aif, *.mp3, *.flac
   - Sort all formats together alphabetically
   - No format-based prioritization

## Technical Architecture

### JUCE Format Support

JUCE's `AudioFormatManager` supports multiple formats through format readers:

**Built-in Formats (already available):**
- `WavAudioFormat` - WAV files ✅ (already in use)
- `AiffAudioFormat` - AIFF files ➕ (add registration)

**Additional Formats (require plugins/libs):**
- MP3: Via JUCE's built-in MP3 decoder (requires enabling)
- FLAC: Via JUCE's FLAC support (built-in, requires enabling)

### Format Manager Registration

Current code:
```cpp
formatManager.registerBasicFormats();
```

This registers WAV and AIFF by default. Need to ensure FLAC and MP3 are enabled.

### CMakeLists.txt Updates

Add compiler definitions:
```cmake
target_compile_definitions(lunch_box
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CAMERA=0
        JUCE_USE_FLAC=1          # Enable FLAC
        JUCE_USE_MP3AUDIOFORMAT=1 # Enable MP3
)
```

### Optimized Sample Generation Architecture

**Pitch-Shifting Approach:**

Pitching up one octave = doubling playback speed = halving sample count while preserving pitch relationship.

**Implementation using JUCE ResamplingAudioSource:**

```cpp
// Pseudo-code for optimized sample generation
void generateOptimizedSample(const juce::File& baseFile,
                             const juce::File& optimizedFile,
                             juce::AudioFormatManager& formatManager)
{
    // 1. Read base sample
    auto reader = formatManager.createReaderFor(baseFile);

    // 2. Create resampler with 2.0 ratio (double speed = octave up)
    juce::ResamplingAudioSource resampler(reader, false, 2);
    resampler.setResamplingRatio(2.0); // Double speed

    // 3. Prepare output buffer (half the length of original)
    int numSamples = reader->lengthInSamples / 2;
    juce::AudioBuffer<float> buffer(reader->numChannels, numSamples);

    // 4. Process through resampler
    resampler.prepareToPlay(numSamples, 48000.0);
    juce::AudioSourceChannelInfo info(&buffer, 0, numSamples);
    resampler.getNextAudioBlock(info);

    // 5. Write to output file with _double suffix
    writeWavFile(optimizedFile, buffer, 48000.0, 16);
}
```

**Alternative Approach (Simpler):**

Since we're already converting to 48kHz, we can:
1. Read source at 48kHz
2. For optimized version: Write every other sample (decimate by 2)
3. Result: Halved duration, doubled perceived pitch

**Duration Validation:**

```cpp
bool validateSampleDuration(juce::AudioFormatReader* reader)
{
    double durationSeconds = reader->lengthInSamples / reader->sampleRate;
    const double MAX_DURATION = 120.0; // 2 minutes

    if (durationSeconds > MAX_DURATION)
    {
        logger.logWarning("Sample exceeds 2-minute limit: " +
                         juce::String(durationSeconds, 1) + "s");
        return false;
    }
    return true;
}
```

### Code Changes Required

**1. FileSystemHelper Updates**
- Update file search patterns
- Add method to get all supported extensions
- Update file counting/validation

**2. Logger Updates**
- Display source format in logs
- Report format conversions clearly
- Add format-specific warnings

**3. AudioConverter Updates**
- Add `generateOptimizedSample()` method for creating _double versions
- Add duration validation before processing
- Modify main conversion loop to generate both base and optimized samples
- AudioFormatReader remains format-agnostic for reading

**4. GUI Updates**
- Update file validation to check all formats
- Display detected format in status
- Update documentation strings

## Implementation Steps

### Phase 1: JUCE Format Configuration

1. **Update CMakeLists.txt**
   - Add `JUCE_USE_FLAC=1` definition
   - Add `JUCE_USE_MP3AUDIOFORMAT=1` definition
   - Verify JUCE modules support these formats

2. **Test Format Manager Registration**
   - Verify `registerBasicFormats()` includes AIFF
   - Verify FLAC registration works
   - Verify MP3 registration works
   - Test reading sample files

### Phase 2: File Discovery Updates

1. **Update FileSystemHelper**
   ```cpp
   // Add method to get supported extensions
   static juce::StringArray getSupportedAudioExtensions()
   {
       return {"*.wav", "*.aiff", "*.aif", "*.mp3", "*.flac"};
   }

   // Update findAudioFiles method
   static void findAudioFiles(const juce::File& folder,
                             juce::Array<juce::File>& results)
   {
       for (const auto& pattern : getSupportedAudioExtensions())
       {
           folder.findChildFiles(results,
                               juce::File::findFiles,
                               true,
                               pattern);
       }
       // Sort all found files alphabetically
       results.sort();
   }
   ```

2. **Update CLI Argument Parsing**
   - Update usage message to list supported formats
   - Update validation to accept all formats

3. **Update GUI File Validation**
   - Check for all supported formats
   - Display format counts in status
   - Update warning messages

### Phase 3: Format Detection and Logging

1. **Add Format Detection Utility**
   ```cpp
   static juce::String getAudioFormatName(const juce::File& file)
   {
       auto ext = file.getFileExtension().toLowerCase();
       if (ext == ".wav") return "WAV";
       if (ext == ".aiff" || ext == ".aif") return "AIFF";
       if (ext == ".mp3") return "MP3";
       if (ext == ".flac") return "FLAC";
       return "Unknown";
   }
   ```

2. **Update Logging**
   - Show source format in conversion logs
   - Example: "Converting: song.mp3 (MP3) → cubbi_a1.wav"
   - Report format statistics in summary

3. **Add Format-Specific Warnings**
   - Warn if MP3 source quality is low
   - Warn if FLAC file is very large
   - Inform about lossy → lossless conversion

### Phase 4: Optimized Sample Generation

1. **Add Duration Validation**
   ```cpp
   // In AudioConverter or LunchBoxProcessor
   bool validateSampleDuration(juce::AudioFormatReader* reader,
                              const juce::File& file,
                              juce::String& errorMessage)
   {
       const double MAX_DURATION_SECONDS = 120.0;
       double duration = reader->lengthInSamples / reader->sampleRate;

       if (duration > MAX_DURATION_SECONDS)
       {
           errorMessage = juce::String("Sample exceeds 2-minute limit: ") +
                         file.getFileName() + " (" +
                         juce::String(duration, 1) + "s)";
           return false;
       }
       return true;
   }
   ```

2. **Implement Pitch-Shifting Function**
   ```cpp
   bool generateOptimizedSample(const juce::File& baseFile,
                               const juce::File& optimizedFile,
                               juce::AudioFormatManager& formatManager)
   {
       // Read base file
       std::unique_ptr<juce::AudioFormatReader> reader(
           formatManager.createReaderFor(baseFile));

       if (reader == nullptr)
           return false;

       // Create buffer for optimized version (half length)
       int optimizedSamples = reader->lengthInSamples / 2;
       juce::AudioBuffer<float> buffer(reader->numChannels, optimizedSamples);

       // Read and resample (simple decimation approach)
       juce::AudioBuffer<float> tempBuffer(reader->numChannels,
                                          reader->lengthInSamples);
       reader->read(&tempBuffer, 0, reader->lengthInSamples, 0, true, true);

       // Decimate by 2 (take every other sample)
       for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
       {
           for (int i = 0; i < optimizedSamples; ++i)
           {
               buffer.setSample(ch, i, tempBuffer.getSample(ch, i * 2));
           }
       }

       // Write optimized file
       return writeWavFile(optimizedFile, buffer, 48000.0, 16);
   }
   ```

3. **Update LunchBoxProcessor Conversion Loop**
   ```cpp
   // For each input file, generate both base and optimized
   for (const auto& assignment : assignments)
   {
       // Generate base sample
       juce::File baseOutput = outputFolder.getChildFile(assignment.outputName);
       bool baseSuccess = convertFile(assignment.sourceFile, baseOutput);

       if (baseSuccess)
       {
           // Generate optimized sample with _double suffix
           juce::String optimizedName = assignment.outputName.replace(".wav",
                                                                      "_double.wav");
           juce::File optimizedOutput = outputFolder.getChildFile(optimizedName);
           bool optSuccess = generateOptimizedSample(baseOutput,
                                                    optimizedOutput,
                                                    formatManager);

           logger.log("Generated: " + assignment.outputName +
                     " + optimized version");
       }
   }
   ```

4. **Update Logging**
   - Log both base and optimized file creation
   - Report total output count (2x input count)
   - Show duration for samples approaching limit

### Phase 5: Testing

1. **Create Test Files**
   - Prepare sample files in each format (WAV, AIFF, MP3, FLAC)
   - Various bit depths and sample rates
   - Various durations: 10s, 60s, 119s, 121s (over limit)
   - Include edge cases (very large, very small)

2. **Test Format Reading**
   - Verify each format can be read
   - Verify metadata extraction works
   - Verify sample data is correct

3. **Test Conversion Pipeline**
   - Convert AIFF → WAV (base + _double)
   - Convert MP3 → WAV (base + _double)
   - Convert FLAC → WAV (base + _double)
   - Verify output quality for both versions

4. **Test Optimized Sample Generation**
   - Verify _double files created for all inputs
   - Verify _double files are half the duration
   - Verify _double files playback at higher pitch
   - Verify both mono and stereo preserved correctly
   - Verify 16-bit 48kHz format maintained

5. **Test Duration Validation**
   - Process 119-second sample → should succeed
   - Process 121-second sample → should be skipped with warning
   - Verify log messages for rejected files
   - Verify duration reported accurately

6. **Test Mixed Format Processing**
   - Process folder with multiple formats
   - Verify alphabetical sorting across formats
   - Verify all formats converted to base + _double
   - Verify output count = 2x input count

7. **Test Error Handling**
   - Corrupted AIFF files
   - Invalid MP3 files
   - Truncated FLAC files
   - Over-duration files
   - Verify graceful failure

8. **Test Capacity Limits**
   - Process 70 input files → expect 140 output files per category
   - Verify bank assignment correct with doubled output
   - Verify logs report correct file counts

### Phase 6: Documentation

1. **Update README.md**
   - List all supported formats
   - Add format support to features list
   - Update technical specifications

2. **Update HOW_TO.md**
   - Add section on supported formats
   - Add format conversion notes
   - Add quality considerations

3. **Update Usage Messages**
   - CLI usage to mention formats
   - GUI tooltips to mention formats

## Format-Specific Considerations

### AIFF (Audio Interchange File Format)

**Characteristics:**
- Uncompressed, like WAV
- Commonly used on macOS
- Supports various bit depths
- Very similar to WAV internally

**Implementation:**
- Already supported by JUCE's `registerBasicFormats()`
- No additional configuration needed
- Conversion is straightforward

### MP3 (MPEG Audio Layer 3)

**Characteristics:**
- Lossy compression (typically 128-320 kbps)
- Smaller file sizes
- Quality loss during encoding
- Very common format

**Implementation:**
- JUCE supports MP3 decoding
- Requires `JUCE_USE_MP3AUDIOFORMAT=1`
- May have slight quality degradation
- Cannot improve quality during conversion

**Considerations:**
- Converting MP3 → WAV doesn't improve quality
- Useful for processing DJ sets, podcasts
- Warn user about lossy source quality

### FLAC (Free Lossless Audio Codec)

**Characteristics:**
- Lossless compression
- Smaller than WAV/AIFF
- Perfect quality preservation
- Popular for archival

**Implementation:**
- JUCE supports FLAC decoding
- Requires `JUCE_USE_FLAC=1`
- Decompression is automatic
- Quality fully preserved

**Considerations:**
- Decompression is CPU-intensive (but fast)
- Large FLAC files may take longer
- Perfect for high-quality sources

## Success Criteria

**Format Support:**
- [ ] JUCE format configuration updated (FLAC and MP3 enabled)
- [ ] AIFF files can be read and converted
- [ ] MP3 files can be read and converted
- [ ] FLAC files can be read and converted
- [ ] Multiple formats can be processed in same batch
- [ ] Format detection works correctly
- [ ] Files sorted alphabetically regardless of format
- [ ] Format reported in logs and status

**Optimized Sample Generation:**
- [ ] Base samples generated with standard naming (e.g., `cubbi_a1.wav`)
- [ ] Optimized samples generated with _double suffix (e.g., `cubbi_a1_double.wav`)
- [ ] Optimized samples are half duration of base samples
- [ ] Optimized samples playback at higher pitch (one octave up)
- [ ] Both base and optimized files are 16-bit 48kHz WAV
- [ ] Mono/stereo preserved in both versions
- [ ] Output count = 2x input count (verified in logs)

**Duration Validation:**
- [ ] Samples under 2 minutes processed successfully
- [ ] Samples over 2 minutes skipped with warning
- [ ] Duration reported in validation logs
- [ ] Appropriate error messages for rejected files

**General:**
- [ ] Error handling works for all formats
- [ ] GUI shows format information and output count
- [ ] CLI processes all formats and generates both versions
- [ ] Documentation updated (README, HOW_TO, CLAUDE.MD)
- [ ] Test files processed successfully (all formats + durations)

## Testing Plan

### Test Suite

**Format Reading Tests:**
- Read AIFF: 16-bit, 24-bit
- Read MP3: 128kbps, 192kbps, 320kbps
- Read FLAC: 16-bit, 24-bit
- Verify metadata extraction

**Conversion Tests:**
- AIFF → WAV base + _double (16-bit 48kHz each)
- MP3 → WAV base + _double (16-bit 48kHz each)
- FLAC → WAV base + _double (16-bit 48kHz each)
- Verify both outputs playable
- Verify _double version is half duration
- Verify _double version sounds pitched up

**Mixed Format Tests:**
- Folder with WAV + AIFF + MP3 + FLAC
- Verify alphabetical sorting
- Verify all converted
- Verify bank assignments correct

**Error Handling Tests:**
- Corrupted AIFF file
- Invalid MP3 file
- Truncated FLAC file
- Non-audio file with audio extension

**CHOMPI Mode Tests:**
- Process cubbi with mixed formats
- Process jammi with mixed formats
- Process 35 input files → expect 70 output files (35 base + 35 _double)
- Process 70 input files (max) → expect 140 output files (70 base + 70 _double)
- Verify output naming correct (base and _double suffix)
- Verify bank assignments correct
- Verify _double files have correct suffix

## Dependencies

- ✅ JUCE 8.0.12+ with FLAC and MP3 support
- ✅ Milestone 4 complete (GUI and CLI)
- ➕ JUCE_USE_FLAC=1 enabled
- ➕ JUCE_USE_MP3AUDIOFORMAT=1 enabled

## Future Considerations (Not in Milestone 5)

- **Ogg Vorbis** (.ogg) support
- **Opus** (.opus) support
- **AAC/M4A** (.m4a) support (requires additional libraries)
- **Format preference** (prioritize lossless over lossy)
- **Quality warnings** (alert for low bitrate MP3s)
- **Batch format conversion** (convert to specific format before processing)

## Notes

**Format Support:**
- Focus on common formats (AIFF, MP3, FLAC)
- Maintain 16-bit 48kHz WAV output (unchanged)
- No need to support output to other formats
- Quality considerations important for user expectations
- MP3 → WAV doesn't improve quality (inform users)

**Optimized Sample Generation:**
- CHOMPI hardware automatically creates _double files if not present
- We generate them proactively to create complete libraries
- Optimized = pitched up one octave (double playback speed)
- Both base and _double files go into CHOMPI library
- Halves duration: 2-minute base → 1-minute optimized
- Total storage: doubles output file count but maintains 70 slot limit

**Capacity:**
- Input limit: 70 samples per category (CHOMPI hardware bank structure)
- Output per category: 140 files (70 base + 70 _double)
- Total output: 280 files maximum (140 cubbi + 140 jammi)
- Bank structure unchanged: 5 banks × 14 slots = 70 per category

**Duration Limits:**
- Maximum input duration: 2 minutes (120 seconds)
- Resulting optimized duration: 1 minute (60 seconds)
- Files exceeding limit skipped with warning
- Rationale: Keep sample loading and memory reasonable on hardware

## Estimated Impact

**Code Changes:** Moderate
- CMakeLists.txt: 2 lines (format support)
- FileSystemHelper: ~20 lines (multi-format search)
- AudioConverter: ~100 lines (optimized sample generation + duration validation)
- LunchBoxProcessor: ~30 lines (dual-output logic)
- Logger: ~20 lines (enhanced logging)
- Documentation: ~150 lines (formats + optimized samples + limits)
- Tests: ~200 lines (formats + duration + optimized outputs)

**Testing:** Significant
- Need sample files for each format
- Need various duration test files
- Need to verify quality preservation
- Need to verify _double generation
- Need to test error cases
- Need to validate pitch-shifting quality

**User Benefit:** Very High
- Eliminates pre-conversion step
- Supports common audio sources
- Generates complete CHOMPI libraries (base + optimized)
- No manual _double file creation needed
- Simplifies workflow significantly
- Maintains quality appropriately
- Ensures CHOMPI hardware doesn't need to generate files
