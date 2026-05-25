// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "DragModel.h"

//==============================================================================
// DragProxy — translucent overlay that paints the moving cells under the cursor
// during a drag. Sits on top of the host panel; fully transparent to mouse
// events. Fades to a dimmer state after the cursor has been idle for 200 ms.
//==============================================================================
class DragProxy : public juce::Component,
                  private juce::Timer
{
public:
    DragProxy();
    ~DragProxy() override;

    // One moving cell: its file (for the thumbnail/label), the on-screen
    // bounds at drag start (used to compute the offset from the pickup cell).
    struct ProxyCell
    {
        LunchBoxDrag::GridCell cell;
        juce::Rectangle<int>   originalBounds;   // panel coords, at pickup
        juce::File             file;
    };

    // Begin a new drag visualization. `pickupAnchor` is the panel-coord point
    // where the cursor was when the drag started (typically the pickup cell's
    // top-left or the precise mouseDown position). The proxy follows the cursor
    // and renders each cell at `(cursorPos - pickupAnchor) + cell.originalBounds`.
    void begin(juce::Array<ProxyCell> cells,
               juce::Point<int>       pickupAnchor);

    // Cursor moved — repaint at new position. Resets the idle-fade timer.
    void updateCursor(juce::Point<int> cursorPanelPt);

    // End the drag visual. Hides the proxy and forgets all state.
    void finish();

    bool isActive() const { return active; }

    void paint(juce::Graphics&) override;

private:
    void timerCallback() override;     // 200ms idle → fade
    float currentAlpha() const;

    static constexpr int   kIdleFadeAfterMs = 200;
    static constexpr float kActiveAlpha     = 0.40f;
    static constexpr float kIdleAlpha       = 0.20f;
    static constexpr int   kFadeTweenMs     = 100;

    bool                       active = false;
    juce::Array<ProxyCell>     cells;
    juce::Point<int>           pickupAnchor;
    juce::Point<int>           cursorPos;
    juce::Time                 lastMoveTime;
    bool                       fadedToIdle = false;
    juce::Time                 fadeStartTime;
    float                      fadeFrom = kActiveAlpha;
    float                      fadeTo   = kActiveAlpha;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragProxy)
};
