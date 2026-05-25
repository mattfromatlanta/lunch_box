// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>

// Force static registration of all test suites by including them here.
// Each file defines a static instance of its UnitTest subclass.
#include "ChompiNamerTests.cpp"
#include "BankFolderParserTests.cpp"
#include "FileSystemHelperTests.cpp"
#include "DragModelTests.cpp"

int main ()
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);

    // Run all registered tests
    runner.runAllTests();

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* result = runner.getResult (i);
        failures += result->failures;

        juce::String status = result->failures == 0 ? "PASS" : "FAIL";
        std::cout << "[" << status << "] " << result->unitTestName << " / " << result->subcategoryName
                  << " (" << result->passes << " passed, " << result->failures << " failed)\n";
    }

    std::cout << "\n" << (failures == 0 ? "All tests passed." : "FAILURES: " + std::to_string (failures)) << "\n";
    return failures == 0 ? 0 : 1;
}
