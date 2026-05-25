// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include "GuiProcessor.h"
#include "LunchBoxFonts.h"
#include "UIColours.h"
#include "UIConstants.h"
#include "PreviewPanel.h"
#include "BankEditorPanel.h"
#include "BankFocusPanel.h"
#if CHOMPI_MELATONIN_INSPECTOR
#include "melatonin_inspector/melatonin_inspector.h"
#endif

class ConsoleWindow;
void suppressTooltipsUntilMouseMove();

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

            // Build the advancing (target) region path — same tooth shape on both buttons.
            // The half+full+half pattern on the retreating button emerges naturally as the
            // complement: the remaining fromFill has valleys where the teeth bit in at H/4, 3H/4,
            // which is exactly where the incoming button's tips point.
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

//==============================================================================
// MainComponent - Main GUI component for Lunch Box
//==============================================================================

class MainComponent : public juce::Component,
                      public juce::KeyListener,
                      public juce::ApplicationCommandTarget
{
public:
    // Command IDs — used by AppMenuBar to wire menu items with keyboard shortcuts
    enum CommandIDs
    {
        cmdCopy          = 0x2001,
        cmdCut           = 0x2002,
        cmdPaste         = 0x2003,
        cmdToggleConsole = 0x2004,
        cmdUndo          = 0x2005,
        cmdRedo          = 0x2006,
        cmdSelectAll     = 0x2007,
        cmdOpenOutput    = 0x2008,
        cmdProcess       = 0x2009,
        cmdFill          = 0x200a,
        cmdClear         = 0x200b,
    };

    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

    // Public interface for menu bar actions
    void selectOutputFolder();
    void processFiles();
    void showLogFolder();
    void clearStatusLog();
    void setShowRuntimeLogs(bool shouldShow);

    void saveSessionState();
    bool getShowRuntimeLogs() const { return showRuntimeLogs; }
    bool getConsoleVisible()  const { return consoleVisible; }
    void toggleConsole();

    // Edit operations (also invoked via ApplicationCommandTarget::perform)
    void editCopy();
    void editCut();
    void editPaste();

    // ApplicationCommandTarget
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>&) override;
    void getCommandInfo(juce::CommandID, juce::ApplicationCommandInfo&) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo&) override;

    // Shared command manager — AppMenuBar uses this for addCommandItem
    juce::ApplicationCommandManager commandManager;

