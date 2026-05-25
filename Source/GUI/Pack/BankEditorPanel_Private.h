// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Implementation-private helpers shared across BankEditorPanel translation
// units. Not part of the public API of the panel.

#pragma once

#include "BankEditorPanel.h"
#include "UIColours.h"

namespace BankEditorImpl
{
    inline const juce::Colour panelBg   = LunchBoxColours::DARK_GREY;
    inline const juce::Colour accentCol { 0xff4caf50 };
    inline const juce::Colour buttonCol { 0xff2a3a4a };
    inline const juce::Colour buttonTxt { 0xffaabbcc };

}
