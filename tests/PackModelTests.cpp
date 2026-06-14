// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_core/juce_core.h>
#include "../Source/GUI/Common/PackModel.h"

using Cat = LunchBoxNamer::Category;

namespace
{
    juce::File fakeFile(const juce::String& name)
    {
        return juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(name);
    }
}

class PackModelTests : public juce::UnitTest
{
public:
    PackModelTests() : juce::UnitTest("PackModel", "lunch_box") {}

    void runTest() override
    {
        beginTest("empty model has no assignments or filled slots");
        {
            PackModel model;
            expectEquals(model.getFilledCount(Cat::Cubbi), 0);
            expectEquals(model.getFilledCount(Cat::Jammi), 0);
            expect(model.getAssignments(Cat::Cubbi).isEmpty());
            expect(model.getSlot(Cat::Cubbi, 0, 0) == juce::File{});
        }

        beginTest("setSlot stores per category, bank and slot independently");
        {
            PackModel model;
            auto a = fakeFile("a.wav");
            auto b = fakeFile("b.wav");

            expect(model.setSlot(Cat::Cubbi, 0, 0, a));
            expect(model.setSlot(Cat::Jammi, 0, 0, b));

            expect(model.getSlot(Cat::Cubbi, 0, 0) == a);
            expect(model.getSlot(Cat::Jammi, 0, 0) == b);
            // Same coordinate in the other category is untouched.
            expect(model.getSlot(Cat::Cubbi, 0, 1) == juce::File{});
        }

        beginTest("setSlot returns true only when the value changes");
        {
            PackModel model;
            auto a = fakeFile("a.wav");
            expect(model.setSlot(Cat::Cubbi, 1, 2, a));        // new value
            expect(! model.setSlot(Cat::Cubbi, 1, 2, a));      // unchanged
            expect(model.setSlot(Cat::Cubbi, 1, 2, juce::File{})); // cleared
        }

        beginTest("out-of-range coordinates are ignored");
        {
            PackModel model;
            auto a = fakeFile("a.wav");
            expect(! model.setSlot(Cat::Cubbi, -1, 0, a));
            expect(! model.setSlot(Cat::Cubbi, LunchBoxNamer::NUM_BANKS, 0, a));
            expect(! model.setSlot(Cat::Cubbi, 0, LunchBoxNamer::SLOTS_PER_BANK, a));
            expectEquals(model.getFilledCount(Cat::Cubbi), 0);
            expect(model.getSlot(Cat::Cubbi, 99, 99) == juce::File{});
        }

        beginTest("getAssignments maps banks to letters and slots to 1-based numbers");
        {
            PackModel model;
            auto a = fakeFile("a.wav");
            model.setSlot(Cat::Cubbi, 0, 0, a);   // bank A, slot 1
            model.setSlot(Cat::Cubbi, 4, 13, a);  // bank E, slot 14

            auto assignments = model.getAssignments(Cat::Cubbi);
            expectEquals(assignments.size(), 2);
            expectEquals((int) assignments[0].bankLetter, (int) 'a');
            expectEquals(assignments[0].slotNumber, 1);
            expectEquals((int) assignments[1].bankLetter, (int) 'e');
            expectEquals(assignments[1].slotNumber, 14);
        }

        beginTest("clearCategory empties one category but leaves the other");
        {
            PackModel model;
            auto a = fakeFile("a.wav");
            model.setSlot(Cat::Cubbi, 0, 0, a);
            model.setSlot(Cat::Jammi, 0, 0, a);

            model.clearCategory(Cat::Cubbi);
            expectEquals(model.getFilledCount(Cat::Cubbi), 0);
            expectEquals(model.getFilledCount(Cat::Jammi), 1);
        }

        beginTest("snapshot and restore round-trip the full state");
        {
            PackModel model;
            auto a = fakeFile("a.wav");
            auto b = fakeFile("b.wav");
            model.setSlot(Cat::Cubbi, 2, 5, a);
            model.setSlot(Cat::Jammi, 3, 7, b);

            auto snap = model.snapshot();

            // Mutate after snapshotting.
            model.clearAll();
            expectEquals(model.getFilledCount(Cat::Cubbi), 0);

            model.restore(snap);
            expect(model.getSlot(Cat::Cubbi, 2, 5) == a);
            expect(model.getSlot(Cat::Jammi, 3, 7) == b);
            expectEquals(model.getFilledCount(Cat::Cubbi), 1);
            expectEquals(model.getFilledCount(Cat::Jammi), 1);
        }
    }
};

static PackModelTests packModelTests;
