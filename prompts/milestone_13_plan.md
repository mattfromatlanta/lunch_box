# Milestone 13: Open Source Publication Preparation

## Objective
Prepare Lunch Box for public release as an open-source project, including licensing, documentation, community guidelines, and distribution infrastructure.

## Requirements

### Legal and Licensing
- Choose appropriate open-source license
- Ensure dependency licenses compatible
- Add license headers to source files
- Copyright attribution

### Documentation
- Professional README
- Contribution guidelines
- Code of conduct
- Installation instructions
- Build instructions (all platforms)
- User guide
- Developer documentation
- Changelog

### Repository Setup
- Clean git history
- Organized file structure
- `.gitignore` properly configured
- Issue templates
- Pull request templates
- GitHub Actions CI/CD

### Community Infrastructure
- Issue tracking
- Discussion forums
- Communication channels
- Project website (optional)
- Social media presence (optional)

### Distribution
- Release builds (macOS, Windows, Linux)
- Installation packages
- Homebrew formula (macOS)
- Chocolatey package (Windows)
- Snap/AppImage (Linux)

## License Selection

### Recommended: MIT License

**Pros:**
- Permissive and simple
- Allows commercial use
- Minimal restrictions
- Good for adoption

**MIT License Text:**
```
MIT License

Copyright (c) 2026 Matt from Atlanta

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### Alternative: GPL v3

**Use if:**
- Want to ensure derivative works stay open source
- Strong copyleft preference
- Align with free software philosophy

**Note:** JUCE has dual licensing (GPL or commercial). Using GPL is compatible.

### Dependency Licenses

**JUCE:** GPL v3 or ISC (depending on modules used)
- Our use: ISC modules only (audio, GUI) - permissive ✅
- Compatible with MIT ✅

**Catch2:** Boost Software License (permissive) ✅

## Documentation Structure

### 1. README.md (Enhanced)

```markdown
# Lunch Box

![Logo](docs/assets/logo.png)

**Professional audio sample processor for the CHOMPI sampler**

