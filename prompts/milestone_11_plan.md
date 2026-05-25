# Milestone 11: Unit Testing Review

## Objective
Establish comprehensive unit testing coverage for all core modules, ensuring code reliability and facilitating future development.

## Requirements

### Testing Framework
- **Framework:** Catch2 or JUCE UnitTest framework
- **Coverage Goal:** 80%+ code coverage
- **Test Types:** Unit, integration, regression

### Test Scope

**Core Modules to Test:**
1. **LunchBoxNamer** ✅ (partially tested)
   - Bank/slot calculation
   - Filename generation
   - Edge cases (overflow, empty)

2. **AudioConverter**
   - Format conversion correctness
   - Sample rate conversion accuracy
   - Bit depth conversion
   - Channel preservation

3. **BankFolderParser** (Milestone 6)
   - Bank folder detection
   - File sorting and assignment
   - Mixed sorted/unsorted handling

4. **FileSystemHelper**
   - File discovery
   - Path validation
   - Extension filtering

5. **Logger**
   - File creation
   - Message formatting
   - Timestamp accuracy

6. **LunchBoxProcessor**
   - End-to-end processing
   - Error handling
   - Bank assignment accuracy

### Test Categories

**1. Unit Tests**
- Test individual functions in isolation
- Mock dependencies
- Fast execution (<100ms per test)

**2. Integration Tests**
- Test module interactions
- Use real file system (test fixtures)
- Verify complete workflows

**3. Regression Tests**
- Prevent reintroduction of fixed bugs
- Test known edge cases
- Validate backward compatibility

**4. Performance Tests**
- Measure conversion speed
- Check memory usage
- Validate large file handling

## Implementation Steps

### Phase 1: Test Infrastructure

1. **Setup Testing Framework**
   ```cmake
   # CMakeLists.txt
   enable_testing()

   juce_add_console_app(lunch_box_tests
       PRODUCT_NAME "Lunch Box Tests")

   target_sources(lunch_box_tests
       PRIVATE
           tests/TestMain.cpp
           tests/LunchBoxNamerTests.cpp
           tests/AudioConverterTests.cpp
           tests/BankFolderParserTests.cpp
           tests/FileSystemHelperTests.cpp
           tests/LoggerTests.cpp
           tests/IntegrationTests.cpp)

   target_link_libraries(lunch_box_tests
       PRIVATE
           juce::juce_core
           juce::juce_audio_formats
           Catch2::Catch2)
   ```

2. **Create Test Fixtures**
   - Test audio files (various formats)
   - Test folder structures
   - Expected output files
   - Mock data

### Phase 2: LunchBoxNamer Tests (Expand)

```cpp
TEST_CASE("LunchBoxNamer bank calculations", "[chompi_namer]")
{
    SECTION("First file goes to bank A slot 1")
    {
        auto result = LunchBoxNamer::indexToBankSlot(0);
        REQUIRE(result.bank == 'a');
        REQUIRE(result.slot == 1);
    }

    SECTION("15th file goes to bank B slot 1")
    {
        auto result = LunchBoxNamer::indexToBankSlot(14);
        REQUIRE(result.bank == 'b');
        REQUIRE(result.slot == 1);
    }

    SECTION("Last file goes to bank E slot 14")
    {
        auto result = LunchBoxNamer::indexToBankSlot(69);
        REQUIRE(result.bank == 'e');
        REQUIRE(result.slot == 14);
    }

    SECTION("Overflow index handled gracefully")
    {
        auto result = LunchBoxNamer::indexToBankSlot(70);
        REQUIRE(result.bank == '\0'); // Invalid
    }
}

TEST_CASE("LunchBoxNamer filename generation", "[chompi_namer]")
{
    SECTION("Cubbi filenames formatted correctly")
    {
        auto name = LunchBoxNamer::generateFileName(
            LunchBoxNamer::Category::Cubbi, 0);
        REQUIRE(name == "cubbi_a1.wav");
    }

    SECTION("Jammi filenames formatted correctly")
    {
        auto name = LunchBoxNamer::generateFileName(
            LunchBoxNamer::Category::Jammi, 13);
        REQUIRE(name == "jammi_a14.wav");
    }
}
```

### Phase 3: AudioConverter Tests

```cpp
TEST_CASE("AudioConverter format conversion", "[audio_converter]")
{
    Logger logger;
    AudioConverter converter(logger);
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    SECTION("Convert 44.1kHz to 48kHz")
    {
        juce::File input = getTestFile("44100hz_16bit.wav");
        juce::File output = getTempFile("output_48k.wav");

        auto result = converter.convertFile(input, output, formatManager);

        REQUIRE(result.success);

        // Verify output
        auto reader = formatManager.createReaderFor(output);
        REQUIRE(reader != nullptr);
        REQUIRE(reader->sampleRate == 48000.0);
        delete reader;
    }

    SECTION("Convert 24-bit to 16-bit")
    {
        juce::File input = getTestFile("24bit.wav");
        juce::File output = getTempFile("output_16bit.wav");

        auto result = converter.convertFile(input, output, formatManager);

        REQUIRE(result.success);

        auto reader = formatManager.createReaderFor(output);
        REQUIRE(reader->bitsPerSample == 16);
        delete reader;
    }

    SECTION("Preserve mono channel")
    {
        juce::File input = getTestFile("mono.wav");
        juce::File output = getTempFile("output_mono.wav");

        converter.convertFile(input, output, formatManager);

        auto reader = formatManager.createReaderFor(output);
        REQUIRE(reader->numChannels == 1);
        delete reader;
    }
}
```

