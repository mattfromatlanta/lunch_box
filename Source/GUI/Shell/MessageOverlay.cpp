// SPDX-License-Identifier: AGPL-3.0-or-later
#include "MessageOverlay.h"
#include "UIColours.h"
#include "LunchBoxFonts.h"

MessageOverlay::MessageOverlay()
{
    setInterceptsMouseClicks(true, true);
    setWantsKeyboardFocus(false);
}

MessageOverlay::~MessageOverlay()
{
    if (auto* top = getTopLevelComponent())
        top->removeKeyListener(this);
}

void MessageOverlay::show(const juce::String& msg)
{
    message = msg;
    setVisible(true);
    toFront(false);
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<MessageOverlay>(this)]
        { if (safe != nullptr) safe->grabKeyboardFocus(); });
    repaint();
}

void MessageOverlay::visibilityChanged()
{
    if (auto* top = getTopLevelComponent())
    {
        if (isVisible()) top->addKeyListener(this);
        else             top->removeKeyListener(this);
    }
}

void MessageOverlay::mouseDown(const juce::MouseEvent&)
{
    dismiss();
}

void MessageOverlay::dismiss()
{
    setVisible(false);
}

bool MessageOverlay::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (key == juce::KeyPress::escapeKey)
    {
        dismiss();
        return true;
    }
    return false;
}

juce::Rectangle<int> MessageOverlay::dialogBounds() const
{
    return { (getWidth() - W) / 2, getHeight() - FOOTER_OFFSET - H, W, H };
}

void MessageOverlay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.55f));

    auto db = dialogBounds();
    g.setColour(LunchBoxColours::GRID);
    g.fillRoundedRectangle(db.toFloat(), 12.0f);

    g.setColour(LunchBoxColours::WHITE_CREAM);
    g.setFont(LunchBoxFonts::h3().withHeight(19.0f));  // ~20% smaller than the h3 title
    g.drawFittedText(message, db.reduced(INNER_PAD), juce::Justification::centred, 3);
}
