# Contributing to Lunch Box

Thank you for your interest in contributing!

## How to Contribute

### Reporting Bugs

1. Check [existing issues](https://github.com/mattfromatlanta/lunch_box/issues)
2. Create a new issue with:
   - Clear description and steps to reproduce
   - Expected vs. actual behavior
   - OS, app version, and any relevant log output from the `logs/` directory

### Suggesting Features

Open a [discussion](https://github.com/mattfromatlanta/lunch_box/discussions) in the Ideas category with a description of the use case.

### Pull Requests

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Make your changes following the code style below
4. Run tests and verify the build
5. Open a pull request with a clear description of the change and why

## Development Setup

**Prerequisites:**
- C++17 compiler (Clang on macOS, GCC/Clang on Linux, MSVC on Windows)
- CMake 3.22+
- [JUCE](https://github.com/juce-framework/JUCE) 8.0.12+

```bash
# Update the JUCE path in CMakeLists.txt to point at your local JUCE install, then:
mkdir build && cd build
cmake ..
make
```

**Running tests:**
```bash
./build/lunch_box_tests_artefacts/lunch_box_tests
```

## Code Style

- Classes: `PascalCase` — functions/variables: `camelCase` — constants: `UPPER_SNAKE_CASE`
- `#pragma once` for header guards
- `std::unique_ptr` for owned objects; JUCE `OwnedArray` for components
- No exceptions in hot paths — return `bool` or result structs
- Comments only when the *why* is non-obvious — never explain what the code does

## Architecture

Processing logic lives in `Source/Processing/` and is shared between CLI and GUI. Neither
interface should reach into the other, and neither should bypass `LunchBoxProcessor`. See
`CLAUDE.md` for the full architecture overview.

## Important Constraints

- CHOMPI format is fixed: 16-bit 48kHz WAV, 5 banks x 14 slots, CHOMPI naming convention
- Never modify input files
- CLI backward compatibility must be preserved -- test CLI after any change to shared code

## Questions?

Open a [discussion](https://github.com/mattfromatlanta/lunch_box/discussions).
