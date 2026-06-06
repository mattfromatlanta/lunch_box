// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_core/juce_core.h>

// One entry on the internal sample clipboard.  rowOffset / colOffset are the
// cell's position relative to the top-left (earliest) selected cell so that
// discontiguous selections keep their spatial shape on paste.
//   Pack mode: rowOffset = bank delta, colOffset = slot delta
//   Bank mode: rowOffset = row delta,  colOffset = 0 (always)
struct ClipboardEntry
{
    juce::File file;
    int rowOffset = 0;
    int colOffset = 0;
};
