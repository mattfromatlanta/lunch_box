# Milestone 12: Code Review and Refactoring

## Objective
Comprehensive code review to improve code quality, maintainability, performance, and adherence to best practices before open-source publication.

## Review Scope

### Code Quality
- Consistent coding style
- Clear naming conventions
- Proper error handling
- Memory management
- Resource cleanup

### Architecture
- Module boundaries clear
- Dependencies well-managed
- SOLID principles followed
- Design patterns appropriate

### Performance
- Efficient algorithms
- No unnecessary copies
- Proper use of const/references
- Memory footprint reasonable

### Documentation
- All public APIs documented
- Complex logic explained
- Examples provided
- Comments accurate and useful

## Review Checklist

### 1. Naming Conventions

**Classes:** PascalCase
- `ChompiNamer`, `AudioConverter`, `BankFolderParser` ✅

**Functions:** camelCase
- `processFiles()`, `generateFileName()` ✅

**Variables:** camelCase
- `selectedFolder`, `outputPath` ✅

**Constants:** UPPER_SNAKE_CASE or kPascalCase
- `MAX_FILES_PER_BANK` or `kMaxFilesPerBank`

**Private members:** Trailing underscore (optional)
- `logger_` or just `logger` (be consistent)

### 2. Error Handling

**Current Issues:**
- Some functions don't check return values
- Not all exceptions caught
- Error messages could be more specific

**Improvements:**
```cpp
// Before
void processFile(const juce::File& file)
{
    auto reader = formatManager.createReaderFor(file);
    // What if reader is nullptr?
    process(reader);
}

// After
bool processFile(const juce::File& file, juce::String& errorMessage)
{
    auto reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
    {
        errorMessage = "Failed to create reader for: " + file.getFullPathName();
        return false;
    }

    try
    {
        process(reader);
        delete reader;
        return true;
    }
    catch (const std::exception& e)
    {
        errorMessage = "Processing failed: " + juce::String(e.what());
        delete reader;
        return false;
    }
}
```

### 3. Memory Management

**Smart Pointers:**
- Use `std::unique_ptr` for owned objects
- Use `std::shared_ptr` sparingly (only when needed)
- Avoid raw `new`/`delete`

**JUCE Objects:**
- Use `juce::OwnedArray` for arrays of owned objects
- Use `juce::ReferenceCountedObjectPtr` for ref-counted objects
- Understand JUCE ownership model

**Resources:**
- RAII for file handles
- Proper cleanup in destructors
- No leaked AudioFormatReaders

### 4. Const Correctness

```cpp
// Good
const juce::String& getFileName() const;
void processFiles(const juce::Array<juce::File>& files);

// Avoid non-const references (prefer const or value)
void setName(juce::String& name);  // Bad
void setName(const juce::String& name);  // Good
void setName(juce::String name);  // Also good (for small types)
```

### 5. Header Organization

```cpp
// MyClass.h
#pragma once

// System includes
#include <vector>
#include <memory>

// Third-party includes
#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>

// Project includes
#include "Logger.h"
#include "AudioConfiguration.h"

// Forward declarations (prefer over includes)
class AudioConverter;
class ChompiNamer;

class MyClass
{
    // Public interface first
public:
    MyClass();
    ~MyClass();

    // Then protected
protected:
    virtual void onProcessComplete();

    // Then private
private:
    void internalHelper();

    // Member variables last
private:
    Logger& logger;
    std::unique_ptr<AudioConverter> converter;
};
```

### 6. Function Length

- Functions > 50 lines should be split
- Each function does one thing
- Extract complex logic into helper functions

**Example Refactoring:**
```cpp
// Before: 200-line function
void ChompiProcessor::processCategory(...)
{
    // 50 lines of file discovery
    // 50 lines of sorting
    // 50 lines of assignment
    // 50 lines of conversion
}

// After: Multiple focused functions
void ChompiProcessor::processCategory(...)
{
    auto files = discoverFiles(sourceFolder);
    auto sorted = sortFiles(files);
    auto assignments = createAssignments(sorted, category);
    convertFiles(assignments, outputFolder);
}
```

### 7. Code Duplication

**Identify duplicated code:**
- Search for copy-pasted blocks
- Extract common logic into functions
- Use templates for type-generic code

**Example:**
```cpp
// Before: Duplicated validation
void validateCubbiFolder(const juce::File& folder)
{
    if (!folder.exists()) /* error */
    if (!folder.isDirectory()) /* error */
    // ... more checks
}

void validateJammiFolder(const juce::File& folder)
{
    if (!folder.exists()) /* error */
    if (!folder.isDirectory()) /* error */
    // ... same checks
}

// After: Shared function
bool validateAudioFolder(const juce::File& folder,
                        const juce::String& folderType,
                        juce::String& errorMsg)
{
    if (!folder.exists())
    {
        errorMsg = folderType + " folder does not exist";
        return false;
    }
    // ... unified validation
    return true;
}
```

