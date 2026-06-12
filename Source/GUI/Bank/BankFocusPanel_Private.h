// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Implementation-private helpers shared across BankFocusPanel translation
// units. Not part of the public API of the panel.

#pragma once

#include "BankFocusPanel.h"
#include "UIColours.h"
#include "UIConstants.h"
#include "LunchBoxFonts.h"

namespace BankFocusImpl
{
    inline const juce::Colour panelBg = LunchBoxColours::DARK_GREY;

    inline juce::Colour bankColourForIndex(int idx)
    {
        switch (idx)
        {
            case 0: return LunchBoxColours::RED;
            case 1: return LunchBoxColours::PINK_SALMON;
            case 2: return LunchBoxColours::YELLOW;
            case 3: return LunchBoxColours::TEAL;
            case 4: return LunchBoxColours::PURPLE;
            default: return LunchBoxColours::WHITE_CREAM;
        }
    }

    // LookAndFeel for bank selector buttons
    struct SlotStyleLAF : public juce::LookAndFeel_V4
    {
        void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                                  const juce::Colour& backgroundColour,
                                  bool isHighlighted, bool) override
        {
            auto bounds = btn.getLocalBounds().reduced(2).toFloat();
            const juce::Colour fill = btn.getToggleState()
                ? LunchBoxColours::getFocused(backgroundColour)
                : backgroundColour;
            g.setColour(isHighlighted ? fill.brighter(0.1f) : fill);
            g.fillRoundedRectangle(bounds, LunchBoxConstants::CORNER_RADIUS);
            g.setColour(LunchBoxColours::WHITE_CREAM.withAlpha(0.3f));
            g.drawRoundedRectangle(bounds, LunchBoxConstants::CORNER_RADIUS, LunchBoxConstants::BORDER_WIDTH_ACTIVE);
        }

        void drawButtonText(juce::Graphics& g, juce::TextButton& btn, bool, bool) override
        {
            const bool active = btn.getToggleState();
            const auto font = LunchBoxFonts::h3();

            if (active)
            {
                const float circleR = 14.0f;
                juce::GlyphArrangement glyphs;
                glyphs.addFittedText(font, btn.getButtonText(),
                    0.0f, 0.0f, (float) btn.getWidth(), (float) btn.getHeight(),
                    juce::Justification::centred, 1);
                auto charBounds = glyphs.getBoundingBox(0, -1, true);
                float charCentreX = charBounds.getCentreX() - 0.5f;
                float charCentreY = charBounds.getCentreY();
                g.setColour(LunchBoxColours::getLabelBg(btn.findColour(juce::TextButton::buttonOnColourId)));
                g.fillEllipse(charCentreX - circleR, charCentreY - circleR, circleR * 2.0f, circleR * 2.0f);
                g.setColour(LunchBoxColours::WHITE_CREAM.withAlpha(0.5f));
                g.drawEllipse(charCentreX - circleR, charCentreY - circleR, circleR * 2.0f, circleR * 2.0f, 1.5f);
            }

            g.setColour(btn.findColour(active ? juce::TextButton::textColourOnId
                                              : juce::TextButton::textColourOffId));
            g.setFont(font);
            g.drawText(btn.getButtonText(), btn.getLocalBounds(), juce::Justification::centred);
        }
    };

}
