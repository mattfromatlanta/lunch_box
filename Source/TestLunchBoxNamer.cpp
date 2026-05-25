// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Processing/LunchBoxNamer.h"
#include "Logger.h"
#include <iostream>

// Simple test program for LunchBoxNamer logic
int main()
{
    Logger logger;
    LunchBoxNamer namer(logger);

    logger.logLine("=== LunchBoxNamer Unit Tests ===");
    logger.logLine("");

    // Test 1: indexToBankSlot conversions
    logger.logLine("Test 1: Index to Bank/Slot Conversion");
    logger.logLine("--------------------------------------");

    struct TestCase
    {
        int index;
        char expectedBank;
        int expectedSlot;
        juce::String expectedName;
    };

    TestCase testCases[] = {
        {0, 'a', 1, "cubbi_a1.wav"},      // First file
        {13, 'a', 14, "cubbi_a14.wav"},   // Last slot of bank A
        {14, 'b', 1, "cubbi_b1.wav"},     // First slot of bank B
        {27, 'b', 14, "cubbi_b14.wav"},   // Last slot of bank B
        {28, 'c', 1, "cubbi_c1.wav"},     // First slot of bank C
        {42, 'd', 1, "cubbi_d1.wav"},     // First slot of bank D
        {56, 'e', 1, "cubbi_e1.wav"},     // First slot of bank E
        {69, 'e', 14, "cubbi_e14.wav"},   // Last file (70th)
    };

    bool allTestsPassed = true;

    for (const auto& test : testCases)
    {
        LunchBoxNamer::BankSlot result = LunchBoxNamer::indexToBankSlot(test.index);
        juce::String generatedName = namer.generateFileName(LunchBoxNamer::Category::Cubbi, test.index);

        bool bankMatch = (result.bank == test.expectedBank);
        bool slotMatch = (result.slot == test.expectedSlot);
        bool nameMatch = (generatedName == test.expectedName);

        bool testPassed = bankMatch && slotMatch && nameMatch;

        logger.logLine("Index " + juce::String(test.index) + ": " +
                       (testPassed ? "PASS" : "FAIL"));
        logger.logLine("  Expected: bank=" + juce::String::charToString(test.expectedBank) +
                       ", slot=" + juce::String(test.expectedSlot) +
                       ", name=" + test.expectedName);
        logger.logLine("  Got:      bank=" + juce::String::charToString(result.bank) +
                       ", slot=" + juce::String(result.slot) +
                       ", name=" + generatedName);

        if (!testPassed)
            allTestsPassed = false;
    }

    logger.logLine("");

    // Test 2: Category names
    logger.logLine("Test 2: Category Names");
    logger.logLine("----------------------");

    juce::String cubbiName = LunchBoxNamer::categoryToString(LunchBoxNamer::Category::Cubbi);
    juce::String jammiName = LunchBoxNamer::categoryToString(LunchBoxNamer::Category::Jammi);

    logger.logLine("Cubbi: " + cubbiName + " (expected: Cubbi) - " +
                   juce::String(cubbiName == "Cubbi" ? "PASS" : "FAIL"));
    logger.logLine("Jammi: " + jammiName + " (expected: Jammi) - " +
                   juce::String(jammiName == "Jammi" ? "PASS" : "FAIL"));

    allTestsPassed = allTestsPassed && (cubbiName == "Cubbi") && (jammiName == "Jammi");

    logger.logLine("");

    // Test 3: Jammi filename generation
    logger.logLine("Test 3: Jammi Filename Generation");
    logger.logLine("----------------------------------");

    juce::String jammiFile1 = namer.generateFileName(LunchBoxNamer::Category::Jammi, 0);
    juce::String jammiFile70 = namer.generateFileName(LunchBoxNamer::Category::Jammi, 69);

    logger.logLine("Index 0:  " + jammiFile1 + " (expected: jammi_a1.wav) - " +
                   juce::String(jammiFile1 == "jammi_a1.wav" ? "PASS" : "FAIL"));
    logger.logLine("Index 69: " + jammiFile70 + " (expected: jammi_e14.wav) - " +
                   juce::String(jammiFile70 == "jammi_e14.wav" ? "PASS" : "FAIL"));

    allTestsPassed = allTestsPassed && (jammiFile1 == "jammi_a1.wav") &&
                    (jammiFile70 == "jammi_e14.wav");

    logger.logLine("");

    // Final results
    logger.logLine("=================================");
    if (allTestsPassed)
    {
        logger.logLine("ALL TESTS PASSED ✓");
    }
    else
    {
        logger.logLine("SOME TESTS FAILED ✗");
    }
    logger.logLine("=================================");

    return allTestsPassed ? 0 : 1;
}
