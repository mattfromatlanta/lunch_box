// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

namespace LunchBoxConstants
{
    constexpr float CORNER_RADIUS       = 6.0f;
    constexpr float BORDER_WIDTH        = 2.0f;
    constexpr float BORDER_WIDTH_ACTIVE = 4.0f;

    // Fixed gaps — do not scale with window size
    constexpr float BANK_GAP     = 8.0f;   // between banks in Pack and Bank view
    constexpr float SLOT_ROW_GAP = 4.0f;   // between slot rows in Bank view

    constexpr int ANIM_DURATION_MS      = 350;
    constexpr int ANIM_CLEANUP_DELAY_MS = ANIM_DURATION_MS + 10;

    // ── MainComponent layout ───────────────────────────────
    constexpr int HEADER_HEIGHT       = 64;   // "LUNCH BOX" title band at top
    constexpr int NAV_ROW_HEIGHT      = 32;   // Cubbi/Jammi + Pack/Bank tab row
    constexpr int NAV_PAIR_GAP        = 24;   // gap between left-pair and right-pair in nav row
    constexpr int NAV_TO_CONTENT_GAP  = 8;    // gap below nav row before content
    constexpr int CONTENT_H_MARGIN    = 12;   // left/right margin around content
    constexpr int CONTENT_BOTTOM_PAD  = 12;   // bottom margin under content
    constexpr int FOOTER_HEIGHT       = 54;   // footer button row height
    constexpr int FOOTER_TOP_GAP      = 12;   // gap above footer
    constexpr int FOOTER_BAND_HEIGHT  = FOOTER_HEIGHT + FOOTER_TOP_GAP;
    constexpr int BUTTON_H_INSET      = 2;    // horizontal reduction for buttons in a row

    // ── ConsoleWindow ──────────────────────────────────────
    constexpr int CONSOLE_WIDTH       = 520;
    constexpr int CONSOLE_HEIGHT      = 280;
    constexpr float CONSOLE_FONT_SIZE = 12.0f;

    // ── WaveformDisplay ────────────────────────────────────
    constexpr int WAVEFORM_REPAINT_HZ = 30;
}