[![Build Status](https://github.com/user/lunch_box/workflows/Tests/badge.svg)](https://github.com/user/lunch_box/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/user/lunch_box)](https://github.com/user/lunch_box/releases)

## Features

- 🎚️ **GUI & CLI** - Both graphical and command-line interfaces
- 🔄 **Format Support** - WAV, AIFF, MP3, FLAC input
- 🏦 **Bank Organization** - Automatic CHOMPI bank assignment
- 📂 **Smart Parsing** - Recognizes bank folder structures
- 🎯 **Precise Control** - Individual sample management
- 🎨 **Modern UI** - Professional, intuitive interface
- ⚡ **Fast** - ~44 MB/sec conversion throughput

## Quick Start

### macOS
```bash
brew install chompi-pack
chompi-pack
```

### Windows
```powershell
choco install chompi-pack
chompi-pack
```

### Linux
```bash
snap install chompi-pack
chompi-pack
```

### Build from Source
```bash
git clone https://github.com/user/lunch_box.git
cd lunch_box
mkdir build && cd build
cmake ..
make
```

## Documentation

- [User Guide](docs/USER_GUIDE.md)
- [Developer Guide](docs/DEVELOPER.md)
- [API Documentation](docs/API.md)
- [Changelog](CHANGELOG.md)

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details.

## License

MIT License - see [LICENSE](LICENSE) for details.

## Credits

- **Author:** Matt from Atlanta
- **Framework:** [JUCE](https://juce.com/)
- **CHOMPI Sampler:** [creditor.technology](https://creditor.technology)

## Support

- **Issues:** [GitHub Issues](https://github.com/user/lunch_box/issues)
- **Discussions:** [GitHub Discussions](https://github.com/user/lunch_box/discussions)
- **Email:** support@chompipack.com (optional)
```

### 2. CONTRIBUTING.md

```markdown
# Contributing to Lunch Box

Thank you for your interest in contributing!

## Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md).

## How to Contribute

### Reporting Bugs

1. Check [existing issues](https://github.com/user/lunch_box/issues)
2. Create a new issue with:
   - Clear title
   - Steps to reproduce
   - Expected vs actual behavior
   - System info (OS, JUCE version)
   - Log files if applicable

### Suggesting Features

1. Check [discussions](https://github.com/user/lunch_box/discussions)
2. Open a new discussion in "Ideas" category
3. Describe use case and proposed solution

### Pull Requests

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Run tests (`make test`)
5. Update documentation
6. Commit changes (`git commit -m 'Add amazing feature'`)
7. Push to branch (`git push origin feature/amazing-feature`)
8. Open pull request

## Development Setup

### Prerequisites
- C++17 compiler
- CMake 3.22+
- JUCE 8.0.12+
- Catch2 (for tests)

### Building
```bash
mkdir build && cd build
cmake ..
make
```

### Running Tests
```bash
make test
```

### Code Style
- Follow existing code style
- Run `clang-format` before committing
- Add comments for complex logic
- Write tests for new features

## Project Structure

```
lunch_box/
├── Source/          # C++ source code
│   ├── CLI/         # Command-line interface
│   ├── GUI/         # Graphical interface
│   └── ...          # Core modules
├── test/            # Unit tests
├── docs/            # Documentation
└── prompts/         # Milestone plans
```

## Questions?

Open a [discussion](https://github.com/user/lunch_box/discussions) or reach out!
```

### 3. CODE_OF_CONDUCT.md

Use [Contributor Covenant](https://www.contributor-covenant.org/) (industry standard):

```markdown
# Contributor Covenant Code of Conduct

## Our Pledge

We as members, contributors, and leaders pledge to make participation in our
community a harassment-free experience for everyone...

[Full text from contributor-covenant.org]
```

### 4. CHANGELOG.md

```markdown
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-02-02

### Added
- GUI application with folder selection
- Output folder selection
- Drag and drop support
- Status display with progress feedback

### Changed
- Improved error messages
- Enhanced logging format

### Fixed
- Fixed race condition in file processing
- Fixed memory leak in AudioConverter

## [1.0.0] - 2026-01-31

### Added
- Initial release
- CHOMPI mode with bank assignment
- CLI interface
- Audio conversion (16-bit 48kHz WAV)
- Support for WAV, AIFF, MP3, FLAC inputs
```

## Repository Configuration

### .github/ISSUE_TEMPLATE/bug_report.md

```markdown
---
name: Bug Report
about: Report a problem with Lunch Box
title: '[BUG] '
labels: bug
assignees: ''
---

**Describe the bug**
A clear and concise description of what the bug is.

**To Reproduce**
Steps to reproduce the behavior:
1. Launch application
2. Select folder '...'
3. Click process
4. See error

**Expected behavior**
What you expected to happen.

**Screenshots**
If applicable, add screenshots.

**System Information:**
- OS: [e.g. macOS 14.2]
- Lunch Box Version: [e.g. 1.1.0]
- JUCE Version: [e.g. 8.0.12]

**Log File**
Attach log file from `logs/` directory.

**Additional context**
Any other context about the problem.
```

### .github/workflows/build.yml

```yaml
name: Build and Test

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-macos:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3
      - name: Setup JUCE
        run: |
          git clone https://github.com/juce-framework/JUCE.git
          cd JUCE && git checkout 8.0.12
      - name: Build
        run: |
          mkdir build && cd build
          cmake ..
          make
      - name: Test
        run: |
          cd build
          make test

  build-windows:
    runs-on: windows-latest
    steps:
      # Similar steps for Windows

  build-linux:
    runs-on: ubuntu-latest
    steps:
      # Similar steps for Linux
```

## Distribution

### macOS

**Homebrew Formula:**
```ruby
class LunchBox < Formula
  desc "Audio sample processor for CHOMPI sampler"
  homepage "https://github.com/user/lunch_box"
  url "https://github.com/user/lunch_box/archive/v1.1.0.tar.gz"
  sha256 "..."
  license "MIT"

  depends_on "cmake" => :build
  depends_on "juce"

  def install
    mkdir "build" do
      system "cmake", "..", *std_cmake_args
      system "make"
      system "make", "install"
    end
  end

  test do
    system "#{bin}/lunch_box", "--help"
  end
end
```

### Windows

**Chocolatey Package:**
- Create `.nuspec` file
- Submit to Chocolatey repository
- Provide installer executable

### Linux

**Snap:**
```yaml
name: chompi-pack
version: '1.1.0'
summary: Audio sample processor for CHOMPI sampler
description: |
  Professional tool for processing audio samples...

grade: stable
confinement: strict

apps:
  chompi-pack:
    command: bin/lunch_box
    plugs:
      - home
      - audio-playback
      - pulseaudio

parts:
  chompi-pack:
    plugin: cmake
    source: .
```

## Pre-Release Checklist

- [ ] License selected and added
- [ ] License headers in all source files
- [ ] README.md comprehensive
- [ ] CONTRIBUTING.md complete
- [ ] CODE_OF_CONDUCT.md added
- [ ] CHANGELOG.md up to date
- [ ] Issue templates created
- [ ] PR templates created
- [ ] CI/CD pipeline working
- [ ] All tests passing
- [ ] Documentation complete
- [ ] Release builds tested (all platforms)
- [ ] Installation packages created
- [ ] Version tagged in git
- [ ] GitHub release created
- [ ] Social media announcement prepared
- [ ] Project website live (optional)

## Launch Strategy

### Phase 1: Soft Launch
- Release to close friends/testers
- Gather initial feedback
- Fix critical bugs

### Phase 2: Public Announcement
- Post on relevant forums (CHOMPI users, audio production)
- Share on social media
- Submit to product directories

### Phase 3: Community Building
- Respond to issues promptly
- Engage with contributors
- Build documentation based on questions
- Regular updates and improvements

## Success Metrics

- GitHub stars
- Download counts
- Issue response time
- Pull request merge rate
- Community engagement
- User testimonials

## Estimated Impact

- Time: 2-3 weeks (complete documentation and setup)
- Reach: Potentially thousands of CHOMPI users
- Community: Active open-source project
- Legacy: Tool continues beyond original development
