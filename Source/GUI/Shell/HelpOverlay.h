// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// HelpOverlay — quick-reference card shown when the user clicks the app title
//==============================================================================
class HelpOverlay : public juce::Component, private juce::KeyListener
{
public:
    HelpOverlay();
    ~HelpOverlay() override;

    void show();
    void visibilityChanged() override;
    void mouseDown(const juce::MouseEvent&) override;
    void paint(juce::Graphics& g) override;
    void resized() override {}

    juce::Rectangle<int> dialogBounds() const;

    static constexpr int W         = 380;
    static constexpr int INNER_PAD = 20;

private:
    using juce::Component::keyPressed;
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    void dismiss();

    mutable int cachedH = -1;
    int computeDialogHeight() const;
};
