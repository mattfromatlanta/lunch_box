// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "LunchBoxFonts.h"

//==============================================================================
// PackNameOverlay — modal dialog overlay for naming an output pack folder
//==============================================================================
class PackNameOverlay : public juce::Component, private juce::KeyListener
{
public:
    std::function<void(bool, juce::String)> onResult;

    PackNameOverlay();
    ~PackNameOverlay() override;

    void show(const juce::String& name);

    void visibilityChanged() override;
    void mouseDown(const juce::MouseEvent&) override {}  // absorb — do not cancel on outside click
    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::Rectangle<int> dialogBounds() const;

    static constexpr int W             = 380;
    static constexpr int H             = 170;
    static constexpr int INNER_PAD     = 20;
    static constexpr int TITLE_H       = 28;
    static constexpr int EDITOR_H      = 36;
    static constexpr int BTN_H         = 40;
    static constexpr int GAP           = 12;
    static constexpr int FOOTER_OFFSET = 76;

private:
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    void confirm();
    void cancel();

    struct BtnLAF : public juce::LookAndFeel_V4
    {
        juce::Font getTextButtonFont(juce::TextButton&, int) override { return LunchBoxFonts::body(); }
        void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                                  const juce::Colour& bg, bool hi, bool) override;
    } btnLAF;

    juce::TextEditor nameEditor;
    juce::TextButton continueButton, cancelButton;
};
