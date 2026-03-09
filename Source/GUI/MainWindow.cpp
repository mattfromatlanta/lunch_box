#include "MainWindow.h"

MainWindow::MainWindow(juce::String name)
    : DocumentWindow(name,
                     juce::Desktop::getInstance().getDefaultLookAndFeel()
                         .findColour(juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);

    mainComponent = std::make_unique<MainComponent>();
    setContentOwned(mainComponent.get(), true);

    #if JUCE_IOS || JUCE_ANDROID
        setFullScreen(true);
    #else
        setResizable(true, false);
        setResizeLimits(400, 500, 900, 10000);
        centreWithSize(getWidth(), getHeight());
    #endif

    setVisible(true);
}

MainWindow::~MainWindow()
{
    mainComponent = nullptr;
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
