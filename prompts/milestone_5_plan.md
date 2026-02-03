# Milestone 5: Additional Input Format Support

## Objective
Expand input format support beyond WAV to include AIFF, MP3, and FLAC formats, enabling users to process audio files from diverse sources without pre-conversion.

## Requirements

### Supported Input Formats

**Current:**
- WAV (.wav) - 16/24/32-bit, any sample rate

**New Additions:**
- **AIFF (.aiff, .aif)** - Apple Interchange File Format
- **MP3 (.mp3)** - MPEG Audio Layer 3 (compressed)
- **FLAC (.flac)** - Free Lossless Audio Codec

### Output Format (Unchanged)
- WAV, 16-bit, 48kHz
- Mono or stereo preserved

### Processing Requirements

1. **Automatic Format Detection**
   - Detect format from file extension
   - Validate format compatibility
   - Report format in logs/status

2. **Quality Preservation**
   - Lossless formats (AIFF, FLAC) → preserve quality
   - Lossy formats (MP3) → accept quality as-is
   - All formats → convert to 16-bit 48kHz WAV output

3. **Error Handling**
   - Invalid/corrupted files → skip with warning
   - Unsupported bit rates → skip with warning
   - DRM-protected files → skip with warning

4. **File Discovery**
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
target_compile_definitions(chompi_pack
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CAMERA=0
        JUCE_USE_FLAC=1          # Enable FLAC
        JUCE_USE_MP3AUDIOFORMAT=1 # Enable MP3
)
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
- No changes needed (format-agnostic)
- AudioFormatReader handles all formats

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

### Phase 4: Testing

1. **Create Test Files**
   - Prepare sample files in each format
   - Various bit depths and sample rates
   - Include edge cases (very large, very small)

2. **Test Format Reading**
   - Verify each format can be read
   - Verify metadata extraction works
   - Verify sample data is correct

3. **Test Conversion Pipeline**
   - Convert AIFF → WAV
   - Convert MP3 → WAV
   - Convert FLAC → WAV
   - Verify output quality

4. **Test Mixed Format Processing**
   - Process folder with multiple formats
   - Verify alphabetical sorting across formats
   - Verify all formats converted correctly

5. **Test Error Handling**
   - Corrupted AIFF files
   - Invalid MP3 files
   - Truncated FLAC files
   - Verify graceful failure

### Phase 5: Documentation

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

- [ ] JUCE format configuration updated
- [ ] AIFF files can be read and converted
- [ ] MP3 files can be read and converted
- [ ] FLAC files can be read and converted
- [ ] Multiple formats can be processed in same batch
- [ ] Format detection works correctly
- [ ] Files sorted alphabetically regardless of format
- [ ] Format reported in logs and status
- [ ] Error handling works for all formats
- [ ] GUI shows format information
- [ ] CLI processes all formats
- [ ] Documentation updated
- [ ] Test files processed successfully

## Testing Plan

### Test Suite

**Format Reading Tests:**
- Read AIFF: 16-bit, 24-bit
- Read MP3: 128kbps, 192kbps, 320kbps
- Read FLAC: 16-bit, 24-bit
- Verify metadata extraction

**Conversion Tests:**
- AIFF → WAV (16-bit 48kHz)
- MP3 → WAV (16-bit 48kHz)
- FLAC → WAV (16-bit 48kHz)
- Verify output playable

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
- Verify 70-file limit with mixed formats
- Verify output naming correct

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

- Focus on common formats (AIFF, MP3, FLAC)
- Maintain 16-bit 48kHz WAV output (unchanged)
- No need to support output to other formats
- Processing logic remains the same
- Only input reading changes
- Quality considerations important for user expectations
- MP3 → WAV doesn't improve quality (inform users)

## Estimated Impact

**Code Changes:** Minimal
- CMakeLists.txt: 2 lines
- FileSystemHelper: ~20 lines
- Logger: ~10 lines
- Documentation: ~50 lines
- Tests: ~100 lines

**Testing:** Moderate
- Need sample files for each format
- Need to verify quality preservation
- Need to test error cases

**User Benefit:** High
- Eliminates pre-conversion step
- Supports common audio sources
- Simplifies workflow
- Maintains quality appropriately
