// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_graphics/juce_graphics.h>

namespace ChompiColours
{
    struct ColourFactor { float V; float S; };

    constexpr ColourFactor FOCUS_FACTOR  { .V = 1.20f, .S = 0.90f };
    constexpr ColourFactor BORDER_FACTOR { .V = 0.70f, .S = 1.25f };
    constexpr ColourFactor TAB_BG_FACTOR   { .V = 0.50f, .S = 1.30f };
    constexpr ColourFactor LABEL_BG_FACTOR { .V = 0.50f, .S = 1.30f };

    inline juce::Colour getFocused(juce::Colour c)
    {
        return c.withBrightness (juce::jmin(1.0f, c.getBrightness()  * FOCUS_FACTOR.V))
                .withSaturation (juce::jmin(1.0f, c.getSaturation()  * FOCUS_FACTOR.S));
    }

    inline juce::Colour getBorder(juce::Colour c)
    {
        return c.withBrightness (juce::jmin(1.0f, c.getBrightness() * BORDER_FACTOR.V))
                .withSaturation (juce::jmin(1.0f, c.getSaturation() * BORDER_FACTOR.S));
    }

    inline juce::Colour getTabBg(juce::Colour c)
    {
        return c.withBrightness (juce::jmin(1.0f, c.getBrightness() * TAB_BG_FACTOR.V))
                .withSaturation (juce::jmin(1.0f, c.getSaturation() * TAB_BG_FACTOR.S));
    }

    inline juce::Colour getLabelBg(juce::Colour c)
    {
        return c.withBrightness (juce::jmin(1.0f, c.getBrightness() * LABEL_BG_FACTOR.V))
                .withSaturation (juce::jmin(1.0f, c.getSaturation() * LABEL_BG_FACTOR.S));
    }

    //                                              H     S     V
    inline const juce::Colour DARK_GREY   = juce::Colour::fromHSV(0.67f, 0.15f, 0.21f, 1.0f);
    inline const juce::Colour BUTTON_BG   = juce::Colour::fromHSV(0.73f, 0.32f, 0.13f, 1.0f);
    inline const juce::Colour PURPLE      = juce::Colour::fromHSV(0.75f, 0.45f, 0.78f, 1.0f);
    inline const juce::Colour RED         = juce::Colour::fromHSV(0.02f, 0.71f, 0.79f, 1.0f);
    inline const juce::Colour PINK_SALMON = juce::Colour::fromHSV(0.05f, 0.45f, 0.91f, 1.0f);
    inline const juce::Colour YELLOW      = juce::Colour::fromHSV(0.12f, 0.68f, 0.91f, 1.0f);
    inline const juce::Colour TEAL        = juce::Colour::fromHSV(0.49f, 0.57f, 0.68f, 1.0f);
    inline const juce::Colour WHITE_CREAM = juce::Colour::fromHSV(0.10f, 0.05f, 0.96f, 1.0f);
    inline const juce::Colour GRID        = juce::Colour::fromHSV(0.67f, 0.16f, 0.27f, 1.0f);
}
