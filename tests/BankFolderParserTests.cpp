// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_core/juce_core.h>
#include "../Source/Processing/BankFolderParser.h"

class BankFolderParserTests : public juce::UnitTest
{
public:
    BankFolderParserTests() : juce::UnitTest ("BankFolderParser") {}

    void runTest() override
    {
        beginTest ("isBankFolder: single uppercase letters A-E");
        {
            char bank = '\0';
            expect (BankFolderParser::isBankFolder ("A", bank));
            expectEquals (bank, 'a');

            expect (BankFolderParser::isBankFolder ("B", bank));
            expectEquals (bank, 'b');

            expect (BankFolderParser::isBankFolder ("E", bank));
            expectEquals (bank, 'e');
        }

        beginTest ("isBankFolder: single lowercase letters a-e");
        {
            char bank = '\0';
            expect (BankFolderParser::isBankFolder ("a", bank));
            expectEquals (bank, 'a');

            expect (BankFolderParser::isBankFolder ("e", bank));
            expectEquals (bank, 'e');
        }

        beginTest ("isBankFolder: bank_ prefix variants");
        {
            char bank = '\0';
            expect (BankFolderParser::isBankFolder ("bank_a", bank));
            expectEquals (bank, 'a');

            expect (BankFolderParser::isBankFolder ("bank_E", bank));
            expectEquals (bank, 'e');

            expect (BankFolderParser::isBankFolder ("Bank A", bank));
            expectEquals (bank, 'a');

            expect (BankFolderParser::isBankFolder ("Bank C", bank));
            expectEquals (bank, 'c');
        }

        beginTest ("isBankFolder: invalid names rejected");
        {
            char bank = '\0';
            expect (! BankFolderParser::isBankFolder ("F", bank));
            expect (! BankFolderParser::isBankFolder ("misc", bank));
            expect (! BankFolderParser::isBankFolder ("AB", bank));
            expect (! BankFolderParser::isBankFolder ("", bank));
            expect (! BankFolderParser::isBankFolder ("1", bank));
            expect (! BankFolderParser::isBankFolder ("bank_f", bank));
        }
    }
};

static BankFolderParserTests bankFolderParserTests;
