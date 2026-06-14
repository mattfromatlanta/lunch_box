// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Persistent preferences (folders, strings) and full session save/restore.

#include "MainComponent.h"

juce::File MainComponent::getSavedFolder(const juce::String& key)
{
    if (auto* prefs = appProperties.getUserSettings())
    {
        auto path = prefs->getValue(key);
        if (path.isNotEmpty())
        {
            juce::File f(path);
            if (f.isDirectory()) return f;
        }
    }
    return juce::File{};
}

void MainComponent::saveFolder(const juce::String& key, const juce::File& folder)
{
    if (auto* prefs = appProperties.getUserSettings())
    {
        prefs->setValue(key, folder.getFullPathName());
        prefs->saveIfNeeded();
    }
}

juce::String MainComponent::getSavedString(const juce::String& key,
                                            const juce::String& fallback)
{
    if (auto* prefs = appProperties.getUserSettings())
    {
        auto val = prefs->getValue(key);
        if (val.isNotEmpty()) return val;
    }
    return fallback;
}

void MainComponent::saveString(const juce::String& key, const juce::String& value)
{
    if (auto* prefs = appProperties.getUserSettings())
    {
        prefs->setValue(key, value);
        prefs->saveIfNeeded();
    }
}

void MainComponent::saveSessionState()
{
    auto* props = appProperties.getUserSettings();
    if (!props) return;

    auto state = readCurrentState();

    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
        {
            props->setValue("session_cubbi_b" + juce::String(b) + "_s" + juce::String(s),
                            state.cubbiSlots[b][s].getFullPathName());
            props->setValue("session_jammi_b" + juce::String(b) + "_s" + juce::String(s),
                            state.jammiSlots[b][s].getFullPathName());
        }

    props->saveIfNeeded();
}

void MainComponent::loadSessionState()
{
    auto* props = appProperties.getUserSettings();
    if (!props) return;

    juce::StringArray missing;

    for (int b = 0; b < LunchBoxNamer::NUM_BANKS; ++b)
        for (int s = 0; s < LunchBoxNamer::SLOTS_PER_BANK; ++s)
        {
            for (int cat = 0; cat < 2; ++cat)
            {
                auto prefix = (cat == 0) ? "cubbi" : "jammi";
                auto path = props->getValue("session_" + juce::String(prefix)
                                            + "_b" + juce::String(b) + "_s" + juce::String(s));
                if (path.isEmpty()) continue;

                juce::File f(path);
                if (f.existsAsFile())
                {
                    auto* editor = (cat == 0) ? cubbiEditor.get() : jammiEditor.get();
                    editor->setSlotFile(b, s, f);
                }
                else
                {
                    missing.add(path);
                }
            }
        }

    if (!missing.isEmpty())
    {
        appendStatus("Session restore: " + juce::String(missing.size()) + " file(s) not available:");
        for (auto& path : missing)
            appendStatus("  Missing: " + path);
    }

    updateProcessButtonState();
}

// ─── File browser launchers ───────────────────────────────────────────────────
