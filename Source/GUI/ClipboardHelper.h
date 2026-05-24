// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_core/juce_core.h>

//==============================================================================
// ClipboardHelper - reads file paths from the system clipboard
//
// On macOS, copies from Finder / Sononym / etc. land on NSPasteboard as
// NSFilenamesPboardType (an array of POSIX path strings). JUCE's own
// SystemClipboard only exposes text, so we go to NSPasteboard directly.
//==============================================================================

namespace ClipboardHelper
{
    /** Returns any file paths currently on the system clipboard.
        Filters to only audio files supported by the app (wav, aiff, mp3, flac…).
        Returns an empty array if the clipboard holds no recognised audio files. */
    juce::Array<juce::File> getAudioFilesFromClipboard();

    /** Returns the system clipboard's current change count.
        Each external copy increments this value; use it to detect whether the
        system clipboard has been updated since the last internal copy. */
    int getChangeCount();
}
