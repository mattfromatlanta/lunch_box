// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include "LunchBoxFonts.h"
#include "UIColours.h"
#include "UIConstants.h"
#include "LabelStrings.h"

//==============================================================================
// ConsoleWindow — floating status log window owned by MainComponent.
// MainComponent appends text to consoleContent and pushes it here when visible.
//==============================================================================
class ConsoleWindow : public juce::DocumentWindow
{
public:
    explicit ConsoleWindow(const juce::String& initialContent)
        : juce::DocumentWindow(LunchBoxLabels::kConsoleTitle, LunchBoxColours::CONSOLE_BG, DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        editor.setMultiLine(true);
        editor.setReadOnly(true);
        editor.setScrollbarsShown(true);
        editor.setCaretVisible(false);
        editor.setFont(juce::Font(juce::FontOptions{}
                                      .withName(juce::Font::getDefaultMonospacedFontName())
                                      .withHeight(LunchBoxConstants::CONSOLE_FONT_SIZE)));
        editor.setColour(juce::TextEditor::backgroundColourId, LunchBoxColours::CONSOLE_BG);
        editor.setColour(juce::TextEditor::textColourId,       LunchBoxColours::CONSOLE_TEXT);
        editor.setColour(juce::TextEditor::outlineColourId,    LunchBoxColours::CONSOLE_OUTLINE);
        editor.setText(initialContent);
        setContentNonOwned(&editor, true);
        setSize(LunchBoxConstants::CONSOLE_WIDTH, LunchBoxConstants::CONSOLE_HEIGHT);
        setResizable(true, false);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        if (onClose) onClose();
    }

    juce::TextEditor editor;
    std::function<void()> onClose;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleWindow)
};
