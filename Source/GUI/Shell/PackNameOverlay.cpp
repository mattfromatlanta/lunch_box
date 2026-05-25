// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PackNameOverlay.h"
#include "UIColours.h"

PackNameOverlay::PackNameOverlay()
{
    nameEditor.setFont(LunchBoxFonts::body());
    nameEditor.setMultiLine(false);
    nameEditor.setJustification(juce::Justification::centred);
    nameEditor.setIndents(0, 0);
    nameEditor.setColour(juce::TextEditor::backgroundColourId,     juce::Colours::transparentBlack);
    nameEditor.setColour(juce::TextEditor::textColourId,           LunchBoxColours::WHITE_CREAM);
    nameEditor.setColour(juce::TextEditor::outlineColourId,        juce::Colours::transparentBlack);
    nameEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    nameEditor.setColour(juce::TextEditor::highlightColourId,      LunchBoxColours::PURPLE.withAlpha(0.4f));
    nameEditor.onReturnKey = [this] { confirm(); };
    nameEditor.onEscapeKey = [this] { cancel(); };
    addAndMakeVisible(nameEditor);

    auto setupBtn = [this](juce::TextButton& btn, juce::Colour bg, float alpha)
    {
        btn.setColour(juce::TextButton::buttonColourId,  bg);
        btn.setColour(juce::TextButton::buttonOnColourId,bg);
        btn.setColour(juce::TextButton::textColourOffId, LunchBoxColours::WHITE_CREAM.withAlpha(alpha));
        btn.setColour(juce::TextButton::textColourOnId,  LunchBoxColours::WHITE_CREAM.withAlpha(alpha));
        btn.setLookAndFeel(&btnLAF);
    };

    continueButton.setButtonText("Continue");
    setupBtn(continueButton, LunchBoxColours::BUTTON_BG, 1.0f);
    continueButton.onClick = [this] { confirm(); };
    addAndMakeVisible(continueButton);

    cancelButton.setButtonText("Cancel");
    setupBtn(cancelButton, LunchBoxColours::DARK_GREY, 0.6f);
    cancelButton.onClick = [this] { cancel(); };
    addAndMakeVisible(cancelButton);

    cancelButton.setColour(juce::TextButton::textColourOffId, LunchBoxColours::WHITE_CREAM.withAlpha(0.7f));
    cancelButton.setColour(juce::TextButton::textColourOnId,  LunchBoxColours::WHITE_CREAM.withAlpha(0.7f));

    nameEditor.addKeyListener(this);
    continueButton.addKeyListener(this);
    cancelButton.addKeyListener(this);

    setInterceptsMouseClicks(true, true);
    setWantsKeyboardFocus(false);
}

PackNameOverlay::~PackNameOverlay()
{
    nameEditor.removeKeyListener(this);
    continueButton.removeKeyListener(this);
    cancelButton.removeKeyListener(this);
    if (auto* top = getTopLevelComponent())
        top->removeKeyListener(this);
    continueButton.setLookAndFeel(nullptr);
    cancelButton.setLookAndFeel(nullptr);
}

void PackNameOverlay::visibilityChanged()
{
    if (auto* top = getTopLevelComponent())
    {
        if (isVisible()) top->addKeyListener(this);
        else             top->removeKeyListener(this);
    }
}

void PackNameOverlay::show(const juce::String& name)
{
    nameEditor.setText(name, false);
    nameEditor.selectAll();
    setVisible(true);
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<PackNameOverlay>(this)]
        { if (safe != nullptr) safe->nameEditor.grabKeyboardFocus(); });
}

void PackNameOverlay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.55f));
    auto db = dialogBounds();
    g.setColour(LunchBoxColours::GRID);
    g.fillRoundedRectangle(db.toFloat(), 12.0f);

    g.setFont(LunchBoxFonts::h3());
    g.setColour(LunchBoxColours::WHITE_CREAM);
    g.drawText("Name your pack.",
               db.reduced(INNER_PAD).removeFromTop(TITLE_H),
               juce::Justification::centred, false);

    float ulW  = W * 0.85f;
    float ulX  = db.getX() + (W - ulW) * 0.5f;
    float ulY  = nameEditor.getBounds().getBottom() - 1.0f;
    g.setColour(LunchBoxColours::WHITE_CREAM.withAlpha(0.35f));
    g.drawLine(ulX, ulY, ulX + ulW, ulY, 1.5f);
}

void PackNameOverlay::resized()
{
    auto db   = dialogBounds();
    float ulW = W * 0.85f;
    int edX   = db.getX() + juce::roundToInt((W - ulW) * 0.5f);
    int edW   = juce::roundToInt(ulW);

    auto inner = db.reduced(INNER_PAD);
    inner.removeFromTop(TITLE_H + GAP);
    nameEditor.setBounds(edX, inner.getY(), edW, EDITOR_H);
    inner.removeFromTop(EDITOR_H + GAP);
    auto row = inner.removeFromTop(BTN_H).withX(db.getX() + INNER_PAD).withWidth(db.getWidth() - 2 * INNER_PAD);
    int bw = (row.getWidth() - GAP) / 2;
    cancelButton.setBounds(row.removeFromLeft(bw));
    row.removeFromLeft(GAP);
    continueButton.setBounds(row);
}

juce::Rectangle<int> PackNameOverlay::dialogBounds() const
{
    return { (getWidth() - W) / 2, getHeight() - FOOTER_OFFSET - H, W, H };
}

bool PackNameOverlay::keyPressed(const juce::KeyPress& key, juce::Component* origin)
{
    if (key == juce::KeyPress::escapeKey)
    {
        cancel();
        return true;
    }

    const bool isTab      = key.getKeyCode() == juce::KeyPress::tabKey;
    const bool isShiftTab = isTab && key.getModifiers().isShiftDown();

    if (isTab)
    {
        auto focusEditor = [this] { nameEditor.grabKeyboardFocus(); nameEditor.selectAll(); };

        if (isShiftTab)
        {
            if (origin == &nameEditor)          continueButton.grabKeyboardFocus();
            else if (origin == &continueButton) cancelButton.grabKeyboardFocus();
            else                                focusEditor();
        }
        else
        {
            if (origin == &nameEditor)          cancelButton.grabKeyboardFocus();
            else if (origin == &cancelButton)   continueButton.grabKeyboardFocus();
            else                                focusEditor();
        }
        return true;
    }

    return false;
}

void PackNameOverlay::confirm()
{
    auto name = nameEditor.getText().trim();
    if (name.isEmpty()) return;
    setVisible(false);
    if (onResult) onResult(true, name);
}

void PackNameOverlay::cancel()
{
    setVisible(false);
    if (onResult) onResult(false, {});
}

void PackNameOverlay::BtnLAF::drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                                                   const juce::Colour& bg, bool hi, bool)
{
    auto bounds = btn.getLocalBounds().toFloat();
    g.setColour(hi ? bg.brighter(0.12f) : bg);
    g.fillRoundedRectangle(bounds, 12.0f);
    if (btn.hasKeyboardFocus(false))
    {
        auto outlineColour = btn.findColour(juce::TextButton::textColourOffId);
        g.setColour(outlineColour);
        g.drawRoundedRectangle(bounds.reduced(1.0f), 12.0f, 1.5f);
    }
}
