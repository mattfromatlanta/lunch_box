// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LunchBoxFonts.h"
#include "UIColours.h"
#include "UIConstants.h"

//==============================================================================
// FooterButtonLAF — shared LookAndFeel for footer action buttons
//==============================================================================
struct FooterButtonLAF : public juce::LookAndFeel_V4
{
    juce::Font getTextButtonFont(juce::TextButton&, int) override { return LunchBoxFonts::h3(); }

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& backgroundColour,
                              bool isHighlighted, bool) override
    {
        auto bounds = btn.getLocalBounds().reduced(1).toFloat();
        g.setColour(isHighlighted ? backgroundColour.brighter(0.1f) : backgroundColour);
        g.fillRoundedRectangle(bounds, LunchBoxConstants::CORNER_RADIUS);
        g.setColour(LunchBoxColours::WHITE_CREAM.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds, LunchBoxConstants::CORNER_RADIUS, LunchBoxConstants::BORDER_WIDTH_ACTIVE);
    }
};
