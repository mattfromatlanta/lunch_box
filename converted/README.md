# Converted Audio Files

This directory contains audio files that have been converted by Chompi Pack.

## Conversion Specifications

All files in this directory have been converted to:
- **Format:** WAV (uncompressed)
- **Bit Depth:** 16-bit
- **Sample Rate:** 48000 Hz (48kHz)
- **Channels:** Preserved from original (mono or stereo only)

## File Organization

- Original filenames are preserved
- Files maintain their relative directory structure from the source
- Original files are never modified during conversion

## Notes

- Files in this directory are ignored by git (see .gitignore)
- Multi-channel files (>2 channels) are skipped during conversion
- All conversion operations are logged in the logs/ directory
- This directory is automatically created if it doesn't exist