### 8. Comments and Documentation

**Good Comments:**
- Explain WHY, not WHAT
- Document assumptions
- Note edge cases
- Reference external docs (CHOMPI specs)

**Bad Comments:**
```cpp
// Increment i
i++;

// Create a reader
auto reader = formatManager.createReaderFor(file);
```

**Good Comments:**
```cpp
// CHOMPI sampler requires exactly 14 slots per bank
const int SLOTS_PER_BANK = 14;

// Convert to 48kHz because CHOMPI hardware only supports 48kHz
targetSampleRate = 48000.0;

// Skip files >2 channels (CHOMPI doesn't support multi-channel)
if (numChannels > 2)
    continue;
```

**API Documentation:**
```cpp
/**
 * Converts an audio file to CHOMPI-compatible format.
 *
 * @param sourceFile    Input audio file (WAV, AIFF, MP3, FLAC)
 * @param outputFile    Output WAV file path
 * @param formatManager JUCE format manager with registered formats
 *
 * @returns ConversionResult with success status and error message
 *
 * @note Output is always 16-bit 48kHz WAV
 * @note Mono/stereo is preserved; multi-channel files are skipped
 */
ConversionResult convertFile(const juce::File& sourceFile,
                             const juce::File& outputFile,
                             juce::AudioFormatManager& formatManager);
```

### 9. Performance Review

**Profiling Targets:**
- File conversion speed
- GUI responsiveness
- Memory usage during processing
- Startup time

**Optimization Opportunities:**
- Unnecessary file reads
- Inefficient sorting algorithms
- Memory allocations in loops
- String concatenation in loops

### 10. Security Review

**File Path Validation:**
- Check for path traversal attacks
- Validate file extensions
- Limit file sizes
- Check available disk space

**User Input:**
- Sanitize folder paths
- Validate command-line arguments
- Prevent buffer overflows

## Implementation Steps

### Phase 1: Static Analysis

1. **Run clang-tidy**
   ```bash
   clang-tidy Source/*.cpp -- -I/path/to/JUCE
   ```

2. **Run cppcheck**
   ```bash
   cppcheck --enable=all Source/
   ```

3. **Review warnings**
   - Fix all warnings
   - Understand suppressed warnings
   - Document exceptions

### Phase 2: Code Review

1. **Review Each Module**
   - ChompiNamer
   - AudioConverter
   - BankFolderParser
   - FileSystemHelper
   - Logger
   - ChompiProcessor
   - GUI components
   - CLI components

2. **Apply Checklist**
   - Naming conventions
   - Error handling
   - Memory management
   - Const correctness
   - Function length
   - Code duplication
   - Comments

### Phase 3: Refactoring

1. **Extract Functions**
   - Split long functions
   - Extract duplicated code
   - Create helper classes

2. **Improve Naming**
   - Rename unclear variables
   - Rename misleading functions
   - Add type suffixes if needed

3. **Add Documentation**
   - Doxygen comments for public APIs
   - Inline comments for complex logic
   - Update README/HOW_TO

### Phase 4: Performance Optimization

1. **Profile Critical Paths**
   - File conversion
   - Folder scanning
   - Bank assignment

2. **Optimize Hotspots**
   - Only if profiling shows issue
   - Measure before/after
   - Don't sacrifice readability

### Phase 5: Final Review

1. **Peer Review**
   - External code review (if possible)
   - Fresh eyes on code
   - User testing

2. **Documentation Pass**
   - Ensure all docs current
   - Fix outdated comments
   - Update examples

## Success Criteria

- [ ] All static analysis warnings fixed
- [ ] Code passes style guidelines
- [ ] No memory leaks (Valgrind/Instruments)
- [ ] All public APIs documented
- [ ] No code duplication
- [ ] Functions < 50 lines
- [ ] Headers properly organized
- [ ] Error handling comprehensive
- [ ] Performance acceptable
- [ ] Security review complete
- [ ] Peer review complete
- [ ] User testing positive

## Tools

- **clang-tidy** - Static analysis
- **cppcheck** - Additional static analysis
- **Valgrind** (Linux) - Memory leak detection
- **Instruments** (macOS) - Profiling and leak detection
- **Doxygen** - Documentation generation
- **clang-format** - Code formatting

## Estimated Impact

- Time: 2-3 weeks (thorough review)
- Code changes: 500+ lines refactored
- Quality: Significantly improved
- Maintainability: Much easier for contributors
- Professional: Ready for open source
