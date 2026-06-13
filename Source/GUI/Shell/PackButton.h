// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "LunchBoxFonts.h"

//==============================================================================
// PackButton — TextButton whose "Pack" label rolls vertically while exporting.
//==============================================================================
// One animation: a single constant-rate vertical roll. startSpin() begins it;
// stopSpin() lowers the `spinning` flag but lets the current cycle play out, so
// the roll halts exactly at the wrap point — "Pack" comes to rest centred where
// it started, with no speed change or visual pop.
//==============================================================================
struct PackButton : public juce::TextButton, private juce::Timer
{
    // Begin the constant-rate roll (no-op if already spinning).
    void startSpin()
    {
        if (spinning) return;
        spinning  = true;
        animPhase = 0.0f;
        startTimerHz(60);
    }

    // Request a stop: the in-progress cycle finishes and then the roll halts.
    void stopSpin()
    {
        spinning = false;
    }

    // Fired (on the message thread) the moment the roll comes fully to rest,
    // i.e. one cycle after stopSpin(). Lets the owner re-enable the button only
    // once the animation is truly finished.
    std::function<void()> onAnimationStopped;

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override
    {
        getLookAndFeel().drawButtonBackground(g, *this,
            findColour(shouldDrawButtonAsDown ? buttonOnColourId : buttonColourId),
            shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        auto bounds = getLocalBounds().toFloat();
        // Keep the label fully opaque even when disabled (during export) — the
        // spin animation, not dimming, signals the deactivated state.
        auto colour = findColour(textColourOffId);

        g.saveState();
        g.reduceClipRegion(getLocalBounds());
        g.setFont(LunchBoxFonts::h3());
        g.setColour(colour);

        if (isTimerRunning())
        {
            // Constant-speed vertical roll: one copy leaves as the next enters.
            float h = bounds.getHeight();
            g.drawText("Pack", bounds.translated(0.0f,  animPhase * h),         juce::Justification::centred, false);
            g.drawText("Pack", bounds.translated(0.0f, (animPhase - 1.0f) * h), juce::Justification::centred, false);
        }
        else
        {
            g.drawText("Pack", bounds, juce::Justification::centred, false);
        }

        g.restoreState();
    }

private:
    static constexpr float SECONDS_PER_CYCLE = 0.6f;

    void timerCallback() override
    {
        animPhase += 1.0f / (60.0f * SECONDS_PER_CYCLE);

        if (animPhase >= 1.0f)
        {
            if (spinning)
            {
                animPhase -= 1.0f;   // keep looping at a constant rate
            }
            else
            {
                // Stop requested: halt at the wrap, resting centred on "Pack".
                animPhase = 0.0f;
                stopTimer();
                repaint();
                if (onAnimationStopped) onAnimationStopped();
                return;
            }
        }

        repaint();
    }

    bool  spinning  = false;
    float animPhase = 0.0f;
};
