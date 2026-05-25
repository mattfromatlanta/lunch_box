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

    struct CellFile
    {
        BankEditorPanel::Cell c;
        juce::File f;
    };

    // Sort an array of CellFile by (row, col) ascending — used for stable swap pairing
    inline void sortCellsRowMajor(juce::Array<CellFile>& arr)
    {
        for (int i = 0; i < arr.size() - 1; ++i)
            for (int j = i + 1; j < arr.size(); ++j)
            {
                auto a = arr[i], b = arr[j];
                if (b.c.row < a.c.row || (b.c.row == a.c.row && b.c.col < a.c.col))
                    arr.swap(i, j);
            }
    }
}
