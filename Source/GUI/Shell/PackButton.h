// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include "LunchBoxFonts.h"

//==============================================================================
// PackButton — TextButton with a vertical text-slide success animation
//==============================================================================
struct PackButton : public juce::TextButton, private juce::Timer
{
    void triggerSuccessAnimation()
    {
        animPhase = 0.0f;
        startTimerHz(60);
    }

    void paint(juce::Graphics& g) override
    {
        getLookAndFeel().drawButtonBackground(g, *this,
            findColour(isDown() ? buttonOnColourId : buttonColourId),
            isOver(), isDown());

        auto bounds = getLocalBounds().toFloat();
        auto colour = findColour(textColourOffId).withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f);

        g.saveState();
        g.reduceClipRegion(getLocalBounds());
        g.setFont(LunchBoxFonts::h3());
        g.setColour(colour);

        if (isTimerRunning())
        {
            float eased = 1.0f - std::pow(1.0f - animPhase, 3.0f);
            float h = bounds.getHeight();
            g.drawText("Pack", bounds.translated(0.0f,  eased * h),        juce::Justification::centred, false);
            g.drawText("Pack", bounds.translated(0.0f, (eased - 1.0f) * h), juce::Justification::centred, false);
        }
        else
        {
            g.drawText("Pack", bounds, juce::Justification::centred, false);
        }

        g.restoreState();
    }

private:
    void timerCallback() override
    {
        animPhase += 1.0f / (60.0f * 0.28f);
        if (animPhase >= 1.0f)
        {
            animPhase = 1.0f;
            stopTimer();
        }
        repaint();
    }

    float animPhase = 1.0f;
};
