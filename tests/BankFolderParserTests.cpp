// SPDX-License-Identifier: AGPL-3.0-or-later
#include <juce_core/juce_core.h>
#include "../Source/Processing/BankFolderParser.h"

class BankFolderParserTests : public juce::UnitTest
{
public:
    BankFolderParserTests() : juce::UnitTest ("BankFolderParser") {}

    void runTest() override
    {
        beginTest ("parseBankFolder: single uppercase letters A-E");
        {
            expect (BankFolderParser::parseBankFolder ("A") == 'a');
            expect (BankFolderParser::parseBankFolder ("B") == 'b');
            expect (BankFolderParser::parseBankFolder ("E") == 'e');
        }

        beginTest ("parseBankFolder: single lowercase letters a-e");
        {
            expect (BankFolderParser::parseBankFolder ("a") == 'a');
            expect (BankFolderParser::parseBankFolder ("e") == 'e');
        }

        beginTest ("parseBankFolder: bank_ prefix variants");
        {
            expect (BankFolderParser::parseBankFolder ("bank_a") == 'a');
            expect (BankFolderParser::parseBankFolder ("bank_E") == 'e');
            expect (BankFolderParser::parseBankFolder ("Bank A") == 'a');
            expect (BankFolderParser::parseBankFolder ("Bank C") == 'c');
        }

        beginTest ("parseBankFolder: invalid names rejected");
        {
            expect (! BankFolderParser::parseBankFolder ("F").has_value());
            expect (! BankFolderParser::parseBankFolder ("misc").has_value());
            expect (! BankFolderParser::parseBankFolder ("AB").has_value());
            expect (! BankFolderParser::parseBankFolder ("").has_value());
            expect (! BankFolderParser::parseBankFolder ("1").has_value());
            expect (! BankFolderParser::parseBankFolder ("bank_f").has_value());
        }
    }
};

static BankFolderParserTests bankFolderParserTests;
