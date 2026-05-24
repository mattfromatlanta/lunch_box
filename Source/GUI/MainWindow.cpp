#include "MainWindow.h"

// Tooltip that only appears after the cursor has been still for the full delay,
// and disappears immediately on intentional mouse movement.
//
// Intentional movement = >= 3 logical pixels since the last recorded position.
// This filters out trackpad micro-jitter that would otherwise keep the timer
// from ever completing.
struct ChompiTooltipWindow : public juce::TooltipWindow
{
    ChompiTooltipWindow (juce::Component* parent, int delayMs)
        : juce::TooltipWindow (parent, delayMs)
    {
        juce::Desktop::getInstance().addGlobalMouseListener (this);
    }

    ~ChompiTooltipWindow() override
    {
        juce::Desktop::getInstance().removeGlobalMouseListener (this);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const auto pos = e.getScreenPosition().toFloat();
        if (pos.getDistanceFrom (lastPos) >= 3.0f)
        {
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
        const auto elapsed = (juce::int64) juce::Time::getApproximateMillisecondCounter()
                           - (juce::int64) lastMoveMs;
        if (elapsed < 200)
            return {};
        return juce::TooltipWindow::getTipFor (c);
    }

private:
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

    mainComponent = std::make_unique<MainComponent>();
    setContentOwned(mainComponent.get(), true);

    tooltipWindow = std::make_unique<ChompiTooltipWindow>(this, 1200);

    #if JUCE_IOS || JUCE_ANDROID
        setFullScreen(true);
    #else
        setResizable(true, false);
        setResizeLimits(350, 500, 800, 10000);
        centreWithSize(getWidth(), getHeight());
    #endif

    setVisible(true);
}

MainWindow::~MainWindow()
{
    tooltipWindow = nullptr;
    mainComponent = nullptr;
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
