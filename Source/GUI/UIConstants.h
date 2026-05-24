// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

namespace ChompiConstants
{
    constexpr float CORNER_RADIUS       = 6.0f;
    constexpr float BORDER_WIDTH        = 2.0f;
    constexpr float BORDER_WIDTH_ACTIVE = 4.0f;

    // Fixed gaps — do not scale with window size
    constexpr float BANK_GAP     = 8.0f;   // between banks in Pack and Bank view
    constexpr float SLOT_ROW_GAP = 4.0f;   // between slot rows in Bank view

    // Base content area at default 525×900 window
    // (525 - 2×12 margin = 501w; 900 - 2×12 - 44 header - 32 nav - 8 gap - 66 footer = 726h)
    constexpr float BASE_CONTENT_W = 501.0f;
    constexpr float BASE_CONTENT_H = 726.0f;
}
