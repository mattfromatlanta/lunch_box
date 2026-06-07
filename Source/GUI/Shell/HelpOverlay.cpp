// SPDX-License-Identifier: AGPL-3.0-or-later
#include "HelpOverlay.h"
#include "UIColours.h"
#include "UIConstants.h"
#include "LunchBoxFonts.h"

namespace
{
    const char* kSection1 =
        "Lunch Box packs samples for the Chompi sampler. Add samples to slots by dragging, pasting, or clicking in to browse. "
        "Switch to Pack view to see the entire pack and Bank mode to focus on one bank "
        "at a time. Cubbi is for one-shots and Jammi is for pitched samples.";

    const char* kSection2 =
        "Clear to empty the current Pack or Bank. Fill to open the browser so you can "
        "select a source folder. Pack to export. Lunch Box will reformat your samples "
        "and organize them into banks ready for Chompi.";

    const char* kSection3 =
        "When you Fill to a Pack from a folder, samples that are organized into A, B, "
        "C, D, and E sub-folders will be arranged that way in Lunch Box automatically.";

    struct HotkeyRow { const char* key; const char* desc; };

    constexpr HotkeyRow kHotkeys[] = {
        { "Arrows",          "Move focus"               },
        { "Shift+Arrows",    "Expand selection"         },
        { "Tab",             "Pack / Bank view"         },
        { "Shift+Tab",       "Cubbi / Jammi"            },
        { "Cmd+Up / Down",   "Change bank (Bank mode)"  },
        { "Space",           "Preview"                  },
        { "Return",          "Browse"                   },
        { "Del / Bksp",      "Clear slot"               },
        { "Esc",             "Clear selection"          },
        { "Cmd+Z / Cmd+Shift+Z", "Undo / Redo"          },
        { "Cmd+C / X / V",  "Copy / Cut / Paste"        },
        { "Cmd+A",           "Select all"               },
        { "Cmd+F",           "Fill"                     },
        { "Cmd+Return",      "Process"                  },
        { "Cmd+O",           "Open output"              },
    };

    constexpr int NUM_HOTKEY_ROWS = 15;
    constexpr int HOTKEY_ROW_H   = 22;
    constexpr int SECTION_GAP    = 14;
    constexpr int CORNER_R       = 12.0f;
}

HelpOverlay::HelpOverlay()
{
    setInterceptsMouseClicks(true, true);
    setWantsKeyboardFocus(false);
}

HelpOverlay::~HelpOverlay()
{
    if (auto* top = getTopLevelComponent())
        top->removeKeyListener(this);
}

void HelpOverlay::visibilityChanged()
{
    if (auto* top = getTopLevelComponent())
    {
        if (isVisible()) top->addKeyListener(this);
        else             top->removeKeyListener(this);
    }
}

void HelpOverlay::show()
{
    setVisible(true);
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<HelpOverlay>(this)]
        { if (safe != nullptr) safe->grabKeyboardFocus(); });
}

void HelpOverlay::mouseDown(const juce::MouseEvent&)
{
    dismiss();
}

void HelpOverlay::dismiss()
{
    setVisible(false);
}

bool HelpOverlay::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (key == juce::KeyPress::escapeKey)
    {
        dismiss();
        return true;
    }
    return false;
}

// ─── Layout ───────────────────────────────────────────────────────────────────

int HelpOverlay::computeDialogHeight() const
{
    if (cachedH >= 0)
        return cachedH;

    const float cw = (float)(W - 2 * INNER_PAD);

    auto measureParagraph = [cw](const char* text, const juce::Font& font) -> int
    {
        juce::AttributedString as;
        as.append(text, font, juce::Colours::white);
        as.setJustification(juce::Justification::topLeft);
        juce::TextLayout layout;
        layout.createLayout(as, cw);
        return (int)std::ceil(layout.getHeight());
    };

    int h = INNER_PAD;
    h += measureParagraph(kSection1, juce::Font(juce::FontOptions{}.withTypeface(LunchBoxFonts::regular()).withHeight(18.0f)));
    h += SECTION_GAP;
    h += measureParagraph(kSection2, juce::Font(juce::FontOptions{}.withTypeface(LunchBoxFonts::regular()).withHeight(18.0f)));
    h += SECTION_GAP;
    h += measureParagraph(kSection3, juce::Font(juce::FontOptions{}.withTypeface(LunchBoxFonts::regular()).withHeight(18.0f)));
    h += SECTION_GAP;
    h += (int)std::ceil(juce::Font(juce::FontOptions{}.withTypeface(LunchBoxFonts::regular()).withHeight(18.0f)).getHeight());  // "Hotkeys" label
    h += 8;
    h += NUM_HOTKEY_ROWS * HOTKEY_ROW_H;
    h += INNER_PAD;

    cachedH = h;
    return cachedH;
}

