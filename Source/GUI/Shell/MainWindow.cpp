// SPDX-License-Identifier: AGPL-3.0-or-later
#include "MainWindow.h"
#include "../Style/LunchBoxFonts.h"

// Tooltip that only appears after the cursor has been still for the full delay,
// and disappears immediately on intentional mouse movement.
//
// Intentional movement = >= 3 logical pixels since the last recorded position.
// This filters out trackpad micro-jitter that would otherwise keep the timer
// from ever completing.
static bool g_tooltipsSuppressed = false;

void suppressTooltipsUntilMouseMove() { g_tooltipsSuppressed = true; }

struct TooltipLAF : public juce::LookAndFeel_V4
{
    void drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h) override
    {
        juce::Rectangle<int> bounds (w, h);
        g.setColour (findColour (juce::TooltipWindow::backgroundColourId));
        g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
        g.setColour (findColour (juce::TooltipWindow::outlineColourId));
        g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 4.0f, 1.0f);
        g.setColour (findColour (juce::TooltipWindow::textColourId));
        g.setFont (juce::Font (juce::FontOptions{}.withTypeface (LunchBoxFonts::regular()).withHeight (16.0f)));
        g.drawFittedText (text, bounds.reduced (6, 3), juce::Justification::centred, 1);
    }
};

struct LunchBoxTooltipWindow : public juce::TooltipWindow
{
    LunchBoxTooltipWindow (juce::Component* parent, int delayMs)
        : juce::TooltipWindow (parent, delayMs)
    {
        setLookAndFeel (&laf);
        juce::Desktop::getInstance().addGlobalMouseListener (this);
    }

    ~LunchBoxTooltipWindow() override
    {
        setLookAndFeel (nullptr);
        juce::Desktop::getInstance().removeGlobalMouseListener (this);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const auto pos = e.getScreenPosition().toFloat();
        if (pos.getDistanceFrom (lastPos) >= 3.0f)
        {
            g_tooltipsSuppressed = false;
            hideTip();
            lastMoveMs = juce::Time::getApproximateMillisecondCounter();
            lastPos = pos;
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override { mouseMove (e); }

    // Return the real tip only once the cursor has been still for > 200ms.
    // While returning "", JUCE's internal countdown stays reset, so the full
    // 1200ms delay begins only after the cursor actually settles.
    juce::String getTipFor (juce::Component& c) override
    {
        if (g_tooltipsSuppressed)
            return {};
        const auto elapsed = (juce::int64) juce::Time::getApproximateMillisecondCounter()
                           - (juce::int64) lastMoveMs;
        if (elapsed < 200)
            return {};
        return juce::TooltipWindow::getTipFor (c);
    }

private:
    TooltipLAF         laf;
    juce::uint32       lastMoveMs = 0;
    juce::Point<float> lastPos;
};

MainWindow::MainWindow(juce::String name)
    : DocumentWindow(name,
                     juce::Desktop::getInstance().getDefaultLookAndFeel()
                         .findColour(juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);

    setContentOwned(new MainComponent(), true);

    tooltipWindow = std::make_unique<LunchBoxTooltipWindow>(this, 1200);

    #if JUCE_IOS || JUCE_ANDROID
        setFullScreen(true);
    #else
        setResizable(true, false);
        setResizeLimits(420, 500, 800, 10000);
        centreWithSize(getWidth(), getHeight());
    #endif

    setVisible(true);
}

MainWindow::~MainWindow()
{
    tooltipWindow = nullptr;
}

void MainWindow::closeButtonPressed()
{
    if (auto* mc = getMainComponent())
        mc->saveSessionState();

    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
