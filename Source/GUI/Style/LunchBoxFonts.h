// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_graphics/juce_graphics.h>
#include <BinaryData.h>

namespace LunchBoxFonts
{
    // ── Typeface accessors (lazy, cached) ─────────────────────────────────────

    inline juce::Typeface::Ptr regular()
    {
        static auto tf = juce::Typeface::createSystemTypefaceFor(
            BinaryData::PoppinsRegular_ttf, BinaryData::PoppinsRegular_ttfSize);
        return tf;
    }

    inline juce::Typeface::Ptr medium()
    {
        static auto tf = juce::Typeface::createSystemTypefaceFor(
            BinaryData::PoppinsMedium_ttf, BinaryData::PoppinsMedium_ttfSize);
        return tf;
    }

    inline juce::Typeface::Ptr semiBold()
    {
        static auto tf = juce::Typeface::createSystemTypefaceFor(
            BinaryData::PoppinsSemiBold_ttf, BinaryData::PoppinsSemiBold_ttfSize);
        return tf;
    }

    inline juce::Typeface::Ptr bold()
    {
        static auto tf = juce::Typeface::createSystemTypefaceFor(
            BinaryData::PoppinsBold_ttf, BinaryData::PoppinsBold_ttfSize);
        return tf;
    }

    // ── Font style factory functions ──────────────────────────────────────────
    // Letter-spacing maps CSS em values directly to JUCE's kerning factor.
    // Line-height is noted in comments; apply at the layout/TextLayout level.

    // H3 — SemiBold 24px, line-height 1.3
    inline juce::Font h3()
    {
        return juce::Font(juce::FontOptions{}.withTypeface(semiBold()).withHeight(24.0f));
    }

    // Body — Regular 16px, line-height 1.6
    inline juce::Font body()
    {
        return juce::Font(juce::FontOptions{}.withTypeface(regular()).withHeight(16.0f));
    }

    // Nav links — Medium 14px, uppercase, tracking 0.05em
    inline juce::Font nav(float height = 14.0f)
    {
        return juce::Font(juce::FontOptions{}.withTypeface(medium()).withHeight(height))
               .withExtraKerningFactor(0.05f);
    }

    // Footer labels — Bold 12px, uppercase, tracking 0.08em
    inline juce::Font footer()
    {
        return juce::Font(juce::FontOptions{}.withTypeface(bold()).withHeight(12.0f))
               .withExtraKerningFactor(0.08f);
    }
}
