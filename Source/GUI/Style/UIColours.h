// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_graphics/juce_graphics.h>

namespace LunchBoxColours
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
    inline const juce::Colour FOCUS_BORDER = WHITE_CREAM;   // focused cell/row indicator

    // ── Console / status colours ───────────────────────────
    inline const juce::Colour CONSOLE_BG      { 0xff151a26 };
    inline const juce::Colour CONSOLE_TEXT    { 0xffaabbcc };
    inline const juce::Colour CONSOLE_OUTLINE { 0xff2a3a4a };
    inline const juce::Colour ACCENT_GREEN    { 0xff4caf50 };   // success accent

    // ── Bank slot state colours ────────────────────────────
    inline const juce::Colour SLOT_EMPTY_BG       { 0xff1d2228 };
    inline const juce::Colour SLOT_FILLED_BG      { 0xff3a5060 };
    inline const juce::Colour SLOT_BORDER         { 0xff3a4a5a };
    inline const juce::Colour SLOT_DROP_BDR       { 0xff4caf50 };
    inline const juce::Colour SLOT_SELECTED_BG    { 0xff1e2a4a };
    inline const juce::Colour SLOT_DRAG_TARGET_BG { 0xff2d2518 };
    inline const juce::Colour SLOT_DRAG_TARGET_BDR{ 0xffddaa33 };
    inline const juce::Colour SLOT_SWAP_SRC_BG    { 0xff2a1e08 };
    inline const juce::Colour SLOT_SWAP_SRC_BDR   { 0xff997733 };

    // ── WaveformDisplay colours ────────────────────────────
    inline const juce::Colour WAVE_BG     { 0xff151a26 };
    inline const juce::Colour WAVE_FG     { 0xff4caf50 };
    inline const juce::Colour WAVE_CURSOR { 0xccffffff };
    inline const juce::Colour WAVE_EMPTY  { 0xff3a4a5a };

    // ── FolderDropZone colours ─────────────────────────────
    inline const juce::Colour DROPZONE_BG       { 0xff252b3b };
    inline const juce::Colour DROPZONE_BORDER   { 0xff3a4a5a };
    inline const juce::Colour DROPZONE_HOVER_BG     { 0x1a2196f3 };
    inline const juce::Colour DROPZONE_HOVER_BORDER { 0xff2196f3 };
    inline const juce::Colour DROPZONE_PATH         { 0xffccddee };
    inline const juce::Colour DROPZONE_PLACEHOLDER  { 0xff667788 };

    // Draw 45° diagonal (top-left → bottom-right) selection stripes for selected-but-not-focused
    // elements. originInRoot is the component's (0,0) in root-window coordinates, ensuring stripe
    // continuity across cells — they appear as a shared virtual layer being unmasked per cell.
    // phase is reserved for future animation (scroll the stripes by incrementing and repainting).
    inline void drawSelectionStripes(juce::Graphics& g,
                                     juce::Rectangle<float> bounds,
                                     juce::Point<int> originInRoot,
                                     float cornerRadius,
                                     float phase = 0.0f)
    {
        constexpr float period = 24.0f;
        constexpr float sw     = 12.0f;

        juce::Path clip;
        clip.addRoundedRectangle(bounds, cornerRadius);
        g.saveState();
        g.reduceClipRegion(clip);
        g.setColour(WHITE_CREAM.withAlpha(0.5f));

        const float C  = (float)originInRoot.x - (float)originInRoot.y + phase;
        const float bx = bounds.getX();
        const float by = bounds.getY();
        const float bw = bounds.getWidth();
        const float bh = bounds.getHeight();

        const int kMin = (int)std::floor((bx - by - bh - sw + C) / period) - 1;
        const int kMax = (int)std::ceil ((bx - by + bw      + C) / period) + 1;

        juce::Path stripes;
        for (int k = kMin; k <= kMax; ++k)
        {
            const float d = (float)k * period - C;
            stripes.startNewSubPath (d + by,           by);
            stripes.lineTo          (d + sw + by,      by);
            stripes.lineTo          (d + sw + by + bh, by + bh);
            stripes.lineTo          (d + by + bh,      by + bh);
            stripes.closeSubPath();
        }
        g.fillPath(stripes);
        g.restoreState();
    }
}
