// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_core/juce_core.h>
#include "../Source/Processing/ChompiNamer.h"
#include "../Source/Logger.h"

class ChompiNamerTests : public juce::UnitTest
{
public:
    ChompiNamerTests() : juce::UnitTest ("ChompiNamer") {}

    void runTest() override
    {
        beginTest ("indexToBankSlot: boundary values");
        {
            auto first = ChompiNamer::indexToBankSlot (0);
            expectEquals (first.bank, 'a');
            expectEquals (first.slot, 1);

            auto lastA = ChompiNamer::indexToBankSlot (13);
            expectEquals (lastA.bank, 'a');
            expectEquals (lastA.slot, 14);

            auto firstB = ChompiNamer::indexToBankSlot (14);
            expectEquals (firstB.bank, 'b');
            expectEquals (firstB.slot, 1);

            auto firstC = ChompiNamer::indexToBankSlot (28);
            expectEquals (firstC.bank, 'c');
            expectEquals (firstC.slot, 1);

            auto firstD = ChompiNamer::indexToBankSlot (42);
            expectEquals (firstD.bank, 'd');
            expectEquals (firstD.slot, 1);

            auto firstE = ChompiNamer::indexToBankSlot (56);
            expectEquals (firstE.bank, 'e');
            expectEquals (firstE.slot, 1);

            auto last = ChompiNamer::indexToBankSlot (69);
            expectEquals (last.bank, 'e');
            expectEquals (last.slot, 14);
        }

        beginTest ("indexToBankSlot: mid-bank values");
        {
            auto mid = ChompiNamer::indexToBankSlot (27);
            expectEquals (mid.bank, 'b');
            expectEquals (mid.slot, 14);
        }

        beginTest ("bankSlotToString");
        {
            expectEquals (ChompiNamer::bankSlotToString ({ 'a', 1 }),  juce::String ("a1"));
            expectEquals (ChompiNamer::bankSlotToString ({ 'a', 14 }), juce::String ("a14"));
            expectEquals (ChompiNamer::bankSlotToString ({ 'e', 7 }),  juce::String ("e7"));
        }

        beginTest ("categoryToString");
        {
            expectEquals (ChompiNamer::categoryToString (ChompiNamer::Category::Cubbi), juce::String ("Cubbi"));
            expectEquals (ChompiNamer::categoryToString (ChompiNamer::Category::Jammi), juce::String ("Jammi"));
        }

        beginTest ("generateFileName: cubbi");
        {
            Logger logger;
            ChompiNamer namer (logger);

            expectEquals (namer.generateFileName (ChompiNamer::Category::Cubbi, 0),  juce::String ("cubbi_a1.wav"));
            expectEquals (namer.generateFileName (ChompiNamer::Category::Cubbi, 13), juce::String ("cubbi_a14.wav"));
            expectEquals (namer.generateFileName (ChompiNamer::Category::Cubbi, 14), juce::String ("cubbi_b1.wav"));
            expectEquals (namer.generateFileName (ChompiNamer::Category::Cubbi, 69), juce::String ("cubbi_e14.wav"));
        }

        beginTest ("generateFileName: jammi");
        {
            Logger logger;
            ChompiNamer namer (logger);

            expectEquals (namer.generateFileName (ChompiNamer::Category::Jammi, 0),  juce::String ("jammi_a1.wav"));
            expectEquals (namer.generateFileName (ChompiNamer::Category::Jammi, 56), juce::String ("jammi_e1.wav"));
            expectEquals (namer.generateFileName (ChompiNamer::Category::Jammi, 69), juce::String ("jammi_e14.wav"));
        }

        beginTest ("constants");
        {
            expectEquals (ChompiNamer::SLOTS_PER_BANK,         14);
            expectEquals (ChompiNamer::NUM_BANKS,               5);
            expectEquals (ChompiNamer::MAX_FILES_PER_CATEGORY, 70);
        }
    }
};

static ChompiNamerTests chompiNamerTests;