juce::Rectangle<int> HelpOverlay::dialogBounds() const
{
    const int h = computeDialogHeight();
    return { (getWidth() - W) / 2, LunchBoxConstants::HEADER_HEIGHT, W, h };
}

// ─── Paint ────────────────────────────────────────────────────────────────────

void HelpOverlay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.55f));

    auto db = dialogBounds();
    g.setColour(LunchBoxColours::GRID);
    g.fillRoundedRectangle(db.toFloat(), CORNER_R);

    const float cw = (float)(W - 2 * INNER_PAD);
    float x   = (float)(db.getX() + INNER_PAD);
    float y   = (float)(db.getY() + INNER_PAD);
    const juce::Colour textCol = LunchBoxColours::WHITE_CREAM;
    const juce::Colour dimCol  = LunchBoxColours::WHITE_CREAM.withAlpha(0.7f);

    // ── Helper: draw a wrapped paragraph, advance y ────────────────────────
    auto drawParagraph = [&](const char* text, const juce::Font& font, juce::Colour colour)
    {
        juce::AttributedString as;
        as.append(text, font, colour);
        as.setJustification(juce::Justification::topLeft);
        juce::TextLayout layout;
        layout.createLayout(as, cw);
        layout.draw(g, juce::Rectangle<float>(x, y, cw, layout.getHeight() + 2.0f));
        y += std::ceil(layout.getHeight());
    };

    // ── Sections 1-3 ──────────────────────────────────────────────────────
    drawParagraph(kSection1, juce::Font(juce::FontOptions{}.withTypeface(LunchBoxFonts::regular()).withHeight(18.0f)), textCol);
    y += SECTION_GAP;
    drawParagraph(kSection2, juce::Font(juce::FontOptions{}.withTypeface(LunchBoxFonts::regular()).withHeight(18.0f)), textCol);
    y += SECTION_GAP;
    drawParagraph(kSection3, juce::Font(juce::FontOptions{}.withTypeface(LunchBoxFonts::regular()).withHeight(18.0f)), textCol);
    y += SECTION_GAP;

    // ── Hotkeys header ────────────────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions{}.withTypeface(LunchBoxFonts::regular()).withHeight(18.0f)));
    g.setColour(textCol);
    float headerH = juce::Font(juce::FontOptions{}.withTypeface(LunchBoxFonts::regular()).withHeight(18.0f)).getHeight();
    g.drawText("Hotkeys", (int)x, (int)y, (int)cw, (int)headerH, juce::Justification::centredLeft);
    y += std::ceil(headerH) + 8.0f;

    // ── Separator line ────────────────────────────────────────────────────
    g.setColour(textCol.withAlpha(0.2f));
    g.drawLine(x, y, x + cw, y, 1.0f);
    y += 5.0f;

    // ── Single-column hotkey rows ─────────────────────────────────────────
    const float keyW  = cw * 0.46f;
    const float descW = cw * 0.54f;

    const juce::Font keyFont  = LunchBoxFonts::nav(16.0f);
    const juce::Font descFont = LunchBoxFonts::nav(16.0f);

    for (int row = 0; row < NUM_HOTKEY_ROWS; ++row)
    {
        float rowY = y + (float)(row * HOTKEY_ROW_H);

        g.setFont(keyFont);
        g.setColour(textCol);
        g.drawText(kHotkeys[row].key, (int)x, (int)rowY, (int)keyW, HOTKEY_ROW_H,
                   juce::Justification::centredLeft, true);

        g.setFont(descFont);
        g.setColour(dimCol);
        g.drawText(kHotkeys[row].desc, (int)(x + keyW), (int)rowY, (int)descW, HOTKEY_ROW_H,
                   juce::Justification::centredLeft, true);
    }
}
