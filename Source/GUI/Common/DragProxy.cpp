// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DragProxy.h"
#include "UIColours.h"

DragProxy::DragProxy()
{
    setInterceptsMouseClicks(false, false);
    setWantsKeyboardFocus(false);
    setVisible(false);
}

DragProxy::~DragProxy() = default;

void DragProxy::begin(juce::Array<ProxyCell> c, juce::Point<int> anchor)
{
    cells          = std::move(c);
    pickupAnchor   = anchor;
    cursorPos      = anchor;
    lastMoveTime   = juce::Time::getCurrentTime();
    fadedToIdle    = false;
    fadeFrom       = kActiveAlpha;
    fadeTo         = kActiveAlpha;
    fadeStartTime  = lastMoveTime;
    active         = true;
    setVisible(true);
    startTimerHz(30);
    repaint();
}

void DragProxy::updateCursor(juce::Point<int> pt)
{
    if (! active) return;
    if (pt == cursorPos) return;

    cursorPos    = pt;
    lastMoveTime = juce::Time::getCurrentTime();

    if (fadedToIdle)
    {
        fadedToIdle   = false;
        fadeFrom      = currentAlpha();
        fadeTo        = kActiveAlpha;
        fadeStartTime = lastMoveTime;
    }

    repaint();
}

void DragProxy::finish()
{
    active = false;
    cells.clear();
    stopTimer();
    setVisible(false);
}

void DragProxy::timerCallback()
{
    if (! active) return;

    const auto now = juce::Time::getCurrentTime();

    // Idle detection
    if (! fadedToIdle
        && (now - lastMoveTime).inMilliseconds() >= kIdleFadeAfterMs)
    {
        fadedToIdle   = true;
        fadeFrom      = currentAlpha();
        fadeTo        = kIdleAlpha;
        fadeStartTime = now;
    }

    // Drive the tween repaints
    auto sinceFade = (now - fadeStartTime).inMilliseconds();
    if (sinceFade < kFadeTweenMs)
        repaint();
}

float DragProxy::currentAlpha() const
{
    const auto sinceFade = (juce::Time::getCurrentTime() - fadeStartTime).inMilliseconds();
    if (sinceFade >= kFadeTweenMs) return fadeTo;

    const float t = (float) sinceFade / (float) kFadeTweenMs;
    return fadeFrom + (fadeTo - fadeFrom) * t;
}

void DragProxy::paint(juce::Graphics& g)
{
    if (! active || cells.isEmpty()) return;

    const auto offset = cursorPos - pickupAnchor;
    const float alpha = currentAlpha();

    for (const auto& pc : cells)
    {
        auto r = pc.originalBounds.translated(offset.x, offset.y).toFloat();

        // Outer rounded rect — translucent slot-style fill.
        g.setColour(LunchBoxColours::SLOT_FILLED_BG.withAlpha(alpha));
        g.fillRoundedRectangle(r, 6.0f);

        // Subtle border so the shape reads clearly against any background.
        g.setColour(LunchBoxColours::WHITE_CREAM.withAlpha(alpha * 0.7f));
        g.drawRoundedRectangle(r, 6.0f, 1.5f);

        // Label: filename without extension, centered. (Empty cells render as
        // a blank pill — the user still sees the shape traveling.)
        if (pc.file != juce::File{})
        {
            g.setColour(LunchBoxColours::WHITE_CREAM.withAlpha(alpha));
            g.setFont(11.0f);
            g.drawFittedText(pc.file.getFileNameWithoutExtension(),
                             r.toNearestInt().reduced(4),
                             juce::Justification::centred, 1);
        }
    }
}
