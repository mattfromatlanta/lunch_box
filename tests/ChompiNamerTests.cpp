// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_core/juce_core.h>
#include "../Source/Processing/LunchBoxNamer.h"
#include "../Source/Logger.h"

class ChompiNamerTests : public juce::UnitTest
{
public:
    ChompiNamerTests() : juce::UnitTest ("ChompiNamer") {}

    void runTest() override
    {
        beginTest ("indexToBankSlot: boundary values");
        {
            auto first = LunchBoxNamer::indexToBankSlot (0);
            expectEquals (first.bank, 'a');
            expectEquals (first.slot, 1);

            auto lastA = LunchBoxNamer::indexToBankSlot (13);
            expectEquals (lastA.bank, 'a');
            expectEquals (lastA.slot, 14);

            auto firstB = LunchBoxNamer::indexToBankSlot (14);
            expectEquals (firstB.bank, 'b');
            expectEquals (firstB.slot, 1);

            auto firstC = LunchBoxNamer::indexToBankSlot (28);
            expectEquals (firstC.bank, 'c');
            expectEquals (firstC.slot, 1);

            auto firstD = LunchBoxNamer::indexToBankSlot (42);
            expectEquals (firstD.bank, 'd');
            expectEquals (firstD.slot, 1);

            auto firstE = LunchBoxNamer::indexToBankSlot (56);
            expectEquals (firstE.bank, 'e');
            expectEquals (firstE.slot, 1);

            auto last = LunchBoxNamer::indexToBankSlot (69);
            expectEquals (last.bank, 'e');
            expectEquals (last.slot, 14);
        }

        beginTest ("indexToBankSlot: mid-bank values");
        {
            auto mid = LunchBoxNamer::indexToBankSlot (27);
            expectEquals (mid.bank, 'b');
            expectEquals (mid.slot, 14);
        }

        beginTest ("bankSlotToString");
        {
            expectEquals (LunchBoxNamer::bankSlotToString ({ 'a', 1 }),  juce::String ("a1"));
            expectEquals (LunchBoxNamer::bankSlotToString ({ 'a', 14 }), juce::String ("a14"));
            expectEquals (LunchBoxNamer::bankSlotToString ({ 'e', 7 }),  juce::String ("e7"));
        }

        beginTest ("categoryToString");
        {
            expectEquals (LunchBoxNamer::categoryToString (LunchBoxNamer::Category::Cubbi), juce::String ("Cubbi"));
            expectEquals (LunchBoxNamer::categoryToString (LunchBoxNamer::Category::Jammi), juce::String ("Jammi"));
        }

        beginTest ("generateFileName: cubbi");
        {
            Logger logger;
            LunchBoxNamer namer (logger);

            expectEquals (namer.generateFileName (LunchBoxNamer::Category::Cubbi, 0),  juce::String ("cubbi_a1.wav"));
            expectEquals (namer.generateFileName (LunchBoxNamer::Category::Cubbi, 13), juce::String ("cubbi_a14.wav"));
            expectEquals (namer.generateFileName (LunchBoxNamer::Category::Cubbi, 14), juce::String ("cubbi_b1.wav"));
            expectEquals (namer.generateFileName (LunchBoxNamer::Category::Cubbi, 69), juce::String ("cubbi_e14.wav"));
        }

        beginTest ("generateFileName: jammi");
        {
            Logger logger;
            LunchBoxNamer namer (logger);

            expectEquals (namer.generateFileName (LunchBoxNamer::Category::Jammi, 0),  juce::String ("jammi_a1.wav"));
            expectEquals (namer.generateFileName (LunchBoxNamer::Category::Jammi, 56), juce::String ("jammi_e1.wav"));
            expectEquals (namer.generateFileName (LunchBoxNamer::Category::Jammi, 69), juce::String ("jammi_e14.wav"));
        }

        beginTest ("constants");
        {
            expectEquals (LunchBoxNamer::SLOTS_PER_BANK,         14);
            expectEquals (LunchBoxNamer::NUM_BANKS,               5);
            expectEquals (LunchBoxNamer::MAX_FILES_PER_CATEGORY, 70);
        }
    }
};

static ChompiNamerTests chompiNamerTests;