private:

    // Mode toggle (Pack / Bank)
    enum class ViewMode { Pack, Bank };
    ViewMode viewMode = ViewMode::Pack;

    WipeTabButton packModeButton;
    WipeTabButton bankModeButton;
    bool consoleVisible = false;
    juce::String     consoleContent { "Ready to process samples...\n" };
    std::unique_ptr<ConsoleWindow> consoleWindow;

    // ── Pack mode ───────────────────────────────────────────
    WipeTabButton cubbiTabButton;
    WipeTabButton jammiTabButton;
    bool showCubbiEditor = true;

    std::unique_ptr<BankEditorPanel> cubbiEditor;
    std::unique_ptr<BankEditorPanel> jammiEditor;

    // ── Bank focus mode ────────────────────────────────────
    std::unique_ptr<BankFocusPanel> bankFocusPanel;
    juce::Label bankStatusLabel;

    // ── Output folder (shared) ────────────────────────────
    juce::File         outputBaseFolder;     // full output folder path
    juce::String       lastPackName { "lunch_box" };

    struct FooterButtonLAF : public juce::LookAndFeel_V4
    {
        juce::Font getTextButtonFont(juce::TextButton&, int) override { return LunchBoxFonts::h3(); }

        void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                                  const juce::Colour& backgroundColour,
                                  bool isHighlighted, bool) override
        {
            auto bounds = btn.getLocalBounds().reduced(1).toFloat();
            g.setColour(isHighlighted ? backgroundColour.brighter(0.1f) : backgroundColour);
            g.fillRoundedRectangle(bounds, LunchBoxConstants::CORNER_RADIUS);
            g.setColour(LunchBoxColours::WHITE_CREAM.withAlpha(0.3f));
            g.drawRoundedRectangle(bounds, LunchBoxConstants::CORNER_RADIUS, LunchBoxConstants::BORDER_WIDTH_ACTIVE);
        }
    } footerButtonLAF;

    struct PackButton : public juce::TextButton, private juce::Timer
    {
        void triggerSuccessAnimation()
        {
            animPhase = 0.0f;
            startTimerHz(60);
        }

        void paint(juce::Graphics& g) override
        {
            getLookAndFeel().drawButtonBackground(g, *this,
                findColour(isDown() ? buttonOnColourId : buttonColourId),
                isOver(), isDown());

            auto bounds = getLocalBounds().toFloat();
            auto colour = findColour(textColourOffId).withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f);

            g.saveState();
            g.reduceClipRegion(getLocalBounds());
            g.setFont(LunchBoxFonts::h3());
            g.setColour(colour);

            if (isTimerRunning())
            {
                float eased = 1.0f - std::pow(1.0f - animPhase, 3.0f);
                float h = bounds.getHeight();
                g.drawText("Pack", bounds.translated(0.0f,  eased * h),        juce::Justification::centred, false);
                g.drawText("Pack", bounds.translated(0.0f, (eased - 1.0f) * h), juce::Justification::centred, false);
            }
            else
            {
                g.drawText("Pack", bounds, juce::Justification::centred, false);
            }

            g.restoreState();
        }

    private:
        void timerCallback() override
        {
            animPhase += 1.0f / (60.0f * 0.28f);
            if (animPhase >= 1.0f)
            {
                animPhase = 1.0f;
                stopTimer();
            }
            repaint();
        }

        float animPhase = 1.0f;
    } processButton;

    struct PackNameOverlay : public juce::Component, private juce::KeyListener
    {
        std::function<void(bool, juce::String)> onResult;

        PackNameOverlay()
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

            // Register on each child so we intercept Tab before it handles it internally
            nameEditor.addKeyListener(this);
            continueButton.addKeyListener(this);
            cancelButton.addKeyListener(this);

            setInterceptsMouseClicks(true, true);
            setWantsKeyboardFocus(false);
        }

        ~PackNameOverlay() override
        {
            nameEditor.removeKeyListener(this);
            continueButton.removeKeyListener(this);
            cancelButton.removeKeyListener(this);
            if (auto* top = getTopLevelComponent())
                top->removeKeyListener(this);
            continueButton.setLookAndFeel(nullptr);
            cancelButton.setLookAndFeel(nullptr);
        }

        void visibilityChanged() override
        {
            if (auto* top = getTopLevelComponent())
            {
                if (isVisible()) top->addKeyListener(this);
                else             top->removeKeyListener(this);
            }
        }

        void show(const juce::String& name)
        {
            nameEditor.setText(name, false);
            nameEditor.selectAll();
            setVisible(true);
            juce::MessageManager::callAsync(
                [safe = juce::Component::SafePointer<PackNameOverlay>(this)]
                { if (safe != nullptr) safe->nameEditor.grabKeyboardFocus(); });
        }

        void mouseDown(const juce::MouseEvent&) override {}  // absorb — do not cancel on outside click

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::black.withAlpha(0.55f));
            auto db = dialogBounds();
            g.setColour(LunchBoxColours::GRID);
            g.fillRoundedRectangle(db.toFloat(), 12.0f);

            // Title — centred
            g.setFont(LunchBoxFonts::h3());
            g.setColour(LunchBoxColours::WHITE_CREAM);
            g.drawText("Name your pack.",
                       db.reduced(INNER_PAD).removeFromTop(TITLE_H),
                       juce::Justification::centred, false);

            // Underline — 85% of dialog width, centred, at bottom of editor area
            float ulW  = W * 0.85f;
            float ulX  = db.getX() + (W - ulW) * 0.5f;
            float ulY  = nameEditor.getBounds().getBottom() - 1.0f;
            g.setColour(LunchBoxColours::WHITE_CREAM.withAlpha(0.35f));
            g.drawLine(ulX, ulY, ulX + ulW, ulY, 1.5f);
        }

        void resized() override
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

        juce::Rectangle<int> dialogBounds() const
        {
            return { (getWidth() - W) / 2, getHeight() - FOOTER_OFFSET - H, W, H };
        }

        static constexpr int W             = 380;
        static constexpr int H             = 170;
        static constexpr int INNER_PAD     = 20;
        static constexpr int TITLE_H       = 28;
        static constexpr int EDITOR_H      = 36;
        static constexpr int BTN_H         = 40;
        static constexpr int GAP           = 12;
        static constexpr int FOOTER_OFFSET = 76;

    private:
        bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override
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

        void confirm()
        {
            auto name = nameEditor.getText().trim();
            if (name.isEmpty()) return;
            setVisible(false);
            if (onResult) onResult(true, name);
        }
        void cancel()
        {
            setVisible(false);
            if (onResult) onResult(false, {});
        }

        struct BtnLAF : public juce::LookAndFeel_V4
        {
            juce::Font getTextButtonFont(juce::TextButton&, int) override { return LunchBoxFonts::body(); }
            void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                                      const juce::Colour& bg, bool hi, bool) override
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
        } btnLAF;

        juce::TextEditor nameEditor;
        juce::TextButton continueButton, cancelButton;
    } packNameOverlay;

    juce::TextButton fillButton;
    juce::TextButton clearButton;

    PreviewPanel previewPanel;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<GuiProcessor> processor;

    juce::ApplicationProperties appProperties;
    juce::Array<juce::File> sampleClipboard;       // shared between Pack and Bank views
    int lastInternalCopyChangeCount = -1;          // NSPasteboard changeCount at last internal copy

    bool showRuntimeLogs = false;

    // Mode switching
    void setViewMode(ViewMode mode);
    void setCategoryTab(bool showCubbi, bool animate = false);
    void styleTabButton(WipeTabButton& btn, bool active, juce::Colour activeCol);
    juce::Rectangle<int> computeContentArea() const;
    bool isTransitioning = false;
    std::unique_ptr<juce::ImageComponent> bankTransitionOverlay;
    void animateBankCategorySwitch(bool showCubbi);

    // Cross-tab data sync
    void syncPackToBankFocus();
    void syncBankFocusToAdvanced();

    // Helpers
    juce::File getResolvedOutputFolder();
    BankEditorPanel* getActiveEditor();  // returns the visible Pack-mode editor

    // Persistent folder preferences
    juce::File getSavedFolder(const juce::String& key);
    void       saveFolder(const juce::String& key, const juce::File& folder);
    void       saveString(const juce::String& key, const juce::String& value);
    juce::String getSavedString(const juce::String& key, const juce::String& fallback = {});

    // Session persistence
    void loadSessionState();

    // Undo / redo
    struct UndoState
    {
        juce::File cubbiSlots[LunchBoxNamer::NUM_BANKS][LunchBoxNamer::SLOTS_PER_BANK];
        juce::File jammiSlots[LunchBoxNamer::NUM_BANKS][LunchBoxNamer::SLOTS_PER_BANK];
    };

    static constexpr int MAX_UNDO_STEPS = 10;
    juce::Array<UndoState> undoStack;
    juce::Array<UndoState> redoStack;
    bool isApplyingUndoState = false;

    UndoState readCurrentState();
    void captureUndoState();
    void applyUndoState(const UndoState& state);
    void performUndo();
    void performRedo();


    // Preview
    void stopPreview();

    // juce::KeyListener
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;

    // Processing
    void processFilesFromEditors();
    void updateProcessButtonState();
    void appendStatus(const juce::String& message);
    void appendProcessingResult(const GuiProcessor::ProcessingResult& result, const juce::File& outputFolder);

#if CHOMPI_MELATONIN_INSPECTOR
    std::unique_ptr<melatonin::Inspector> inspector;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
