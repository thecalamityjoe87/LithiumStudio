#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace lithium
{

//==============================================================================
/**
    Implemented by the app's main window, so title-bar code in src/ui can
    maximise/restore it without knowing how each OS does that.
*/
class MaximisableWindow
{
public:
    virtual ~MaximisableWindow() = default;

    virtual bool isMaximised() const = 0;
    virtual void setMaximised (bool) = 0;

    /** Hands an in-progress title-bar drag over to the window manager, which
        then moves the window (and un-maximises it) itself, with the desktop's
        usual snapping. Returns false where that isn't supported; the caller
        then moves the window itself. */
    virtual bool startWindowManagerDrag() = 0;

    /** How much of the title bar's left edge the OS's own window buttons use
        (macOS), or 0 where the app draws them (WindowControls). */
    virtual int getNativeButtonsWidth() const = 0;

    void toggleMaximised()      { setMaximised (! isMaximised()); }
};

//==============================================================================
/**
    Minimise / maximise-restore / close buttons, drawn by the app so they look
    the same on Windows and every Linux desktop. (macOS uses the system's
    own traffic lights instead; see MainWindow.) They match the Lithium
    Drums standalone window's buttons (JUCE's LookAndFeel_V4 ones): an ochre
    line, a green plus and a red cross, filled with their colour on hover.

    They act on whatever DocumentWindow this component lives in, so they can be
    placed anywhere in the UI (we put them in the MenuBar strip).
*/
class WindowControls  : public juce::Component
{
public:
    WindowControls();

    /** Keeps the maximise/restore icon in sync with the window state. */
    void update();

    /** Which control is at a point (in this component's coordinates), for the
        OS hit-testing done in MainWindow::findControlAtPoint. */
    juce::Component::WindowControlKind getControlAt (juce::Point<float>) const;

    void resized() override;

    static constexpr int buttonWidth = 30;   // square, as tall as the title bar (as JUCE's own buttons are)
    static constexpr int preferredWidth = buttonWidth * 3;

private:
    /** One button: its glyph in its colour, or the colour filling the button
        with the glyph cut out of it while the mouse is over it. */
    class WindowButton  : public juce::Button
    {
    public:
        WindowButton (const juce::String& name, juce::Colour, juce::Path normal, juce::Path toggled);
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;

    private:
        juce::Colour colour;
        juce::Path normalShape, toggledShape;
    };

    WindowButton minimiseButton, maximiseButton, closeButton;

    bool showingRestore = false;

    juce::DocumentWindow* getWindow() const;
    MaximisableWindow* getMaximisableWindow() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowControls)
};

} // namespace lithium
