// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LunchBoxFonts.h"
#include "UIColours.h"
#include "UIConstants.h"

//==============================================================================
// WipeTabButton — TextButton with a horizontal colour-wipe animation
//==============================================================================
class WipeTabButton : public juce::TextButton,
                      private juce::Timer
{
public:
    WipeTabButton() = default;

    void snapToColour (juce::Colour col)
    {
        stopTimer();
        currentFill  = col;
        wipeProgress = 1.0f;
        repaint();
    }

    // Animate from current colour to target, sweeping in from the right (fromRight=true)
    // or from the left (fromRight=false), over durationMs milliseconds.
    void startWipe (juce::Colour target, bool fromRight, int durationMs = LunchBoxConstants::ANIM_DURATION_MS)
    {
        stopTimer();
        fromFill      = currentFill;
        targetFill    = target;
        wipeFromRight = fromRight;
        wipeProgress  = 0.0f;
        stepPerFrame  = 1.0f / (float (durationMs) * 60.0f / 1000.0f);
        startTimerHz (60);
    }

private:
    void timerCallback() override
    {
        wipeProgress += stepPerFrame;
        if (wipeProgress >= 1.0f)
        {
            wipeProgress = 1.0f;
            currentFill  = targetFill;
            stopTimer();
        }
        repaint();
    }

    void paintButton (juce::Graphics& g, bool isMouseOver, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        const float r = LunchBoxConstants::CORNER_RADIUS;
        const float W = b.getWidth();
        const float H = b.getHeight();

        if (wipeProgress > 0.0f && wipeProgress < 1.0f)
        {
            // 2 stacked triangles, each H/2 tall with a 90° tip → depth = H/4
            const float depth = H / 4.0f;

            // Match the cosine ease-in/ease-out used by JUCE's animateComponent (startSpeed=0, endSpeed=0)
            const float eased = 0.5f * (1.0f - std::cos (wipeProgress * juce::MathConstants<float>::pi));

            // Edge centre travels from fully off-screen to fully off-screen
            const float edgeCenter = wipeFromRight
                ? (W + depth) - (W + 2.0f * depth) * eased   // W+depth → -depth
                : -depth       + (W + 2.0f * depth) * eased;  // -depth  →  W+depth

            // backX = base of teeth (behind the edge); frontX = tip of teeth (leading point)
            const float bX     = b.getX();
            const float bY     = b.getY();
            const float backX  = bX + edgeCenter + (wipeFromRight ?  depth : -depth);
            const float frontX = bX + edgeCenter + (wipeFromRight ? -depth :  depth);
            const float originX = wipeFromRight ? bX + W : bX;

            // Fill entire button with the receding colour
            g.setColour (LunchBoxColours::getTabBg (fromFill));
            g.fillRoundedRectangle (b, r);

            // Build the advancing (target) region path
            juce::Path leadPath;
            leadPath.startNewSubPath (originX, bY);
            leadPath.lineTo (backX, bY);
            for (int i = 0; i < 2; ++i)
            {
                leadPath.lineTo (backX,  bY + H * float(i)          / 2.0f);
                leadPath.lineTo (frontX, bY + H * (float(i) + 0.5f) / 2.0f);
                leadPath.lineTo (backX,  bY + H * float(i + 1)      / 2.0f);
            }
            leadPath.lineTo (originX, bY + H);
            leadPath.closeSubPath();

            // Paint target colour clipped to the button's rounded shape
            {
                g.saveState();
                juce::Path clip;
                clip.addRoundedRectangle (b, r);
                g.reduceClipRegion (clip);
                g.setColour (LunchBoxColours::getTabBg (targetFill));
                g.fillPath (leadPath);
                g.restoreState();
            }

            // Stroke the tooth edge with the accent colour
            {
                juce::Path edgePath;
                edgePath.startNewSubPath (backX, bY);
                for (int i = 0; i < 2; ++i)
                {
                    edgePath.lineTo (backX,  bY + H * float(i)          / 2.0f);
                    edgePath.lineTo (frontX, bY + H * (float(i) + 0.5f) / 2.0f);
                    edgePath.lineTo (backX,  bY + H * float(i + 1)      / 2.0f);
                }

                g.saveState();
                juce::Path clip;
                clip.addRoundedRectangle (b, r);
                g.reduceClipRegion (clip);
                g.setColour (LunchBoxColours::WHITE_CREAM.withAlpha (0.3f));
                g.strokePath (edgePath, juce::PathStrokeType (LunchBoxConstants::BORDER_WIDTH_ACTIVE));
                g.restoreState();
            }
        }
        else
        {
            const bool inactive = (currentFill == LunchBoxColours::BUTTON_BG);
            auto fill = LunchBoxColours::getTabBg(currentFill);
            if (inactive && isMouseOver)
                fill = fill.brighter(0.1f);
            g.setColour (fill);
            g.fillRoundedRectangle (b, r);
        }

        g.setColour (LunchBoxColours::WHITE_CREAM.withAlpha(0.3f));
        g.drawRoundedRectangle (b, r, LunchBoxConstants::BORDER_WIDTH_ACTIVE);

        g.setColour (findColour (juce::TextButton::textColourOffId));
        g.setFont (LunchBoxFonts::nav());
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred, false);
    }

    juce::Colour fromFill    = LunchBoxColours::BUTTON_BG;
    juce::Colour targetFill  = LunchBoxColours::BUTTON_BG;
    juce::Colour currentFill = LunchBoxColours::BUTTON_BG;
    float        wipeProgress = 1.0f;
    float        stepPerFrame = 1.0f / 12.0f;
    bool         wipeFromRight = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WipeTabButton)
};
