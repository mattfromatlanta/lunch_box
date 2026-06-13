// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// MessageOverlay — brief modal notice (e.g. "Please add some samples to pack.")
//==============================================================================
// Uses the same card styling and bottom position as the pack-name save dialog,
// and is dismissed by a click anywhere or Esc, like the HelpOverlay.
//==============================================================================
class MessageOverlay : public juce::Component, private juce::KeyListener
{
public:
    MessageOverlay();
    ~MessageOverlay() override;

    void show(const juce::String& message);

    void visibilityChanged() override;
    void mouseDown(const juce::MouseEvent&) override;
    void paint(juce::Graphics& g) override;

    juce::Rectangle<int> dialogBounds() const;

    // Match PackNameOverlay's width and bottom offset; height suits one message.
    static constexpr int W             = 380;
    static constexpr int H             = 110;
    static constexpr int INNER_PAD     = 20;
    static constexpr int FOOTER_OFFSET = 76;

private:
    using juce::Component::keyPressed;
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    void dismiss();

    juce::String message;
};
