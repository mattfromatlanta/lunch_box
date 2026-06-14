// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_core/juce_core.h>
#include "../Source/FileSystemHelper.h"
#include "../Source/Logger.h"

class FileSystemHelperTests : public juce::UnitTest
{
public:
    FileSystemHelperTests() : juce::UnitTest ("FileSystemHelper") {}

    void runTest() override
    {
        beginTest ("getSupportedAudioExtensions: required formats present");
        {
            auto exts = FileSystemHelper::getSupportedAudioExtensions();
            expect (exts.contains ("*.wav")  || exts.contains ("*.WAV"),  "wav missing");
            expect (exts.contains ("*.aif")  || exts.contains ("*.aiff") || exts.contains ("*.AIF"), "aif missing");
            expect (exts.contains ("*.mp3")  || exts.contains ("*.MP3"),  "mp3 missing");
            expect (exts.contains ("*.flac") || exts.contains ("*.FLAC"), "flac missing");
            expect (exts.size() >= 4);
        }

        beginTest ("getAudioFormatName: known extensions");
        {
            juce::File wav  (juce::File::getCurrentWorkingDirectory().getChildFile ("test.wav"));
            juce::File mp3  (juce::File::getCurrentWorkingDirectory().getChildFile ("test.mp3"));
            juce::File flac (juce::File::getCurrentWorkingDirectory().getChildFile ("test.flac"));
            juce::File aiff (juce::File::getCurrentWorkingDirectory().getChildFile ("test.aiff"));

            expectEquals (FileSystemHelper::getAudioFormatName (wav).toUpperCase(),  juce::String ("WAV"));
            expectEquals (FileSystemHelper::getAudioFormatName (mp3).toUpperCase(),  juce::String ("MP3"));
            expectEquals (FileSystemHelper::getAudioFormatName (flac).toUpperCase(), juce::String ("FLAC"));
            expectEquals (FileSystemHelper::getAudioFormatName (aiff).toUpperCase(), juce::String ("AIFF"));
        }

        beginTest ("ensureDirectoryExists: creates missing directory");
        {
            auto tmpDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getChildFile ("chompi_test_" + juce::String (juce::Random::getSystemRandom().nextInt (100000)));

            expect (! tmpDir.exists());

            Logger logger;
            bool ok = FileSystemHelper::ensureDirectoryExists (tmpDir, logger);
            expect (ok);
            expect (tmpDir.isDirectory());

            tmpDir.deleteRecursively();
        }

        beginTest ("ensureDirectoryExists: existing directory returns true");
        {
            auto tmpDir = juce::File::getSpecialLocation (juce::File::tempDirectory);
            Logger logger;
            expect (FileSystemHelper::ensureDirectoryExists (tmpDir, logger));
        }

        beginTest ("isDirectoryWritable: temp dir is writable");
        {
            expect (FileSystemHelper::isDirectoryWritable (
                juce::File::getSpecialLocation (juce::File::tempDirectory)));
        }
    }
};

static FileSystemHelperTests fileSystemHelperTests;
