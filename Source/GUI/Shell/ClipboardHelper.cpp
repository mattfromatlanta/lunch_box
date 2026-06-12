// SPDX-License-Identifier: AGPL-3.0-or-later
// Non-macOS fallback. Reading file paths from the system clipboard requires
// platform pasteboard APIs (see ClipboardHelper.mm); on other platforms
// external-app paste is unavailable, so these report an empty clipboard.
#include "ClipboardHelper.h"

namespace ClipboardHelper
{

juce::Array<juce::File> getAudioFilesFromClipboard()
{
    return {};
}

int getChangeCount()
{
    return 0;
}

} // namespace ClipboardHelper
