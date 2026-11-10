#include "ui/titlebar/MenuBar.h"
#include "ui/widgets/Branding.h"
#include "ui/widgets/PaintProbe.h"
#include "ui/theme/Fonts.h"
#include "ui/theme/Tokens.h"

namespace lithium
{

using namespace tokens;

namespace
{
    juce::Font nameFont()   { return fonts::ui (textSize::sm); }

    constexpr int brandInset = 14;    // from the strip's left edge (or the macOS traffic lights): the panels' edge
    // None: each menu item has half the bar's height of padding built in
    // (its hover highlight fills it), which already puts "File" about 14px
    // after the wordmark.
    constexpr int brandToMenus = 0;
    constexpr int minFreeWidth = 40;  // always leave some strip to drag the window by
    constexpr float sessionMargin = 12.0f;   // between the session name and the menus or window buttons

    float stringWidth (const juce::Font& f, const juce::String& s)   { return juce::GlyphArrangement::getStringWidth (f, s); }
}

MenuBar::MenuBar (juce::MenuBarModel* m)
    : model (m),
     #if JUCE_MAC
      menus (nullptr)        // the model is shown in the native macOS menu bar instead
     #else
      menus (m)
     #endif
{
    addAndMakeVisible (menus);

   #if ! JUCE_MAC   // macOS shows the window's own buttons in the strip (see resized())
    addAndMakeVisible (windowControls);
   #endif

}

void MenuBar::setSessionStatus (const juce::String& name, bool /*isUntitled*/, bool unsaved)
{
    if (name != sessionName || unsaved != unsavedChanges)
    {
        sessionName = name;
        unsavedChanges = unsaved;
        setTooltip (unsaved ? "Unsaved changes" : juce::String());
        repaint();
    }
}

void MenuBar::updateWindowState()
{
    windowControls.update();

   #if JUCE_MAC
    // The traffic lights leave the strip in full screen, so their space comes and goes.
    if (auto* window = dynamic_cast<MaximisableWindow*> (getWindow()))
    {
        if (window->getNativeButtonsWidth() != nativeButtonsWidth)
        {
            resized();
            repaint();
        }
    }
   #endif

    // Keep the OS window title (taskbar, Alt+Tab) in step with ours.
    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
    {
        auto title = sessionName + (unsavedChanges ? " *" : "") + " - Lithium Studio";

        if (window->getName() != title)
            window->setName (title);
    }
}

int MenuBar::getMenusWidth()
{
   #if JUCE_MAC
    return 0;
   #else
    // Size the menu bar to its items, so the empty space after "Help" belongs
    // to the strip itself and can be used to drag the window.
    int width = 0;
    auto names = model->getMenuBarNames();

    for (int i = 0; i < names.size(); ++i)
        width += getLookAndFeel().getMenuBarItemWidth (menus, i, names[i]);

    return width;
   #endif
}

juce::Component::WindowControlKind MenuBar::getWindowControlAt (juce::Point<float> p) const
{
    if (windowControls.getBounds().toFloat().contains (p))
        return windowControls.getControlAt (p - windowControls.getPosition().toFloat());

    if (menus.isVisible() && menus.getBounds().toFloat().contains (p))
        return WindowControlKind::client;   // the menus must stay clickable

    return getLocalBounds().toFloat().contains (p) ? WindowControlKind::caption
                                                   : WindowControlKind::client;
}

//==============================================================================
void MenuBar::mouseDown (const juce::MouseEvent& e)
{
    auto* window = getWindow();
    auto* maximisable = dynamic_cast<MaximisableWindow*> (window);
    draggingWindow = window != nullptr && maximisable != nullptr;

    if (! draggingWindow)
        return;

    restoreOnDrag = maximisable->isMaximised();
    dragOffset = e.getScreenPosition() - window->getScreenPosition();
}

void MenuBar::mouseDrag (const juce::MouseEvent& e)
{
    auto* window = getWindow();
    auto* maximisable = dynamic_cast<MaximisableWindow*> (window);

    if (! draggingWindow || maximisable == nullptr)
        return;

    // Wait for a deliberate drag, so a plain click or double-click isn't one.
    if (e.getDistanceFromDragStart() < 4)
        return;

    // Where supported (Linux), let the window manager run the drag: it
    // un-maximises and moves the window like any native one, with snapping.
    if (maximisable->startWindowManagerDrag())
    {
        draggingWindow = false;
        return;
    }

    if (restoreOnDrag)
    {
        restoreOnDrag = false;

        // Remember where along the title bar the mouse was, as a proportion,
        // then restore and keep the mouse at the same proportion of the
        // (narrower) restored window.
        const auto proportion = (float) dragOffset.x / (float) juce::jmax (1, window->getWidth());
        maximisable->setMaximised (false);
        dragOffset.x = juce::roundToInt (proportion * (float) window->getWidth());
    }

    window->setTopLeftPosition (e.getScreenPosition() - dragOffset);
}

void MenuBar::mouseDoubleClick (const juce::MouseEvent&)
{
    if (auto* window = dynamic_cast<MaximisableWindow*> (getWindow()))
        window->toggleMaximised();
}

//==============================================================================
void MenuBar::paint (juce::Graphics& g)
{
    LITHIUM_PERF_PAINT ("MenuBar");
    g.fillAll (theme().titleBarBg);

    g.setColour (theme().titleBarBorder);
    g.fillRect (getLocalBounds().removeFromBottom (1));

    // The branding, left of the menus (see resized()).
    if (! brandArea.isEmpty())
        branding::paint (g, brandArea.toFloat());

    // The session name, centred on the whole strip, but kept clear of the
    // menus and the window buttons: it moves aside first, then ends in an
    // ellipsis. Then the unsaved-changes dot.
    const float dotSpace = unsavedChanges ? 12.0f : 0.0f;
    const auto space = freeArea.toFloat().reduced (sessionMargin, 0.0f);

    if (space.getWidth() <= dotSpace)
        return;

    const float total = juce::jmin (stringWidth (nameFont(), sessionName) + dotSpace, space.getWidth());
    const auto bar = getLocalBounds().withTrimmedBottom (1).toFloat();
    const auto x = juce::jlimit (space.getX(), space.getRight() - total, bar.getCentreX() - total * 0.5f);
    auto title = juce::Rectangle<float> (x, bar.getY(), total, bar.getHeight());

    auto name = title.removeFromLeft (title.getWidth() - dotSpace);
    g.setColour (theme().textSecondary);
    g.setFont (nameFont());
    g.drawText (sessionName, name, juce::Justification::centredLeft, true);   // true: an ellipsis if it doesn't fit

    if (unsavedChanges)
        g.fillEllipse (title.withSizeKeepingCentre (6.0f, 6.0f).translated (3.0f, 0.0f));
}

void MenuBar::resized()
{
    auto r = getLocalBounds().withTrimmedBottom (1);

   #if JUCE_MAC
    // Keep clear of the native traffic lights, which the window draws over the strip.
    auto* window = dynamic_cast<MaximisableWindow*> (getWindow());
    nativeButtonsWidth = window != nullptr ? window->getNativeButtonsWidth() : 0;
    r.removeFromLeft (nativeButtonsWidth);
   #else
    windowControls.setBounds (r.removeFromRight (WindowControls::preferredWidth));
   #endif

    // Item widths depend on the menu bar's height (JUCE adds padding based on
    // it), so give the menus their height before measuring them.
    menus.setBounds (r.withWidth (0));
    const int menusWidth = getMenusWidth();

    // The branding first, then the menus. In a very narrow window the
    // branding gives way before the menus or the draggable strip do.
    r.removeFromLeft (brandInset);
    const int brandRoom = r.getWidth() - brandToMenus - menusWidth - minFreeWidth;
    brandArea = r.removeFromLeft (juce::jlimit (0, juce::jmax (0, brandRoom), branding::getWidth()));
    r.removeFromLeft (brandToMenus);

    menus.setBounds (r.removeFromLeft (juce::jmin (menusWidth, r.getWidth())));

    freeArea = r;
}

} // namespace lithium
