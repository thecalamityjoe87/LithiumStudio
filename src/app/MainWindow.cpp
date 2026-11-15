#include "app/MainWindow.h"
#include "engine/Preferences.h"
#include "platform/WindowManagerHints.h"
#include "ui/theme/Tokens.h"

namespace lithium
{

MainWindow::MainWindow (const juce::String& name, AudioEngine& e)
    : juce::DocumentWindow (name, theme().windowBg, juce::DocumentWindow::allButtons), engine (e)
{
    // No JUCE title bar: the app's MenuBar strip is the title bar.
    // On Windows and Linux there's no OS title bar either, and the strip has
    // its own window buttons (see ui/titlebar). On macOS the window keeps its
    // native title bar, made transparent so the system's traffic lights sit
    // in the strip (see platform::useInlineTitleBar).
   #if JUCE_MAC
    setUsingNativeTitleBar (true);
   #else
    setUsingNativeTitleBar (false);
    setTitleBarHeight (0);
   #endif
    setTitleBarButtonsRequired (0, false);

    // setContentOwned: the window deletes the component when it closes.
    content = new MainComponent (engine);
    setContentOwned (content, true);

    setResizable (true, false);
    setResizeLimits (MainComponent::minWindowWidth, MainComponent::minWindowHeight, 16384, 16384);

   #if JUCE_MAC
    // After setResizable(), which recreates the native window.
    platform::useInlineTitleBar (*this);
   #endif

    // Where the window was when the app last quit, kept on a screen that's
    // still there; otherwise centred at the default size. Either way no
    // bigger than the screen (a large interface size makes the window's
    // units bigger on it).
    auto& prefs = engine.getPreferences();
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto saved = juce::Rectangle<int>::fromString (prefs.get (settings::windowBounds));
    const auto* display = saved.isEmpty() ? displays.getPrimaryDisplay() : displays.getDisplayForRect (saved);

    if (display == nullptr)
        display = displays.getPrimaryDisplay();

    if (display != nullptr)
    {
        const auto area = display->userBounds.toNearestInt();

        if (! saved.isEmpty() && area.intersects (saved))
        {
            setBounds (saved.withSize (juce::jmin (saved.getWidth(), area.getWidth()),
                                       juce::jmin (saved.getHeight(), area.getHeight()))
                            .constrainedWithin (area));
        }
        else
        {
            centreWithSize (juce::jmin (getWidth(), area.getWidth()), juce::jmin (getHeight(), area.getHeight()));
        }
    }
    else
    {
        centreWithSize (getWidth(), getHeight());
    }

    restoredBounds = getBounds();
    setVisible (true);

    // Maximised last time: maximise it once the window manager has it (see
    // timerCallback), just as the maximise button does.
    maximiseWhenReady = prefs.get (settings::windowMaximised);

    // The OS or window manager can also maximise/restore the window (e.g.
    // Super+Up on GNOME, or Aero Snap on Windows), so keep checking. Quicker
    // at first while it's waiting to maximise, so the normal size barely shows.
    startTimerHz (maximiseWhenReady ? 50 : 5);
}

MainWindow::~MainWindow()
{
    auto& prefs = engine.getPreferences();
    prefs.set (settings::windowMaximised, maximised || maximiseWhenReady);
    prefs.set (settings::windowBounds, restoredBounds.toString());   // the size it goes back to from maximised
}

void MainWindow::closeButtonPressed()
{
    // Route through the app so unsaved changes are checked.
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

//==============================================================================
void MainWindow::setMaximised (bool shouldBeMaximised)
{
    if (shouldBeMaximised)
    {
        if (! maximised)
            restoredBounds = getBounds();

        platform::setMaximised (*this, true);
        setMaximisedFlag (true);
    }
    else
    {
        platform::setMaximised (*this, false);
        setMaximisedFlag (false);

        // Put the window back at its restored size. (On Linux the window
        // manager also restores its own saved geometry; they match.)
        setBounds (restoredBounds);
    }
}

bool MainWindow::startWindowManagerDrag()
{
    return platform::startWindowManagerDrag (*this);
}

void MainWindow::setMaximisedFlag (bool m)
{
    if (m != maximised)
    {
        maximised = m;
        resized();   // re-layout for the changed border
        repaint();
    }
}

int MainWindow::getNativeButtonsWidth() const
{
   #if JUCE_MAC
    return platform::getInlineTitleBarButtonsWidth (*this);
   #else
    return 0;
   #endif
}

void MainWindow::timerCallback()
{
    if (maximiseWhenReady)
    {
        // Give up waiting after about two seconds and ask anyway.
        if (! platform::isReadyToMaximise (*this) && ++ticksWaitingToMaximise < 100)
            return;

        maximiseWhenReady = false;
        setMaximised (true);
        startTimerHz (5);
        return;
    }

    setMaximisedFlag (platform::isMaximised (*this));
}

//==============================================================================
// setTitleBarButtonsRequired (0) hides JUCE's own buttons, but it also drops
// these flags. On Windows, JUCE ignores clicks on the button areas reported by
// findControlAtPoint unless they're set, and they're also what gives the
// window Snap Layouts and minimise-from-taskbar. On macOS they enable the
// native close and minimise buttons. (Linux doesn't need them: there they
// would change the window-manager hints.)
int MainWindow::getDesktopWindowStyleFlags() const
{
    auto flags = juce::DocumentWindow::getDesktopWindowStyleFlags();

   #if JUCE_WINDOWS || JUCE_MAC
    flags |= juce::ComponentPeer::windowHasMinimiseButton
           | juce::ComponentPeer::windowHasMaximiseButton
           | juce::ComponentPeer::windowHasCloseButton;
   #endif

    return flags;
}

//==============================================================================
// No resize border while maximised (the window can't be resized then).
// Overriding this instead of calling setResizable() matters: changing
// resizability recreates the native window, losing its maximised state.
juce::BorderSize<int> MainWindow::getBorderThickness() const
{
    return maximised ? juce::BorderSize<int>() : juce::DocumentWindow::getBorderThickness();
}

void MainWindow::moved()
{
    juce::DocumentWindow::moved();
    rememberRestoredBounds();
}

void MainWindow::resized()
{
    juce::DocumentWindow::resized();
    rememberRestoredBounds();
}

void MainWindow::rememberRestoredBounds()
{
    if (maximised || ! isShowing())
        return;

    // While the window manager is still maximising us, a resize can arrive
    // before our state flag catches up. Don't remember a size that fills the
    // screen as the "restored" size.
    if (auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect (getBounds()))
        if (getWidth() >= display->userBounds.getWidth() - 8 && getHeight() >= display->userBounds.getHeight() - 8)
            return;

    restoredBounds = getBounds();
}

//==============================================================================
// Tells the OS which parts of our custom title bar are the caption and which
// are window buttons. On Windows this keeps Aero Snap, Snap Layouts and
// double-click-to-maximise working natively.
juce::Component::WindowControlKind MainWindow::findControlAtPoint (juce::Point<float> p) const
{
    // The resize borders are handled by DocumentWindow.
    auto kind = juce::DocumentWindow::findControlAtPoint (p);

    if (kind != WindowControlKind::client || content == nullptr)
        return kind;

    // Convert to content coordinates with plain arithmetic: calling
    // getLocalPoint() here could recurse into the OS hit-test.
    auto border = getContentComponentBorder();
    return content->getWindowControlAt (p - juce::Point<float> ((float) border.getLeft(), (float) border.getTop()));
}

} // namespace lithium
