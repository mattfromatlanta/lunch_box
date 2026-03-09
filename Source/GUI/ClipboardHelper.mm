#include "ClipboardHelper.h"
#include "../FileSystemHelper.h"

#import <AppKit/AppKit.h>

namespace ClipboardHelper
{

juce::Array<juce::File> getAudioFilesFromClipboard()
{
    juce::Array<juce::File> result;
    NSPasteboard* pb = [NSPasteboard generalPasteboard];
    juce::StringArray paths;

    // ── Strategy 1: enumerate pasteboard items, look for public.file-url ─────
    // This is the modern path written by Finder (macOS 10.14+) and most apps.
    NSArray<NSPasteboardItem*>* items = pb.pasteboardItems;
    if (items != nil)
    {
        for (NSPasteboardItem* item in items)
        {
            // public.file-url — a percent-encoded file:// URL string
            NSString* urlStr = [item stringForType:@"public.file-url"];
            if (urlStr != nil)
            {
                NSURL* url = [NSURL URLWithString:urlStr];
                if (url != nil && url.isFileURL && url.path != nil)
                {
                    paths.add(juce::String::fromUTF8(url.path.UTF8String));
                    continue;
                }
            }

            // com.apple.pasteboard.promised-file-url (Finder promise items)
            NSString* promStr = [item stringForType:@"com.apple.pasteboard.promised-file-url"];
            if (promStr != nil)
            {
                NSURL* url = [NSURL URLWithString:promStr];
                if (url != nil && url.isFileURL && url.path != nil)
                    paths.add(juce::String::fromUTF8(url.path.UTF8String));
            }
        }
    }

    // ── Strategy 2: legacy NSFilenamesPboardType ──────────────────────────────
    // Still written by many older and Electron apps.
    if (paths.isEmpty())
    {
        NSArray* names = [pb propertyListForType:@"NSFilenamesPboardType"];
        if ([names isKindOfClass:[NSArray class]])
            for (NSString* p in names)
                paths.add(juce::String::fromUTF8(p.UTF8String));
    }

    // ── Strategy 3: plain text that looks like a file path ───────────────────
    // Sononym copies file path(s) as plain text rather than file URL types.
    if (paths.isEmpty())
    {
        NSString* text = [pb stringForType:NSPasteboardTypeString];
        if (text != nil)
        {
            // May be one path per line
            NSArray<NSString*>* lines = [text componentsSeparatedByString:@"\n"];
            for (NSString* line in lines)
            {
                NSString* trimmed = [line stringByTrimmingCharactersInSet:
                                     [NSCharacterSet whitespaceAndNewlineCharacterSet]];
                if (trimmed.length > 0)
                    paths.add(juce::String::fromUTF8(trimmed.UTF8String));
            }
        }
    }

    if (paths.isEmpty())
        return result;

    // ── Filter for supported audio extensions ─────────────────────────────────
    // getSupportedAudioExtensions() returns glob patterns like "*.wav";
    // strip the leading "*" so we can compare against getFileExtension() (".wav").
    juce::StringArray supportedExts;
    for (const auto& glob : FileSystemHelper::getSupportedAudioExtensions())
        supportedExts.add(glob.fromFirstOccurrenceOf(".", true, false));

    for (const auto& path : paths)
    {
        juce::File f(path);
        if (!f.existsAsFile()) continue;

        if (supportedExts.contains(f.getFileExtension().toLowerCase()))
            result.add(f);
    }

    return result;
}

int getChangeCount()
{
    return (int)[[NSPasteboard generalPasteboard] changeCount];
}

} // namespace ClipboardHelper