### Phase 4: BankFolderParser Tests

```cpp
TEST_CASE("BankFolderParser bank detection", "[bank_parser]")
{
    SECTION("Single letter folders detected")
    {
        char bank;
        REQUIRE(BankFolderParser::isBankFolder("A", bank));
        REQUIRE(bank == 'a');

        REQUIRE(BankFolderParser::isBankFolder("e", bank));
        REQUIRE(bank == 'e');
    }

    SECTION("Bank prefix folders detected")
    {
        char bank;
        REQUIRE(BankFolderParser::isBankFolder("bank_a", bank));
        REQUIRE(bank == 'a');

        REQUIRE(BankFolderParser::isBankFolder("Bank C", bank));
        REQUIRE(bank == 'c');
    }

    SECTION("Invalid folders rejected")
    {
        char bank;
        REQUIRE_FALSE(BankFolderParser::isBankFolder("F", bank));
        REQUIRE_FALSE(BankFolderParser::isBankFolder("misc", bank));
        REQUIRE_FALSE(BankFolderParser::isBankFolder("AB", bank));
    }
}

TEST_CASE("BankFolderParser file assignment", "[bank_parser]")
{
    Logger logger;
    BankFolderParser parser(logger);

    // Create test folder structure
    auto testFolder = createTestFolderStructure();

    SECTION("Bank folders processed first")
    {
        auto assignments = parser.parseFolderStructure(
            testFolder, LunchBoxNamer::Category::Cubbi);

        // Files from bank A should have bank 'a'
        auto firstFile = assignments[0];
        REQUIRE(firstFile.bankLetter == 'a');
        REQUIRE(firstFile.fromBankFolder == true);
    }

    SECTION("Unsorted files fill remaining slots")
    {
        auto assignments = parser.parseFolderStructure(
            testFolder, LunchBoxNamer::Category::Cubbi);

        // Count unsorted assignments
        int unsortedCount = 0;
        for (const auto& a : assignments)
            if (!a.fromBankFolder) unsortedCount++;

        REQUIRE(unsortedCount > 0);
    }
}
```

### Phase 5: Integration Tests

```cpp
TEST_CASE("End-to-end CHOMPI processing", "[integration]")
{
    SECTION("Process simple folder structure")
    {
        // Setup test input
        auto cubbiFolder = createTestFolder("cubbi", 20);
        auto jammiFolder = createTestFolder("jammi", 15);
        auto outputFolder = getTempFolder();

        // Create configuration
        AudioConfiguration config;
        config.mode = OperationMode::Chompi;
        config.cubbiFolder = cubbiFolder;
        config.jammiFolder = jammiFolder;
        config.outputFolder = outputFolder;
        config.hasCubbi = true;
        config.hasJammi = true;

        // Process
        Logger logger;
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        LunchBoxProcessor processor(logger);

        bool success = processor.processSamples(config, formatManager);
        REQUIRE(success);

        // Verify output
        auto outputFiles = getFilesInFolder(outputFolder);
        REQUIRE(outputFiles.size() == 35); // 20 cubbi + 15 jammi

        // Verify naming
        REQUIRE(outputFolder.getChildFile("cubbi_a1.wav").exists());
        REQUIRE(outputFolder.getChildFile("jammi_a15.wav").exists());
    }
}
```

### Phase 6: Performance Tests

```cpp
TEST_CASE("Performance benchmarks", "[performance]")
{
    SECTION("Convert 100 files under 30 seconds")
    {
        auto files = createTestFiles(100);
        auto start = std::chrono::steady_clock::now();

        // Convert all files
        AudioConverter converter(logger);
        for (const auto& file : files)
        {
            converter.convertFile(file, getOutputFile(file), formatManager);
        }

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

        REQUIRE(duration.count() < 30);
    }
}
```

### Phase 7: CI Integration

1. **GitHub Actions Workflow**
   ```yaml
   name: Tests

   on: [push, pull_request]

   jobs:
     test:
       runs-on: macos-latest
       steps:
         - uses: actions/checkout@v2
         - name: Build
           run: |
             mkdir build
             cd build
             cmake ..
             make
         - name: Run Tests
           run: ./build/lunch_box_tests_artefacts/lunch_box_tests
   ```

2. **Code Coverage**
   - Use lcov or similar
   - Generate coverage reports
   - Publish to Codecov

## Success Criteria

- [ ] 80%+ code coverage
- [ ] All core modules tested
- [ ] Unit tests run in <5 seconds
- [ ] Integration tests run in <30 seconds
- [ ] CI pipeline configured
- [ ] Code coverage reporting enabled
- [ ] All tests passing
- [ ] Regression tests for known bugs
- [ ] Performance benchmarks established
- [ ] Test documentation written

## Dependencies

- ➕ Catch2 or JUCE UnitTest framework
- ➕ Test fixtures and sample files
- ➕ CI/CD pipeline

## Estimated Impact

- Code: ~1500 lines of test code
- Confidence: Very High
- Regression Prevention: High
- Documentation: Test serve as usage examples
